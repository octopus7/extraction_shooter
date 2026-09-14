#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_MeshRotationRate.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace FlameCandidates
{
const FString Root = TEXT("/Game/Effects/FlameProjectiles/");
template<class T> T* Asset(const FString& Name)
{
    UPackage* P = CreatePackage(*(Root + Name));
    T* A = NewObject<T>(P, *Name, RF_Public | RF_Standalone | RF_Transactional);
    FAssetRegistryModule::AssetCreated(A);
    return A;
}
bool Save(UObject* A)
{
    A->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(A->GetPackage(), A, *FPackageName::LongPackageNameToFilename(A->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
template<class T> T* Node(UMaterial* M)
{
    T* N = NewObject<T>(M);
    M->GetExpressionCollection().AddExpression(N);
    return N;
}
UTexture2D* Atlas()
{
    constexpr int32 Tile=128, Side=Tile*4;
    TArray<FColor> Pixels;
    Pixels.SetNum(Side*Side);
    for(int32 Y=0;Y<Side;++Y) for(int32 X=0;X<Side;++X)
    {
        float U=(X%Tile+0.5f)/Tile, V=(Y%Tile+0.5f)/Tile;
        float T=((X/Tile)+(Y/Tile)*4)*2*PI/16;
        float Center=0.5f+0.085f*FMath::Sin(U*13-T)+0.035f*FMath::Sin(U*29+T*2);
        float W=0.37f*(1-U)+0.03f;
        float Waves=0.055f*FMath::Sin(U*24-T*2)+0.028f*FMath::Sin(U*43+T*3);
        float Mask=FMath::Clamp((W+Waves-FMath::Abs(V-Center))*27,0.f,1.f);
        Mask*=FMath::Clamp((1-U)*12,0.f,1.f);
        uint8 C=FMath::RoundToInt(Mask*255);
        Pixels[Y*Side+X]=FColor(C,C,C,255);
    }
    UTexture2D* T=Asset<UTexture2D>(TEXT("T_FlameComet_Flipbook4x4"));
    T->Source.Init(Side,Side,1,1,TSF_BGRA8,(uint8*)Pixels.GetData());
    T->SRGB=false; T->CompressionSettings=TC_Masks; T->MipGenSettings=TMGS_NoMipmaps;
    T->AddressX=TA_Clamp; T->AddressY=TA_Clamp;
    T->PostEditChange(); Save(T); return T;
}
UMaterial* Material(const FString& Name,int Type,int Layer,UTexture2D* Tex)
{
    UMaterial* M=Asset<UMaterial>(Name);
    M->SetShadingModel(MSM_Unlit); M->BlendMode=BLEND_Additive; M->TwoSided=true;
    bool Recompile=false; M->SetMaterialUsage(Recompile,MATUSAGE_NiagaraMeshParticles); M->SetMaterialUsage(Recompile,MATUSAGE_NiagaraSprites);
    auto* UV=Node<UMaterialExpressionTextureCoordinate>(M);
    auto* Time=Node<UMaterialExpressionTime>(M);
    auto* Age=Node<UMaterialExpressionParticleRelativeTime>(M);
    auto* C=Node<UMaterialExpressionCustom>(M);
    C->OutputType=CMOT_Float4;
    C->Description=TEXT("Stylized flame flow: X forward, tail UV.x runs toward -X");
    auto Input=[&](FName N,UMaterialExpression* E){FCustomInput I;I.InputName=N;I.Input.Expression=E;C->Inputs.Add(I);};
    C->Inputs.Empty(); Input(TEXT("UV"),UV);Input(TEXT("T"),Time);Input(TEXT("Age"),Age);
    if(Tex){auto* TN=Node<UMaterialExpressionTextureObject>(M);TN->Texture=Tex;Input(TEXT("Atlas"),TN);}
    FString Code;
    if(Layer==2)
    {
        Code=TEXT("float2 p=UV*2-1; float a=saturate((1-dot(p,p))*4); a*=pow(saturate(1-Age),1.5); return float4(float3(1,0.25,0.035)*5,a);");
    }
    else if(Layer==0)
    {
        Code=TEXT("float flow=0.5+0.5*sin(UV.x*21-T*13+sin(UV.y*18));float seam=smoothstep(0.64,0.8,flow);float3 c=lerp(float3(1,0.16,0.008),float3(1,0.84,0.3),seam);return float4(c*3.5,0.85);");
    }
    else
    {
        Code=TEXT("float u=UV.x;float v=UV.y;float flow=0.5+0.5*sin(u*24-T*10+sin(v*17+T*3)*1.6);float a=0;");
        if(Type==1) Code+=TEXT("float f=floor(frac(T*1.5)*16);float2 tile=float2(fmod(f,4),floor(f/4));float2 at=(tile+clamp(UV,0.008,0.992))/4;a=Texture2DSample(Atlas,AtlasSampler,at).r;");
        else Code+=TEXT("float center=0.5+0.09*sin(u*18-T*9)+0.025*sin(u*43-T*16);float width=0.41*(1-u)+0.018;float rag=0.05*sin(u*30-T*14);a=saturate((width+rag-abs(v-center))*26)*saturate((1-u)*10);");
        Code+=TEXT("float bands=smoothstep(0.37,0.56,flow);float hot=smoothstep(0.72,0.88,flow)*(1-u);float3 col=lerp(float3(1,0.055,0.003),float3(1,0.37,0.012),bands);col=lerp(col,float3(1,0.9,0.35),hot);return float4(col*3,a*(0.7+0.3*bands));");
    }
    C->Code=Code;
    auto* RGB=Node<UMaterialExpressionComponentMask>(M); RGB->Input.Expression=C;RGB->R=true;RGB->G=true;RGB->B=true;RGB->A=false;
    auto* Alpha=Node<UMaterialExpressionComponentMask>(M);Alpha->Input.Expression=C;Alpha->R=false;Alpha->G=false;Alpha->B=false;Alpha->A=true;
    M->GetEditorOnlyData()->EmissiveColor.Expression=RGB;M->GetEditorOnlyData()->Opacity.Expression=Alpha;
    M->PostEditChange();Save(M);return M;
}
struct Geometry
{
    FMeshDescription D;
    FStaticMeshAttributes A;
    FPolygonGroupID Group;
    Geometry():A(D){A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Flame");}
    void Quad(const FVector3f& P0,const FVector3f& P1,const FVector3f& P2,const FVector3f& P3,float U0,float U1)
    {
        FVector3f P[4]={P0,P1,P2,P3};FVector2f UV[4]={{U0,0},{U0,1},{U1,1},{U1,0}};
        TArray<FVertexInstanceID> IDs;
        FVector3f N=FVector3f::CrossProduct(P1-P0,P3-P0).GetSafeNormal();
        for(int K=0;K<4;++K){auto V=D.CreateVertex();A.GetVertexPositions()[V]=P[K];auto I=D.CreateVertexInstance(V);A.GetVertexInstanceUVs().Set(I,0,UV[K]);A.GetVertexInstanceNormals()[I]=N;A.GetVertexInstanceTangents()[I]=(P3-P0).GetSafeNormal();A.GetVertexInstanceBinormalSigns()[I]=1;A.GetVertexInstanceColors()[I]=FVector4f(1,1,1,1);IDs.Add(I);}
        D.CreatePolygon(Group,IDs);
    }
    UStaticMesh* Finish(const FString& Name,UMaterial* M)
    {
        UStaticMesh* S=Asset<UStaticMesh>(Name);S->GetStaticMaterials().Add(FStaticMaterial(M,FName(TEXT("Flame"))));
        UStaticMesh::FBuildMeshDescriptionsParams P;P.bBuildSimpleCollision=false;P.bFastBuild=false;P.bAllowCpuAccess=true;
        S->BuildFromMeshDescriptions({&D},P);Save(S);return S;
    }
};
UStaticMesh* Core(int Type,UMaterial* M)
{
    Geometry G;int N=18, R=18;
    auto Point=[&](int I,int J){float U=float(I)/N, A=2*PI*J/R;float Radius=FMath::Pow(FMath::Max(0.001f,FMath::Sin(PI*U)),0.8f)*(Type==1?19.f:12.f);return FVector3f(22-64*U,Radius*FMath::Cos(A),Radius*FMath::Sin(A));};
    for(int I=0;I<N;++I)for(int J=0;J<R;++J)G.Quad(Point(I,J),Point(I,J+1),Point(I+1,J+1),Point(I+1,J),float(I)/N,float(I+1)/N);
    return G.Finish(FString::Printf(TEXT("SM_Flame_%d_Core"),Type+1),M);
}
UStaticMesh* Tail(int Type,UMaterial* M)
{
    Geometry G;int Fins=Type==0?3:Type==1?4:3;int Segs=48;
    for(int F=0;F<Fins;++F)
    {
        float Phase=F*2*PI/Fins;
        auto P=[&](int I,float Side){float U=float(I)/Segs;float Len=Type==0?260.f:Type==1?210.f:235.f;float Angle=Phase+(Type==2?U*9:0);float Radius=Type==2?(10+24*FMath::Sin(U*PI)):(5+8*U);float W=(Type==0?16.f:Type==1?35.f:18.f)*(1-0.65f*U);float Bend=Type==1?20*FMath::Sin(U*4+Phase)*U:0;return FVector3f(-18-Len*U,(Radius+Side*W)*FMath::Cos(Angle)+Bend,(Radius+Side*W)*FMath::Sin(Angle));};
        for(int I=0;I<Segs;++I)G.Quad(P(I,-1),P(I,1),P(I+1,1),P(I+1,-1),float(I)/Segs,float(I+1)/Segs);
    }
    return G.Finish(FString::Printf(TEXT("SM_Flame_%d_Tail"),Type+1),M);
}
UNiagaraStatelessEmitter* Emitter(UNiagaraSystem* S,const FString& Name,UStaticMesh* Mesh,UMaterial* Mat,bool Sparks,int Type)
{
    auto* E=NewObject<UNiagaraStatelessEmitter>(S,*Name,RF_Transactional);
    E->SetEmitterTemplate(NewObject<UNiagaraStatelessEmitterDefault>(E));
    E->SetUniqueEmitterName(Name);
    for(UNiagaraStatelessModule* Module:E->GetModules())if(Module->CanDisableModule())Module->SetIsModuleEnabled(false);
    auto* Init=CastChecked<UNiagaraStatelessModule_InitializeParticle>(E->GetModule(UNiagaraStatelessModule_InitializeParticle::StaticClass()));
    Init->LifetimeDistribution=FNiagaraDistributionRangeFloat(Sparks?0.35f:1.f,Sparks?0.9f:1.f);
    Init->MeshScaleDistribution.InitConstant(FVector3f(1));
    Init->SpriteSizeDistribution.InitConstant(FVector2f(3,3));
    Init->ColorDistribution.InitConstant(FLinearColor::White);
    auto& Spawn=E->AddSpawnInfo();
    Spawn.Type=Sparks?ENiagaraStatelessSpawnInfoType::Rate:ENiagaraStatelessSpawnInfoType::Burst;
    Spawn.Rate=FNiagaraDistributionRangeFloat(65);Spawn.Amount=FNiagaraDistributionRangeInt(1);
    if(Sparks)
    {
        auto* V=CastChecked<UNiagaraStatelessModule_AddVelocity>(E->GetModule(UNiagaraStatelessModule_AddVelocity::StaticClass()));V->SetIsModuleEnabled(true);
        V->VelocityType=ENSM_VelocityType::Linear;
        V->LinearVelocityDistribution.Mode=ENiagaraDistributionMode::NonUniformRange;
        V->LinearVelocityDistribution.Min=FVector3f(-350,-35,-35);V->LinearVelocityDistribution.Max=FVector3f(-160,35,35);
        auto* Renderer=NewObject<UNiagaraSpriteRendererProperties>(E);Renderer->Material=Mat;E->AddRenderer(Renderer,FGuid());
    }
    else
    {
        auto* Renderer=NewObject<UNiagaraMeshRendererProperties>(E);FNiagaraMeshRendererMeshProperties Entry;Entry.Mesh=Mesh;Renderer->Meshes.Empty();Renderer->Meshes.Add(Entry);E->AddRenderer(Renderer,FGuid());
        if(Type==2 && Name.Contains(TEXT("Tail")))
        {
            auto* R=CastChecked<UNiagaraStatelessModule_MeshRotationRate>(E->GetModule(UNiagaraStatelessModule_MeshRotationRate::StaticClass()));R->SetIsModuleEnabled(true);R->RotationRateDistribution.InitConstant(FVector3f(120,0,0));
        }
    }
    FStructProperty* Bounds=FindFProperty<FStructProperty>(E->GetClass(),TEXT("FixedBounds"));check(Bounds);
    *Bounds->ContainerPtrToValuePtr<FBox>(E)=FBox(FVector(-450,-150,-150),FVector(100,150,150));
    E->PostEditChange();
    FNiagaraEmitterHandle H(*E);H.SetName(*Name,*S);S->AddEmitterHandleDirect(H);return E;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlameCandidatesGenerate,"TunaSweeper.OneShot.GenerateFlameCandidates",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlameCandidatesGenerate::RunTest(const FString& Parameters)
{
    using namespace FlameCandidates;
    UTexture2D* Tex=Atlas();
    UMaterial* CoreMat=Material(TEXT("M_Flame_Core"),0,0,nullptr);
    UMaterial* SparkMat=Material(TEXT("M_Flame_Ember"),0,2,nullptr);
    const TCHAR* Names[]={TEXT("NS_FlameProjectile_ScrollLance"),TEXT("NS_FlameProjectile_FlipbookComet"),TEXT("NS_FlameProjectile_Helix")};
    for(int Type=0;Type<3;++Type)
    {
        UMaterial* TailMat=Material(FString::Printf(TEXT("M_Flame_%d_Tail"),Type+1),Type,1,Type==1?Tex:nullptr);
        UStaticMesh* CM=Core(Type,CoreMat);UStaticMesh* TM=Tail(Type,TailMat);
        UNiagaraSystem* S=Asset<UNiagaraSystem>(Names[Type]);UNiagaraSystemFactoryNew::InitializeSystem(S,true);
        Emitter(S,TEXT("ProjectileCore"),CM,CoreMat,false,Type);
        Emitter(S,TEXT("FlameTail"),TM,TailMat,false,Type);
        Emitter(S,TEXT("TrailingEmbers"),nullptr,SparkMat,true,Type);
        S->RequestCompile(false);S->WaitForCompilationComplete();S->PostEditChange();
        TestTrue(Names[Type],Save(S));TestEqual(TEXT("Three independent layers"),S->GetEmitterHandles().Num(),3);
    }
    return true;
}
#endif
