#include "StylizedWaterBodyActor.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

AStylizedWaterBodyActor::AStylizedWaterBodyActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    WaterSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterSurface"));
    WaterSurface->SetupAttachment(SceneRoot);
    WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterSurface->SetGenerateOverlapEvents(false);
    WaterSurface->SetCastShadow(false);
    WaterSurface->SetCanEverAffectNavigation(false);
    BaseMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/StylizedWater/MaskWater/M_WaterMask.M_WaterMask")));
    // WATER_SKY_PARALLAX_EXPERIMENT: soft default is loaded only when enabled.
    SkyTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/StylizedWater/SkyParallax/T_AnimeSky.T_AnimeSky")));
    SkyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/StylizedWater/SkyParallax/M_WaterMaskSky.M_WaterMaskSky")));
}
void AStylizedWaterBodyActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!HasAnyFlags(RF_ClassDefaultObject)) BuildSurface(false);
}
void AStylizedWaterBodyActor::BeginPlay() { Super::BeginPlay(); BuildSurface(false); }
void AStylizedWaterBodyActor::RebuildSurface() { BuildSurface(false); }
void AStylizedWaterBodyActor::FitSurfaceToTerrain() { Modify(); BuildSurface(true); }
void AStylizedWaterBodyActor::ApplyCalmLakePreset() { ApplyPreset(EStylizedWaterPreset::CalmLake); }
void AStylizedWaterBodyActor::ApplyGentleBeachPreset() { ApplyPreset(EStylizedWaterPreset::GentleBeach); }
void AStylizedWaterBodyActor::ApplyFlowingRiverPreset() { ApplyPreset(EStylizedWaterPreset::FlowingRiver); }
void AStylizedWaterBodyActor::ApplyPreset(EStylizedWaterPreset Preset, bool bRebuild)
{
    const bool Beach = Preset == EStylizedWaterPreset::GentleBeach;
    const bool River = Preset == EStylizedWaterPreset::FlowingRiver;
    SurfaceSize = River ? FVector2D(7000,2400) : (Beach ? FVector2D(6000,5000) : FVector2D(5000,5000));
    const TCHAR* MaskName = River ? TEXT("River") : (Beach ? TEXT("Beach") : TEXT("Lake"));
    const FString Path = FString::Printf(TEXT("/StylizedWater/MaskWater/Masks/T_Mask%s.T_Mask%s"), MaskName, MaskName);
    BoundaryMask = LoadObject<UTexture2D>(nullptr, *Path);
    ShoreRunup = Beach ? 45 : (River ? 8 : 14);
    ShoreWaveSpeed = Beach ? 0.13 : 0.09;
    FoamIntensity = Beach ? 0.78 : 0.5;
    FlowSpeed = River ? 90 : 8;
    RippleStrength = River ? 0.22 : 0.12;
    FittedHeights.Reset(); FittedDepths.Reset(); TerrainDepthInfluence = 0;
    if (bRebuild) BuildSurface(false);
}
void AStylizedWaterBodyActor::BuildSurface(bool bTrace)
{
    if (!WaterSurface || HasAnyFlags(RF_ClassDefaultObject)) return;
    const int32 NX = FMath::Clamp(GridResolution.X,2,192), NY = FMath::Clamp(GridResolution.Y,2,192);
    const int32 Count = (NX+1)*(NY+1);
    SurfaceSize.X = FMath::Max(SurfaceSize.X,100.0); SurfaceSize.Y = FMath::Max(SurfaceSize.Y,100.0);
    if (FittedHeights.Num()!=Count || FittedDepths.Num()!=Count) { FittedHeights.Init(0,Count); FittedDepths.Init(0,Count); }
    TArray<FVector> Vertices, Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    TArray<int32> Indices;
    int32 Hits=0;
    for (int32 Y=0;Y<=NY;++Y) for (int32 X=0;X<=NX;++X)
    {
        const int32 I=Y*(NX+1)+X;
        FVector Local((double(X)/NX-0.5)*SurfaceSize.X,(double(Y)/NY-0.5)*SurfaceSize.Y,WaterLevelOffset);
        if (bTrace && GetWorld())
        {
            const FVector P=GetActorTransform().TransformPosition(Local);
            FHitResult Hit;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(WaterMaskTerrainFit),true,this);
            if (GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,TraceHeight),P-FVector(0,0,MaximumDepth),TerrainTraceChannel,Query))
            {
                ++Hits;
                const FVector Ground=GetActorTransform().InverseTransformPosition(Hit.ImpactPoint);
                FittedHeights[I]=FMath::Max(0.0,Ground.Z+TerrainFilmLift-Local.Z);
                FittedDepths[I]=FMath::Max(0.0,P.Z-Hit.ImpactPoint.Z);
            }
            else { FittedHeights[I]=0; FittedDepths[I]=MaximumDepth; }
        }
        Local.Z+=FittedHeights[I];
        Vertices.Add(Local); Normals.Add(FVector::UpVector); UVs.Add(FVector2D(double(X)/NX,double(Y)/NY));
        Colors.Add(FLinearColor(FittedDepths[I]/FMath::Max(DepthColorRange,1.f),0,0,1));
        Tangents.Add(FProcMeshTangent(FVector::ForwardVector,false));
    }
    // Always retain every triangle. Shore coverage is sampled per pixel from the texture.
    for(int32 Y=0;Y<NY;++Y) for(int32 X=0;X<NX;++X)
    {
        int32 I=Y*(NX+1)+X;
        Indices.Append({I,I+NX+2,I+1,I,I+NX+1,I+NX+2});
    }
    WaterSurface->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UVs,Colors,Tangents,false);
    if(bTrace) { LastTerrainFit=FString::Printf(TEXT("%d / %d terrain hits; complete surface retained"),Hits,Count); TerrainDepthInfluence=Hits>0?1.f:0.f; }
    UpdateMaterial();
}
void AStylizedWaterBodyActor::UpdateMaterial()
{
    if(!BoundaryMask) BoundaryMask=LoadObject<UTexture2D>(nullptr,TEXT("/StylizedWater/MaskWater/Masks/T_MaskLake.T_MaskLake"));
    UMaterialInterface* Parent=nullptr;
    // WATER_SKY_PARALLAX_EXPERIMENT: missing experiment falls back to base.
    if(bEnableSkyParallax) Parent=SkyMaterial.LoadSynchronous();
    if(!Parent) Parent=BaseMaterial.LoadSynchronous();
    if(!Parent || !BoundaryMask) { WaterSurface->SetVisibility(false); return; }
    WaterSurface->SetVisibility(true);
    if(!DynamicMaterial || DynamicMaterial->Parent!=Parent) DynamicMaterial=UMaterialInstanceDynamic::Create(Parent,this);
    WaterSurface->SetMaterial(0,DynamicMaterial);
    auto V=[this](const TCHAR* N,FLinearColor C){DynamicMaterial->SetVectorParameterValue(N,C);};
    auto S=[this](const TCHAR* N,float Value){DynamicMaterial->SetScalarParameterValue(N,Value);};
    const FVector P=GetActorLocation(), Scale=GetActorScale3D();
    const FVector2D Center=bWorldLockedMask?WorldMaskCenter:FVector2D(P.X,P.Y);
    const float Angle=FMath::DegreesToRadians(bWorldLockedMask?WorldMaskYaw:GetActorRotation().Yaw);
    const FVector2D Extent(MaskWorldSize.X>0?MaskWorldSize.X:SurfaceSize.X*FMath::Abs(Scale.X),MaskWorldSize.Y>0?MaskWorldSize.Y:SurfaceSize.Y*FMath::Abs(Scale.Y));
    V(TEXT("MaskCenter"),FLinearColor(Center.X,Center.Y,FMath::Cos(Angle),FMath::Sin(Angle)));
    V(TEXT("MaskExtent"),FLinearColor(FMath::Max(Extent.X,1.0),FMath::Max(Extent.Y,1.0),FMath::Max(MaskUVScale.X,0.01),FMath::Max(MaskUVScale.Y,0.01)));
    V(TEXT("MaskOffset"),FLinearColor(MaskUVOffset.X,MaskUVOffset.Y,0,0));
    V(TEXT("Boundary"),FLinearColor(MaskDistanceRange,EdgeFeather,ShoreOffset,ShoreRunup));
    V(TEXT("Shore"),FLinearColor(ShoreWaveSpeed,FoamWidth,ShoreWavelength,FoamIntensity));
    V(TEXT("Surface"),FLinearColor(Opacity,ShoreFilmOpacity,TerrainDepthInfluence,MaskDepthRange/FMath::Max(DepthColorRange,1.f)));
    FVector2D Flow=FlowDirection.GetSafeNormal();
    V(TEXT("Flow"),FLinearColor(Flow.X,Flow.Y,FlowSpeed,RippleScale));
    V(TEXT("ShallowColor"),ShallowColor); V(TEXT("MidColor"),MidColor); V(TEXT("DeepColor"),DeepColor); V(TEXT("FoamColor"),FoamColor);
    S(TEXT("RippleStrength"),RippleStrength); S(TEXT("AnimationSpeed"),AnimationSpeed); S(TEXT("IntersectionFade"),IntersectionFade); S(TEXT("RefractionStrength"),RefractionStrength);
    DynamicMaterial->SetTextureParameterValue(TEXT("BoundaryMask"),BoundaryMask);
    int32 MaskWidth=BoundaryMask->GetSizeX(),MaskHeight=BoundaryMask->GetSizeY();
#if WITH_EDITOR
    // Source dimensions remain correct while the asynchronous texture build has a temporary resource.
    MaskWidth=BoundaryMask->Source.GetSizeX(); MaskHeight=BoundaryMask->Source.GetSizeY();
#endif
    MaskResolution=FString::Printf(TEXT("%d x %d; %.2f x %.2f cm/texel"),MaskWidth,MaskHeight,Extent.X/FMath::Max(MaskWidth,1)/FMath::Max(MaskUVScale.X,0.01),Extent.Y/FMath::Max(MaskHeight,1)/FMath::Max(MaskUVScale.Y,0.01));
    // WATER_SKY_PARALLAX_EXPERIMENT_BEGIN
    if(bEnableSkyParallax)
    {
        if(UTexture2D* Sky=SkyTexture.LoadSynchronous()) DynamicMaterial->SetTextureParameterValue(TEXT("SkyTexture"),Sky);
        V(TEXT("Sky"),FLinearColor(SkyHeight,SkyWorldSize,SkyReflectionStrength,P.Z+WaterLevelOffset*Scale.Z));
        V(TEXT("SkyAnchor"),FLinearColor(SkyWorldAnchor.X,SkyWorldAnchor.Y,0,0));
    }
    // WATER_SKY_PARALLAX_EXPERIMENT_END
}
#if WITH_EDITOR
void AStylizedWaterBodyActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    const FName N=Event.GetMemberPropertyName();
    if(N==GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor,SurfaceSize) || N==GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor,GridResolution) || N==GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor,WaterLevelOffset)) { FittedHeights.Reset(); FittedDepths.Reset(); TerrainDepthInfluence=0; }
    Super::PostEditChangeProperty(Event);
    BuildSurface(false);
}
void AStylizedWaterBodyActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
    if(bFinished) { FittedHeights.Reset(); FittedDepths.Reset(); TerrainDepthInfluence=0; BuildSurface(false); }
}
#endif
