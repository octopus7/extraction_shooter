#include "Environment/TunaSweeperAnimeTreeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CanopyRimSubsystem.h"
#include "Engine/World.h"

namespace
{
const FName ClumpTag(TEXT("AnimeTreeClump"));
const FName ShadowTag(TEXT("AnimeTreeShadow"));

}

ATunaSweeperAnimeTreeActor::ATunaSweeperAnimeTreeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TreeRoot")));
	Trunk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trunk"));
	Trunk->SetupAttachment(RootComponent);
	Trunk->SetCollisionProfileName(TEXT("BlockAll"));
	GradientGuide = CreateDefaultSubobject<USceneComponent>(TEXT("GradientGuide"));
	GradientGuide->SetupAttachment(RootComponent);
	GradientGuide->SetRelativeLocation(FVector(0, 0, 300));
	GradientGuide->SetRelativeRotation(FVector(.32, -.54, .78).Rotation());
}

void ATunaSweeperAnimeTreeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshTree();
}

void ATunaSweeperAnimeTreeActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshTree();
}

void ATunaSweeperAnimeTreeActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	if (!IsTemplate() && GetWorld()) RefreshTree();
}

void ATunaSweeperAnimeTreeActor::PostUnregisterAllComponents()
{
	if (UWorld* World = GetWorld())
		if (auto* Rim = World->GetSubsystem<UCanopyRimSubsystem>()) Rim->RemoveTree(this);
	Super::PostUnregisterAllComponents();
}

TArray<UStaticMeshComponent*> ATunaSweeperAnimeTreeActor::GetLeafClumps() const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	Components.RemoveAll([](const UStaticMeshComponent* C) { return !C->ComponentHasTag(ClumpTag); });
	return Components;
}

void ATunaSweeperAnimeTreeActor::SetTreeParameters(float NewWindStrength, float NewLeafCardScale, float NewLeafDensity)
{
	WindStrength = NewWindStrength;
	LeafCardScale = NewLeafCardScale;
	LeafDensity = NewLeafDensity;
	RefreshTree();
}

void ATunaSweeperAnimeTreeActor::RefreshTree()
{
	if (bRefreshingTree) return;
	TGuardValue<bool> Guard(bRefreshingTree, true);
	WindStrength = FMath::Clamp(WindStrength, 0.0f, 1.5f);
	LeafCardScale = FMath::Clamp(LeafCardScale, 0.2f, 1.4f);
	LeafDensity = FMath::Clamp(LeafDensity, 0.0f, 1.0f);
	GradientWidth = FMath::Max(GradientWidth, 1.f);
	GradientStrength = FMath::Clamp(GradientStrength, 0.f, 1.f);
	GradientExponent = FMath::Clamp(GradientExponent, .1f, 4.f);
	ClumpShadingStrength = FMath::Clamp(ClumpShadingStrength, 0.f, 1.f);
	RimStrength = FMath::Clamp(RimStrength, 0.f, 1.f);
	RimWidthPixels = FMath::Clamp(RimWidthPixels, 1.f, 128.f);
	RimBrightness = FMath::Clamp(RimBrightness, 0.f, 4.f);

	// Rebuild only on edits/construction, never per frame. Remove stale proxies after deletions too.
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	for (auto* Component : Components)
		if (Component->ComponentHasTag(ShadowTag)) Component->DestroyComponent();
	if (!LeafMaterial) return;
	if (!LeafInstance || LeafInstance->Parent != LeafMaterial || LeafInstance->GetOuter() != this)
		LeafInstance = UMaterialInstanceDynamic::Create(LeafMaterial, this);
	if (!ShadowInstance || ShadowInstance->Parent != LeafMaterial || ShadowInstance->GetOuter() != this)
		ShadowInstance = UMaterialInstanceDynamic::Create(LeafMaterial, this);
	for (UMaterialInstanceDynamic* Instance : {LeafInstance.Get(), ShadowInstance.Get()})
	{
		Instance->SetScalarParameterValue(TEXT("WindStrength"), WindStrength);
		Instance->SetScalarParameterValue(TEXT("LeafCardScale"), LeafCardScale);
		Instance->SetScalarParameterValue(TEXT("LeafDensity"), LeafDensity);
		Instance->SetScalarParameterValue(TEXT("GradientStrength"), GradientStrength);
		Instance->SetScalarParameterValue(TEXT("GradientExponent"), GradientExponent);
		Instance->SetScalarParameterValue(TEXT("ClumpShadingStrength"), ClumpShadingStrength);
		Instance->SetVectorParameterValue(TEXT("GradientDarkColor"), GradientDarkColor);
		Instance->SetVectorParameterValue(TEXT("GradientLightColor"), GradientLightColor);
	}
	LeafInstance->SetScalarParameterValue(TEXT("StableShadowProxy"), 0.f);
	ShadowInstance->SetScalarParameterValue(TEXT("StableShadowProxy"), 1.f);
	for (auto* Clump : GetLeafClumps())
	{
		Clump->SetMaterial(0, LeafInstance);
		Clump->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Clump->SetCanEverAffectNavigation(false);
		Clump->SetCastShadow(false);
		Clump->SetReceivesDecals(false);
		Clump->SetBoundsScale(2.f);
		Clump->TransformUpdated.RemoveAll(this);
		Clump->TransformUpdated.AddUObject(this, &ThisClass::OnTreeComponentTransformUpdated);
		if (!Clump->GetStaticMesh()) continue;
		auto* Shadow = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient | RF_DuplicateTransient);
		Shadow->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Shadow->ComponentTags.Add(ShadowTag);
		Shadow->SetupAttachment(Clump);
		Shadow->SetMobility(Clump->Mobility);
		Shadow->SetStaticMesh(Clump->GetStaticMesh());
		Shadow->SetMaterial(0, ShadowInstance);
		Shadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shadow->SetCanEverAffectNavigation(false);
		Shadow->SetRenderInMainPass(false);
		Shadow->SetRenderInDepthPass(false);
		Shadow->SetCastShadow(true);
		Shadow->bCastHiddenShadow = true;
		Shadow->SetBoundsScale(2.f);
		Shadow->RegisterComponent();
	}
	GradientGuide->TransformUpdated.RemoveAll(this);
	GradientGuide->TransformUpdated.AddUObject(this, &ThisClass::OnTreeComponentTransformUpdated);
	UpdateGradientData();
	UpdateRimMask();
}

void ATunaSweeperAnimeTreeActor::OnTreeComponentTransformUpdated(USceneComponent*, EUpdateTransformFlags, ETeleportType)
{
	if (!bRefreshingTree) UpdateGradientData();
}

void ATunaSweeperAnimeTreeActor::UpdateGradientData()
{
	// Use the actual render transforms: relative matrix chains introduce shear that
	// UE's FTransform hierarchy does not retain under rotated, nonuniform scales.
	const FMatrix Guide = GradientGuide->GetComponentTransform().ToMatrixWithScale();
	// A collapsed guide has a defined flat midpoint, never NaN/Inf shader input.
	const bool bValid = FMath::Abs(Guide.Determinant()) > UE_SMALL_NUMBER;
	const FMatrix Inverse = bValid ? Guide.Inverse() : FMatrix::Identity;
	for (auto* Clump : GetLeafClumps())
	{
		const FMatrix Mapping = Clump->GetComponentTransform().ToMatrixWithScale() * Inverse;
		const double Width = FMath::Max(GradientWidth, 1.f);
		const FVector4 Coefficients = bValid ? FVector4(Mapping.M[0][0]/Width, Mapping.M[1][0]/Width,
			Mapping.M[2][0]/Width, Mapping.M[3][0]/Width) : FVector4(0,0,0,0);
		Clump->SetCustomPrimitiveDataVector4(0, Coefficients);
	}
}

void ATunaSweeperAnimeTreeActor::UpdateRimMask()
{
	uint8 Id = 0;
	if (UWorld* World = GetWorld())
		if (auto* Rim = World->GetSubsystem<UCanopyRimSubsystem>())
			Id = Rim->UpdateTree(this, RimColor, LeafDensity > 0 ? RimStrength : 0.f, RimWidthPixels, RimBrightness);
	for (auto* Clump : GetLeafClumps())
	{
		Clump->SetCustomDepthStencilValue(Id);
		Clump->SetRenderCustomDepth(Id != 0);
	}
}
