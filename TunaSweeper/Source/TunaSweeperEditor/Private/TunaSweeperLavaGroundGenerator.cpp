#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Stateless/Modules/NiagaraStatelessModule_DynamicMaterialParameters.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraSystemEmitterState.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "Stateless/NiagaraStatelessEmitter.h"
#include "Stateless/Modules/NiagaraStatelessModule_InitializeParticle.h"
#include "Stateless/Modules/NiagaraStatelessModule_AddVelocity.h"
#include "Stateless/Modules/NiagaraStatelessModule_GravityForce.h"
#include "Stateless/Modules/NiagaraStatelessModule_MeshRotationRate.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleMeshSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_ScaleSpriteSize.h"
#include "Stateless/Modules/NiagaraStatelessModule_ShapeLocation.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace LavaGround
{
const FString Root=TEXT("/Game/Effects/LavaGround/");
template<class T> T* Asset(const FString& Name)
{
    UPackage* P=CreatePackage(*(Root+Name));
    T* A=NewObject<T>(P,*Name,RF_Public|RF_Standalone|RF_Transactional);
    FAssetRegistryModule::AssetCreated(A);return A;
}
bool Save(UObject* A)
{
    A->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(A->GetPackage(),A,*FPackageName::LongPackageNameToFilename(A->GetPackage()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
template<class T> T* Node(UMaterial* M){auto* N=NewObject<T>(M);M->GetExpressionCollection().AddExpression(N);return N;}
UMaterialExpressionConstant* Scalar(UMaterial* M,float V){auto* N=Node<UMaterialExpressionConstant>(M);N->R=V;return N;}
UMaterialExpressionCustom* Custom(UMaterial* M,const FString& Code,ECustomMaterialOutputType Type)
{
    auto* C=Node<UMaterialExpressionCustom>(M);C->Code=Code;C->OutputType=Type;C->Inputs.Empty();
    auto Add=[&](FName Name,UMaterialExpression* E){FCustomInput I;I.InputName=Name;I.Input.Expression=E;C->Inputs.Add(I);};
    Add(TEXT("UV"),Node<UMaterialExpressionTextureCoordinate>(M));Add(TEXT("T"),Node<UMaterialExpressionTime>(M));Add(TEXT("Age"),Node<UMaterialExpressionDynamicParameter>(M));Add(TEXT("VC"),Node<UMaterialExpressionVertexColor>(M));return C;
}
UMaterialExpressionComponentMask* Mask(UMaterial* M,UMaterialExpression* E,bool Alpha)
{
    auto* N=Node<UMaterialExpressionComponentMask>(M);N->Input.Expression=E;N->R=N->G=N->B=!Alpha;N->A=Alpha;return N;
}
enum class ELook { Lava, Rock, Dust, Ember, Flame, Ring, Heat, Shock };
UMaterial* Material(const FString& Name,ELook Look)
{
    UMaterial* M=Asset<UMaterial>(Name);M->TwoSided=true;
    bool Recompile=false;M->SetMaterialUsage(Recompile,MATUSAGE_NiagaraMeshParticles);M->SetMaterialUsage(Recompile,MATUSAGE_NiagaraSprites);
    M->SetShadingModel(Look==ELook::Rock?MSM_DefaultLit:MSM_Unlit);
    bool Opaque=Look==ELook::Rock||Look==ELook::Lava;
    bool Distort=Look==ELook::Heat||Look==ELook::Shock;
    M->BlendMode=Opaque?BLEND_Masked:(Look==ELook::Dust||Distort?BLEND_Translucent:BLEND_Additive);
    auto* D=M->GetEditorOnlyData();
    FString Code;
    if(Look==ELook::Lava)
        Code=TEXT("float2 p=(UV-.5)*2;float r=length(p);float angle=atan2(p.y,p.x);float edge=.94+.035*sin(angle*9)+.025*sin(angle*17);float reach=saturate(Age*35);float cool=smoothstep(.68,1,Age);float band=sin(p.x*28+p.y*17+sin(p.y*13-T*1.8)*2+T*2.6);float veins=smoothstep(.35,.8,band);float center=exp(-r*r*35);float3 hot=lerp(float3(1,.035,.001),float3(1,.47,.015),veins);hot=lerp(hot,float3(1,.8,.15),center*.8);float living=saturate(1-cool*(.5+r));float3 c=lerp(float3(.025,.005,.003),hot*3,living);float alpha=step(r,edge*reach)*step(r,cool>.9?1-(cool-.9)*10:1);return float4(c,alpha);");
    else if(Look==ELook::Rock)
        Code=TEXT("float n=.5+.5*sin(UV.x*43+sin(UV.y*29)*2);float warm=pow(saturate(1-VC.r),3);float appear=step(length((UV-.5)*2),saturate(Age*32));float gone=1-smoothstep(.91,1,Age);float3 c=lerp(float3(.014,.018,.025),float3(.032,.038,.046),n);return float4(c*(.7+VC.r*.5),appear*step(.05,gone));");
    else if(Look==ELook::Dust)
        Code=TEXT("float2 p=UV*2-1;float n=.7+.18*sin(p.x*11+sin(p.y*9))+ .12*sin(p.y*17+p.x*6);float a=saturate((1-dot(p,p))*2)*n*sin(saturate(Age)*3.14159);return float4(float3(.17,.125,.095)*(n*.5+.7),a*.36);");
    else if(Look==ELook::Ember)
        Code=TEXT("float2 p=UV*2-1;float a=saturate((1-dot(p,p))*5)*pow(saturate(1-Age),1.4);return float4(float3(1,.19,.005)*6,a);");
    else if(Look==ELook::Flame)
        Code=TEXT("float v=UV.y;float center=.5+.13*sin(v*8-T*7)*(1-v);float w=.025+.32*v;float a=saturate((w-abs(UV.x-center))*24)*smoothstep(0,.12,v)*smoothstep(0,.16,1-v);a*=smoothstep(0,.1,Age)*(1-smoothstep(.75,1,Age));float3 c=lerp(float3(1,.045,.001),float3(1,.64,.06),v);return float4(c*3,a);");
    else if(Look==ELook::Ring)
        Code=TEXT("float r=length(UV*2-1);float a=saturate(1-abs(r-.83)*22)*(1-Age);return float4(float3(1,.24,.008)*5,a);");
    if(Distort)
    {
        M->RefractionMethod=RM_2DOffset;M->RefractionDepthBias=0;
        Code=Look==ELook::Heat
            ? TEXT("float2 p=UV*2-1;float a=saturate(1-dot(p,p))*smoothstep(0,.06,Age)*(1-smoothstep(.75,1,Age));return float2(sin(p.y*28-T*5+sin(p.x*17)),cos(p.x*25+T*4))*a*.003;")
            : TEXT("float2 p=UV*2-1;float r=length(p);float a=saturate(1-abs(r-.8)*12)*(1-Age);return normalize(p+.001)*a*.022;");
        D->Refraction.Expression=Custom(M,Code,CMOT_Float2);D->Opacity.Expression=Scalar(M,0.015f);D->EmissiveColor.Expression=Scalar(M,0);
    }
    else
    {
        auto* C=Custom(M,Code,CMOT_Float4);
        if(Look==ELook::Rock)
        {
            D->BaseColor.Expression=Mask(M,C,false);D->Roughness.Expression=Scalar(M,.88f);
            D->EmissiveColor.Expression=Custom(M,TEXT("float glow=pow(saturate(1-VC.r),4)*(1-smoothstep(.65,1,Age));return float3(1,.06,.001)*glow*1.2;"),CMOT_Float3);
            D->WorldPositionOffset.Expression=Custom(M,TEXT("float settle=smoothstep(0,.045,Age);float sink=smoothstep(.87,1,Age);return float3(0,0,-30*(1-settle)-35*sink);"),CMOT_Float3);
        }
        else D->EmissiveColor.Expression=Mask(M,C,false);
        if(Opaque)D->OpacityMask.Expression=Mask(M,C,true);else D->Opacity.Expression=Mask(M,C,true);
    }
    M->PostEditChange();Save(M);return M;
}
struct Geometry
{
    FMeshDescription D;FStaticMeshAttributes A;FPolygonGroupID Group;
    Geometry():A(D){A.Register();A.GetVertexInstanceUVs().SetNumChannels(1);Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Surface");}
    void Face(const TArray<FVector3f>& Points,float Shade=1,bool PlanarUV=true)
    {
        if(Points.Num()<3)return;TArray<FVertexInstanceID> IDs;
        FVector3f N=FVector3f::CrossProduct(Points[1]-Points[0],Points[2]-Points[0]).GetSafeNormal();
        for(int I=0;I<Points.Num();++I){auto V=D.CreateVertex();A.GetVertexPositions()[V]=Points[I];auto VI=D.CreateVertexInstance(V);A.GetVertexInstanceNormals()[VI]=N;A.GetVertexInstanceTangents()[VI]=(Points[1]-Points[0]).GetSafeNormal();A.GetVertexInstanceBinormalSigns()[VI]=1;A.GetVertexInstanceUVs().Set(VI,0,PlanarUV?FVector2f(Points[I].X/480+.5f,Points[I].Y/480+.5f):FVector2f(I==1||I==2?1:0,I>=2?1:0));A.GetVertexInstanceColors()[VI]=FVector4f(Shade,Shade,Shade,1);IDs.Add(VI);}
        D.CreatePolygon(Group,IDs);
    }
    UStaticMesh* Finish(const FString& Name,UMaterial* M)
    {
        UStaticMesh* S=Asset<UStaticMesh>(Name);S->GetStaticMaterials().Add(FStaticMaterial(M,TEXT("Surface")));
        UStaticMesh::FBuildMeshDescriptionsParams P;P.bFastBuild=false;P.bBuildSimpleCollision=false;P.bAllowCpuAccess=true;S->BuildFromMeshDescriptions({&D},P);Save(S);return S;
    }
};
UStaticMesh* Disk(const FString& Name,UMaterial* M,float Height)
{
    Geometry G;for(int I=0;I<96;++I){float A=I*2*PI/96,B=(I+1)*2*PI/96;G.Face({FVector3f(0,0,Height),FVector3f(240*FMath::Cos(A),240*FMath::Sin(A),Height),FVector3f(240*FMath::Cos(B),240*FMath::Sin(B),Height)});}return G.Finish(Name,M);
}
TArray<FVector2f> Clip(const TArray<FVector2f>& Poly,FVector2f N,float Dist)
{
    TArray<FVector2f> Out;
    for(int I=0;I<Poly.Num();++I){auto A=Poly[I],B=Poly[(I+1)%Poly.Num()];float DA=FVector2f::DotProduct(A,N)-Dist,DB=FVector2f::DotProduct(B,N)-Dist;if(DA<=0)Out.Add(A);if((DA<=0)!=(DB<=0))Out.Add(A+(B-A)*(DA/(DA-DB)));}return Out;
}
void Plate(Geometry& G,TArray<FVector2f> Poly,float Z,float Scale=0.85f)
{
    FVector2f Center(0);for(auto P:Poly)Center+=P;Center/=Poly.Num();
    for(auto& P:Poly)P=Center+(P-Center)*Scale;
    TArray<FVector3f> Top;
    for(auto P:Poly){auto Q=Center+(P-Center)*.84f;Top.Add(FVector3f(Q.X,Q.Y,Z));}
    G.Face(Top,1);
    for(int I=0;I<Poly.Num();++I){int J=(I+1)%Poly.Num();FVector3f A(Poly[I].X,Poly[I].Y,Z-5),B(Poly[J].X,Poly[J].Y,Z-5);G.Face({A,B,Top[J],Top[I]},.65f);G.Face({FVector3f(A.X,A.Y,1),FVector3f(B.X,B.Y,1),B,A},.15f);}
}
UStaticMesh* Crust(UMaterial* M)
{
    Geometry G;FRandomStream R(5921);TArray<FVector2f> Seeds{FVector2f(0)};
    for(int Ring=0;Ring<2;++Ring){int Count=Ring==0?7:11;for(int I=0;I<Count;++I){float A=I*2*PI/Count+Ring*.22f;float Radius=(Ring==0?92.f:188.f)+R.FRandRange(-15,15);Seeds.Add(FVector2f(FMath::Cos(A),FMath::Sin(A))*Radius);}}
    TArray<FVector2f> Boundary;for(int I=0;I<48;++I){float A=I*2*PI/48;float Radius=230+9*FMath::Sin(A*9)+5*FMath::Sin(A*17);Boundary.Add(FVector2f(FMath::Cos(A),FMath::Sin(A))*Radius);}
    for(int I=1;I<Seeds.Num();++I){auto Poly=Boundary;for(int J=0;J<Seeds.Num();++J)if(I!=J){auto N=Seeds[J]-Seeds[I];float D=(Seeds[J].SizeSquared()-Seeds[I].SizeSquared())*.5f;Poly=Clip(Poly,N,D);}if(Poly.Num()>2)Plate(G,Poly,R.FRandRange(14,27));}
    return G.Finish(TEXT("SM_Lava_FracturedCrust"),M);
}
UStaticMesh* Fragment(UMaterial* M)
{
    Geometry G;G.Face({{-8,-5,0},{6,-7,1},{9,4,0},{-5,8,2}},.25f);G.Face({{-8,-5,0},{-4,0,13},{6,-7,1}},.85f);G.Face({{6,-7,1},{-4,0,13},{9,4,0}},1);G.Face({{9,4,0},{-4,0,13},{-5,8,2}},.65f);G.Face({{-5,8,2},{-4,0,13},{-8,-5,0}},.8f);return G.Finish(TEXT("SM_Lava_RockFragment"),M);
}
template<class T> T* Module(UNiagaraStatelessEmitter* E){auto* M=CastChecked<T>(E->GetModule(T::StaticClass()));M->SetIsModuleEnabled(true);return M;}
UNiagaraStatelessEmitter* Emitter(UNiagaraSystem* S,const FString& Name,UStaticMesh* Mesh,UMaterial* Mat,float Start,float Life,int Count,bool Loop)
{
    auto* E=NewObject<UNiagaraStatelessEmitter>(S,*Name,RF_Transactional);E->SetEmitterTemplate(NewObject<UNiagaraStatelessEmitterDefault>(E));E->SetUniqueEmitterName(Name);
    for(UNiagaraStatelessModule* M:E->GetModules())if(M->CanDisableModule())M->SetIsModuleEnabled(false);
    auto* Init=Module<UNiagaraStatelessModule_InitializeParticle>(E);Init->LifetimeDistribution.InitConstant(Life);Init->MeshScaleDistribution.InitConstant(FVector3f(1));Init->SpriteSizeDistribution.InitConstant(FVector2f(8,8));Init->ColorDistribution.InitConstant(FLinearColor::White);
    auto& Spawn=E->AddSpawnInfo();Spawn.Type=ENiagaraStatelessSpawnInfoType::Burst;Spawn.SpawnTime=Start;Spawn.Amount=FNiagaraDistributionRangeInt(Count);
    auto* ES=FindFProperty<FStructProperty>(E->GetClass(),TEXT("EmitterState"))->ContainerPtrToValuePtr<FNiagaraEmitterStateData>(E);ES->LoopBehavior=Loop?ENiagaraLoopBehavior::Infinite:ENiagaraLoopBehavior::Once;ES->LoopDuration.InitConstant(12);ES->LoopDurationMode=ENiagaraLoopDurationMode::Fixed;
    *FindFProperty<FStructProperty>(E->GetClass(),TEXT("FixedBounds"))->ContainerPtrToValuePtr<FBox>(E)=FBox(FVector(-550,-550,-100),FVector(550,550,750));
    if(Mesh){auto* RP=NewObject<UNiagaraMeshRendererProperties>(E);RP->Meshes.Empty();auto& Entry=RP->Meshes.AddDefaulted_GetRef();Entry.Mesh=Mesh;E->AddRenderer(RP,FGuid());}
    else{auto* RP=NewObject<UNiagaraSpriteRendererProperties>(E);RP->Material=Mat;E->AddRenderer(RP,FGuid());}
    return E;
}
void Attach(UNiagaraSystem* S,UNiagaraStatelessEmitter* E){Module<UNiagaraStatelessModule_DynamicMaterialParameters>(E)->Parameter0.XChannelDistribution.InitCurve({0.f,1.f});E->PostEditChange();FNiagaraEmitterHandle H(*E);H.SetName(*E->GetName(),*S);S->AddEmitterHandleDirect(H);}
void Position(UNiagaraStatelessEmitter* E,FVector3f P){Module<UNiagaraStatelessModule_InitializeParticle>(E)->InitialPositionDistribution.InitConstant(P);}
void LinearVelocity(UNiagaraStatelessEmitter* E,FVector3f V){auto* M=Module<UNiagaraStatelessModule_AddVelocity>(E);M->VelocityType=ENSM_VelocityType::Linear;M->LinearVelocityDistribution.InitConstant(V);}
void RingLocation(UNiagaraStatelessEmitter* E,float Radius){auto* Shape=Module<UNiagaraStatelessModule_ShapeLocation>(E);Shape->ShapePrimitive=ENSM_ShapePrimitive::Ring;Shape->RingRadius.InitConstant(Radius);Shape->DiscCoverage.InitConstant(1);}
void Launch(UNiagaraStatelessEmitter* E,float Speed,float Gravity)
{
    auto* V=Module<UNiagaraStatelessModule_AddVelocity>(E);V->VelocityType=ENSM_VelocityType::InCone;V->ConeRotation=FRotator::ZeroRotator;V->ConeAngle=68;V->ConeVelocityDistribution=FNiagaraDistributionRangeFloat(Speed*.65f,Speed);
    Module<UNiagaraStatelessModule_GravityForce>(E)->GravityDistribution.InitConstant(FVector3f(0,0,-Gravity));
}
void MoreBursts(UNiagaraStatelessEmitter* E,float Start,float End,float Step,int Count){for(float T=Start;T<End;T+=Step){auto& B=E->AddSpawnInfo();B.SpawnTime=T;B.Amount=FNiagaraDistributionRangeInt(Count);}}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLavaGroundGenerate,"TunaSweeper.OneShot.GenerateLavaGround",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLavaGroundGenerate::RunTest(const FString& Parameters)
{
    using namespace LavaGround;
    UMaterial* Lava=Material(TEXT("M_Lava_FlowingGround"),ELook::Lava),*Rock=Material(TEXT("M_Lava_CharredCrust"),ELook::Rock),*Dust=Material(TEXT("M_Lava_Dust"),ELook::Dust),*Ember=Material(TEXT("M_Lava_Ember"),ELook::Ember),*Flame=Material(TEXT("M_Lava_LowFlame"),ELook::Flame),*Ring=Material(TEXT("M_Lava_ImpactRing"),ELook::Ring),*Heat=Material(TEXT("M_Lava_HeatRefraction"),ELook::Heat),*Shock=Material(TEXT("M_Lava_ShockRefraction"),ELook::Shock);
    UStaticMesh* CrustMesh=Crust(Rock),*RockMesh=Fragment(Rock),*LavaMesh=Disk(TEXT("SM_Lava_Surface"),Lava,3),*RingMesh=Disk(TEXT("SM_Lava_ImpactRing"),Ring,8),*HeatMesh=Disk(TEXT("SM_Lava_HeatLayer"),Heat,36),*ShockMesh=Disk(TEXT("SM_Lava_ShockLayer"),Shock,28);
    for(bool Ground:{false,true})for(bool Loop:{false,true})
    {
        FString Name=Ground?TEXT("NS_LavaGround"):TEXT("NS_LavaImpactBurst");if(Loop)Name+=TEXT("_PreviewLoop");
        auto* S=Asset<UNiagaraSystem>(Name);UNiagaraSystemFactoryNew::InitializeSystem(S,true);
        if(Ground)
        {
            Attach(S,Emitter(S,TEXT("MoltenGround"),LavaMesh,Lava,0,10.3f,1,Loop));
            Attach(S,Emitter(S,TEXT("BrokenCrustPlates"),CrustMesh,Rock,0,10.3f,1,Loop));
            Attach(S,Emitter(S,TEXT("PersistentHeatDistortion"),HeatMesh,Heat,.2f,9.6f,1,Loop));
            auto* E=Emitter(S,TEXT("MoltenDropletsAndEmbers"),nullptr,Ember,.3f,1.4f,5,Loop);Position(E,FVector3f(0,0,20));RingLocation(E,180);Launch(E,180,350);Module<UNiagaraStatelessModule_InitializeParticle>(E)->SpriteSizeDistribution.InitConstant(FVector2f(4,4));MoreBursts(E,.65f,8.1f,.35f,5);Attach(S,E);
            E=Emitter(S,TEXT("LowFlamesInFissures"),nullptr,Flame,.4f,7.5f,9,Loop);Position(E,FVector3f(0,0,35));RingLocation(E,195);Module<UNiagaraStatelessModule_InitializeParticle>(E)->SpriteSizeDistribution.InitConstant(FVector2f(40,75));Attach(S,E);
        }
        else
        {
            auto* E=Emitter(S,TEXT("RockDebrisBurst"),RockMesh,Rock,0,1.15f,34,Loop);Position(E,FVector3f(0,0,35));RingLocation(E,85);Launch(E,430,680);Module<UNiagaraStatelessModule_MeshRotationRate>(E)->RotationRateDistribution.InitConstant(FVector3f(190,270,145));Attach(S,E);
            E=Emitter(S,TEXT("GroundDustBurst"),nullptr,Dust,0,1.6f,22,Loop);Position(E,FVector3f(0,0,22));RingLocation(E,105);Launch(E,190,80);Module<UNiagaraStatelessModule_InitializeParticle>(E)->SpriteSizeDistribution.InitConstant(FVector2f(105,75));Attach(S,E);
            E=Emitter(S,TEXT("ExpandingFlameShock"),RingMesh,Ring,0,.65f,1,Loop);Module<UNiagaraStatelessModule_ScaleMeshSize>(E)->ScaleDistribution.InitCurve({.04f,.65f,1.15f,1.5f});Attach(S,E);
            E=Emitter(S,TEXT("ImpactSpaceDistortion"),ShockMesh,Shock,0,.85f,1,Loop);Module<UNiagaraStatelessModule_ScaleMeshSize>(E)->ScaleDistribution.InitCurve({.06f,.65f,1.25f,1.8f});Attach(S,E);
            E=Emitter(S,TEXT("ImpactMoltenDroplets"),nullptr,Ember,0,1.4f,48,Loop);Position(E,FVector3f(0,0,20));RingLocation(E,85);Launch(E,260,350);Module<UNiagaraStatelessModule_InitializeParticle>(E)->SpriteSizeDistribution.InitConstant(FVector2f(5,5));Attach(S,E);
        }
        S->RequestCompile(false);S->WaitForCompilationComplete();S->PostEditChange();TestTrue(TEXT("Save independent effect system"),Save(S));TestEqual(TEXT("Five effect layers per independent system"),S->GetEmitterHandles().Num(),5);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLavaGroundAgeFix,"TunaSweeper.OneShot.FixLavaAge",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLavaGroundAgeFix::RunTest(const FString& Parameters)
{
    using namespace LavaGround;
    for(const TCHAR* Name:{TEXT("M_Lava_FlowingGround"),TEXT("M_Lava_CharredCrust"),TEXT("M_Lava_Dust"),TEXT("M_Lava_Ember"),TEXT("M_Lava_LowFlame"),TEXT("M_Lava_ImpactRing"),TEXT("M_Lava_HeatRefraction"),TEXT("M_Lava_ShockRefraction")})
    {
        auto* M=LoadObject<UMaterial>(nullptr,*(Root+Name));auto Expressions=M->GetExpressions();
        for(auto Expression:Expressions)if(auto* C=Cast<UMaterialExpressionCustom>(Expression))for(auto& Input:C->Inputs)if(Input.InputName==TEXT("Age"))Input.Input.Expression=Node<UMaterialExpressionDynamicParameter>(M);
        M->PostEditChange();TestTrue(TEXT("Save age material"),Save(M));
    }
    for(const TCHAR* Name:{TEXT("NS_LavaImpactBurst"),TEXT("NS_LavaImpactBurst_PreviewLoop"),TEXT("NS_LavaGround"),TEXT("NS_LavaGround_PreviewLoop")})
    {
        auto* S=LoadObject<UNiagaraSystem>(nullptr,*(Root+Name));
        for(auto& Handle:S->GetEmitterHandles())if(auto* E=Handle.GetStatelessEmitter())
        {
            Module<UNiagaraStatelessModule_DynamicMaterialParameters>(E)->Parameter0.XChannelDistribution.InitCurve({0.f,1.f});
            if(auto* V=Cast<UNiagaraStatelessModule_AddVelocity>(E->GetModule(UNiagaraStatelessModule_AddVelocity::StaticClass())))V->ConeRotation=FRotator::ZeroRotator;
            E->PostEditChange();
        }
        S->RequestCompile(false);S->WaitForCompilationComplete();S->PostEditChange();TestTrue(TEXT("Save age system"),Save(S));
    }
    return true;
}
#endif
