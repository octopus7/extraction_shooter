#include "GenerateMaskWaterAssetsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "StylizedWaterBodyActor.h"
#include "Engine/World.h"

namespace WaterOneShot
{
    bool Save(UObject* Asset)
    {
        FAssetRegistryModule::AssetCreated(Asset);
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
        const FString File=FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
        return UPackage::SavePackage(Asset->GetOutermost(),Asset,*File,Args);
    }
    template<class T> T* Asset(const FString& Path)
    {
        UPackage* Package=CreatePackage(*Path);
        return NewObject<T>(Package,*FPackageName::GetShortName(Path),RF_Public|RF_Standalone);
    }
    template<class T> T* Node(UMaterial* M)
    {
        T* N=NewObject<T>(M); N->Material=M; M->GetExpressionCollection().AddExpression(N); return N;
    }
    UMaterialExpression* V(UMaterial* M,const TCHAR* Name,FLinearColor Value)
    {
        auto* P=Node<UMaterialExpressionVectorParameter>(M); P->ParameterName=Name; P->DefaultValue=Value;
        auto* A=Node<UMaterialExpressionAppendVector>(M); A->A.Connect(0,P); A->B.Connect(4,P); return A;
    }
    UMaterialExpression* S(UMaterial* M,const TCHAR* Name,float Value)
    {
        auto* P=Node<UMaterialExpressionScalarParameter>(M); P->ParameterName=Name; P->DefaultValue=Value; return P;
    }
    void Input(UMaterialExpressionCustom* C,const TCHAR* Name,UMaterialExpression* N,int32 Index=0)
    {
        FCustomInput I; I.InputName=Name; I.Input.Connect(Index,N); C->Inputs.Add(I);
    }
    UMaterialExpressionCustom* Custom(UMaterial* M,const TCHAR* Code,ECustomMaterialOutputType Type,const TCHAR* Include=nullptr)
    {
        auto* C=Node<UMaterialExpressionCustom>(M); C->Inputs.Reset(); C->Code=Code; C->OutputType=Type;
        if(Include) C->IncludeFilePaths.Add(Include);
        return C;
    }
    UTexture2D* Mask(const TCHAR* Name,int32 Preset)
    {
        auto* T=Asset<UTexture2D>(FString(TEXT("/StylizedWater/MaskWater/Masks/T_Mask"))+Name);
        constexpr int32 N=1024;
        TArray<FFloat16Color> Pixels; Pixels.SetNumUninitialized(N*N);
        for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
        {
            const double U=(X+0.5)/N,Vv=(Y+0.5)/N;
            double D=0,Depth=0;
            if(Preset==0)
            {
                const double PX=(U-.5)*5000,PY=(Vv-.5)*5000;
                const double Angle=FMath::Atan2(PY,PX);
                D=1750+95*FMath::Sin(3*Angle)+65*FMath::Sin(5*Angle+1)-FMath::Sqrt(PX*PX+PY*PY);
                Depth=FMath::Clamp(D/1150,0.,1.);
            }
            else if(Preset==1)
            {
                const double PX=(U-.5)*6000,PY=(Vv-.5)*5000;
                D=PY+650+130*FMath::Sin(PX/850)+55*FMath::Sin(PX/340);
                Depth=FMath::Clamp(D/2400,0.,1.);
            }
            else
            {
                const double PX=(U-.5)*7000,PY=(Vv-.5)*2400;
                const double Center=210*FMath::Sin(PX/1150)+65*FMath::Sin(PX/420);
                D=650+55*FMath::Sin(PX/690)-FMath::Abs(PY-Center);
                Depth=FMath::Clamp(D/650,0.,1.);
            }
            Pixels[Y*N+X]=FFloat16Color(FLinearColor(FMath::Clamp(.5+D/2000,0.,1.),Depth,0,1));
        }
        T->Source.Init(N,N,1,1,TSF_RGBA16F,reinterpret_cast<const uint8*>(Pixels.GetData()));
        T->SRGB=false; T->CompressionSettings=TC_HDR; T->MipGenSettings=TMGS_SimpleAverage;
        T->Filter=TF_Trilinear; T->AddressX=TA_Clamp; T->AddressY=TA_Clamp;
        T->PostEditChange(); check(Save(T)); return T;
    }
    UTexture2D* Sky()
    {
        FImage Image;
        const FString Path=IPluginManager::Get().FindPlugin(TEXT("StylizedWater"))->GetBaseDir()/TEXT("Resources/SkyParallax/AnimeSky.png");
        check(FImageUtils::LoadImage(*Path,Image));
        FImage BGRA; Image.CopyTo(BGRA,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
        auto* T=Asset<UTexture2D>(TEXT("/StylizedWater/SkyParallax/T_AnimeSky"));
        T->Source.Init(BGRA.SizeX,BGRA.SizeY,1,1,TSF_BGRA8,BGRA.RawData.GetData());
        T->SRGB=true; T->CompressionSettings=TC_Default; T->Filter=TF_Trilinear;
        T->AddressX=TA_Mirror; T->AddressY=TA_Mirror;
        T->PostEditChange(); check(Save(T)); return T;
    }
    UMaterial* Material(UTexture2D* MaskTexture,UTexture2D* SkyTexture)
    {
        auto* M=Asset<UMaterial>(SkyTexture?TEXT("/StylizedWater/SkyParallax/M_WaterMaskSky"):TEXT("/StylizedWater/MaskWater/M_WaterMask"));
        M->BlendMode=BLEND_Translucent; M->SetShadingModel(MSM_Unlit); M->TwoSided=true;
        M->bUsedWithStaticLighting=true;
        auto* Data=M->GetEditorOnlyData();
        auto* P=Node<UMaterialExpressionWorldPosition>(M); P->WorldPositionShaderOffset=WPT_ExcludeAllShaderOffsets;
        auto* Center=V(M,TEXT("MaskCenter"),FLinearColor(0,0,1,0));
        auto* Extent=V(M,TEXT("MaskExtent"),FLinearColor(5000,5000,1,1));
        auto* Offset=V(M,TEXT("MaskOffset"),FLinearColor(0,0,0,0));
        auto* UV=Custom(M,TEXT("return WaterMaskUV(P,Center,Extent,Offset.xy);"),CMOT_Float2,TEXT("/Plugin/StylizedWater/Private/MaskWater.ush"));
        Input(UV,TEXT("P"),P); Input(UV,TEXT("Center"),Center); Input(UV,TEXT("Extent"),Extent); Input(UV,TEXT("Offset"),Offset);
        auto* Sample=Node<UMaterialExpressionTextureSampleParameter2D>(M);
        Sample->ParameterName=TEXT("BoundaryMask"); Sample->Texture=MaskTexture; Sample->SamplerType=SAMPLERTYPE_LinearColor; Sample->Coordinates.Connect(0,UV);
        auto* T=Node<UMaterialExpressionTime>(M);
        auto* Vert=Node<UMaterialExpressionVertexColor>(M);
        auto* Water=Custom(M,TEXT("return EvaluateMaskWater(P,UV,float4(Mask.rgb,1),TerrainDepth,Time*AnimationSpeed,Boundary,Shore,Surface,Flow,RippleStrength,ShallowColor.rgb,MidColor.rgb,DeepColor.rgb,FoamColor.rgb);"),CMOT_Float4,TEXT("/Plugin/StylizedWater/Private/MaskWater.ush"));
        Input(Water,TEXT("P"),P); Input(Water,TEXT("UV"),UV); Input(Water,TEXT("Mask"),Sample); Input(Water,TEXT("TerrainDepth"),Vert,1); Input(Water,TEXT("Time"),T);
        Input(Water,TEXT("AnimationSpeed"),S(M,TEXT("AnimationSpeed"),1));
        Input(Water,TEXT("Boundary"),V(M,TEXT("Boundary"),FLinearColor(1000,35,0,14)));
        Input(Water,TEXT("Shore"),V(M,TEXT("Shore"),FLinearColor(.09,22,180,.6)));
        Input(Water,TEXT("Surface"),V(M,TEXT("Surface"),FLinearColor(.86,.14,0,1)));
        Input(Water,TEXT("Flow"),V(M,TEXT("Flow"),FLinearColor(1,0,8,230)));
        Input(Water,TEXT("RippleStrength"),S(M,TEXT("RippleStrength"),.12));
        Input(Water,TEXT("ShallowColor"),V(M,TEXT("ShallowColor"),FLinearColor(.13,.49,.48,1)));
        Input(Water,TEXT("MidColor"),V(M,TEXT("MidColor"),FLinearColor(.026,.23,.35,1)));
        Input(Water,TEXT("DeepColor"),V(M,TEXT("DeepColor"),FLinearColor(.012,.073,.18,1)));
        Input(Water,TEXT("FoamColor"),V(M,TEXT("FoamColor"),FLinearColor(.82,.9,.86,1)));
        auto* RGB=Node<UMaterialExpressionComponentMask>(M); RGB->R=RGB->G=RGB->B=true; RGB->A=false; RGB->Input.Connect(0,Water);
        auto* Alpha=Node<UMaterialExpressionComponentMask>(M); Alpha->R=Alpha->G=Alpha->B=false; Alpha->A=true; Alpha->Input.Connect(0,Water);
        auto* Fade=Node<UMaterialExpressionDepthFade>(M); Fade->InOpacity.Connect(0,Alpha); Fade->FadeDistance.Connect(0,S(M,TEXT("IntersectionFade"),4));
        Data->Opacity.Connect(0,Fade); Data->EmissiveColor.Connect(0,RGB);
        auto* Refraction=Custom(M,TEXT("return 1+Strength*Water.a*Water.a*(0.7+0.3*sin(P.x/180+P.y/130));"),CMOT_Float1);
        Input(Refraction,TEXT("Strength"),S(M,TEXT("RefractionStrength"),.015)); Input(Refraction,TEXT("Water"),Water); Input(Refraction,TEXT("P"),P);
        Data->Refraction.Connect(0,Refraction);
        if(SkyTexture)
        {
            auto* Camera=Node<UMaterialExpressionCameraPositionWS>(M);
            auto* SkySettings=V(M,TEXT("Sky"),FLinearColor(6500,28000,.68,0));
            auto* Anchor=V(M,TEXT("SkyAnchor"),FLinearColor(0,0,0,0));
            auto* SkyUV=Custom(M,TEXT("return PaintedSkyUV(P,Camera,Sky,Anchor.xy);"),CMOT_Float2,TEXT("/Plugin/StylizedWater/SkyParallax/PaintedSky.ush"));
            Input(SkyUV,TEXT("P"),P); Input(SkyUV,TEXT("Camera"),Camera); Input(SkyUV,TEXT("Sky"),SkySettings); Input(SkyUV,TEXT("Anchor"),Anchor);
            auto* Clouds=Node<UMaterialExpressionTextureSampleParameter2D>(M); Clouds->ParameterName=TEXT("SkyTexture"); Clouds->Texture=SkyTexture; Clouds->SamplerType=SAMPLERTYPE_Color; Clouds->Coordinates.Connect(0,SkyUV);
            auto* Blend=Custom(M,TEXT("return BlendPaintedSky(Water,Cloud.rgb,P,Camera,Sky.z);"),CMOT_Float3,TEXT("/Plugin/StylizedWater/SkyParallax/PaintedSky.ush"));
            Input(Blend,TEXT("Water"),Water); Input(Blend,TEXT("Cloud"),Clouds); Input(Blend,TEXT("P"),P); Input(Blend,TEXT("Camera"),Camera); Input(Blend,TEXT("Sky"),SkySettings);
            Data->EmissiveColor.Connect(0,Blend);
        }
        int32 I=0;
        for(UMaterialExpression* E:M->GetExpressions()) { E->MaterialExpressionEditorX=-1500+(I/10)*300; E->MaterialExpressionEditorY=(I%10)*180; ++I; }
        M->PostEditChange(); check(Save(M)); return M;
    }
}
UGenerateMaskWaterAssetsCommandlet::UGenerateMaskWaterAssetsCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
UTexture2D* UGenerateMaskWaterAssetsCommandlet::BakeActorMask(AStylizedWaterBodyActor* Body,const FString& PackagePath)
{
    if(!Body || !Body->GetWorld() || !PackagePath.StartsWith(TEXT("/StylizedWater/MaskWater/Masks/"))) return nullptr;
    constexpr int32 N=1024;
    TArray<float> Depth,Distance; Depth.SetNumUninitialized(N*N); Distance.Init(1.e8f,N*N);
    TArray<uint8> Wet; Wet.SetNumUninitialized(N*N);
    int32 Hits=0,WetCount=0;
    const FTransform Transform=Body->GetActorTransform();
    const float DX=Body->SurfaceSize.X*FMath::Abs(Transform.GetScale3D().X)/N;
    const float DY=Body->SurfaceSize.Y*FMath::Abs(Transform.GetScale3D().Y)/N;
    const float Diagonal=FMath::Sqrt(DX*DX+DY*DY);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(WaterOneShotMask),true,Body);
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        const int32 I=Y*N+X;
        FVector P=Transform.TransformPosition(FVector(((X+.5)/N-.5)*Body->SurfaceSize.X,((Y+.5)/N-.5)*Body->SurfaceSize.Y,Body->WaterLevelOffset));
        FHitResult Hit;
        const bool Found=Body->GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,Body->TraceHeight),P-FVector(0,0,Body->MaximumDepth),Body->TerrainTraceChannel,Query);
        if(Found) ++Hits;
        Depth[I]=Found?P.Z-Hit.ImpactPoint.Z:Body->MaximumDepth;
        Wet[I]=Depth[I]>0; WetCount+=Wet[I];
    }
    if(Hits==0 || WetCount==0) { UE_LOG(LogTemp,Error,TEXT("Mask bake needs terrain hits and wet samples: %s, hits=%d wet=%d"),*Body->GetName(),Hits,WetCount); return nullptr; }
    // Two-pass anisotropic chamfer distance; mask filtering removes texel stair steps.
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        const int32 I=Y*N+X;
        if((X>0 && Wet[I]!=Wet[I-1]) || (X+1<N && Wet[I]!=Wet[I+1])) Distance[I]=FMath::Min(Distance[I],DX*.5f);
        if((Y>0 && Wet[I]!=Wet[I-N]) || (Y+1<N && Wet[I]!=Wet[I+N])) Distance[I]=FMath::Min(Distance[I],DY*.5f);
    }
    for(int32 Y=0;Y<N;++Y) for(int32 X=0;X<N;++X)
    {
        int32 I=Y*N+X;
        if(X>0) Distance[I]=FMath::Min(Distance[I],Distance[I-1]+DX);
        if(Y>0) Distance[I]=FMath::Min(Distance[I],Distance[I-N]+DY);
        if(X>0 && Y>0) Distance[I]=FMath::Min(Distance[I],Distance[I-N-1]+Diagonal);
        if(X+1<N && Y>0) Distance[I]=FMath::Min(Distance[I],Distance[I-N+1]+Diagonal);
    }
    for(int32 Y=N-1;Y>=0;--Y) for(int32 X=N-1;X>=0;--X)
    {
        int32 I=Y*N+X;
        if(X+1<N) Distance[I]=FMath::Min(Distance[I],Distance[I+1]+DX);
        if(Y+1<N) Distance[I]=FMath::Min(Distance[I],Distance[I+N]+DY);
        if(X+1<N && Y+1<N) Distance[I]=FMath::Min(Distance[I],Distance[I+N+1]+Diagonal);
        if(X>0 && Y+1<N) Distance[I]=FMath::Min(Distance[I],Distance[I+N-1]+Diagonal);
    }
    TArray<FFloat16Color> Pixels; Pixels.SetNumUninitialized(N*N);
    for(int32 I=0;I<N*N;++I) Pixels[I]=FFloat16Color(FLinearColor(FMath::Clamp(.5f+(Wet[I]?1:-1)*Distance[I]/2000.f,0.f,1.f),FMath::Clamp(Depth[I]/FMath::Max(Body->MaskDepthRange,1.f),0.f,1.f),0,1));
    auto* T=WaterOneShot::Asset<UTexture2D>(PackagePath);
    T->Source.Init(N,N,1,1,TSF_RGBA16F,reinterpret_cast<const uint8*>(Pixels.GetData()));
    T->SRGB=false; T->CompressionSettings=TC_HDR; T->MipGenSettings=TMGS_SimpleAverage; T->Filter=TF_Trilinear; T->AddressX=TA_Clamp; T->AddressY=TA_Clamp;
    T->PostEditChange();
    if(!WaterOneShot::Save(T)) return nullptr;
    UE_LOG(LogTemp,Display,TEXT("WATER_MAP_MASK %s hits=%d wet=%d resolution=%d"),*PackagePath,Hits,WetCount,N);
    return T;
}
int32 UGenerateMaskWaterAssetsCommandlet::Main(const FString& Params)
{
    auto* Lake=WaterOneShot::Mask(TEXT("Lake"),0);
    WaterOneShot::Mask(TEXT("Beach"),1); WaterOneShot::Mask(TEXT("River"),2);
    auto* Sky=WaterOneShot::Sky();
    WaterOneShot::Material(Lake,nullptr); WaterOneShot::Material(Lake,Sky);
    auto* Ground=WaterOneShot::Asset<UMaterial>(TEXT("/StylizedWater/Review/M_ReviewGround"));
    Ground->SetShadingModel(MSM_Unlit);
    auto* Color=WaterOneShot::V(Ground,TEXT("Color"),FLinearColor(.27,.22,.13,1));
    Ground->GetEditorOnlyData()->EmissiveColor.Connect(0,Color); Ground->PostEditChange(); check(WaterOneShot::Save(Ground));
    FAssetCompilingManager::Get().FinishAllCompilation();
    UE_LOG(LogTemp,Display,TEXT("WATER_MASK_REBUILD: explicit generation completed (7 assets)."));
    return 0;
}
