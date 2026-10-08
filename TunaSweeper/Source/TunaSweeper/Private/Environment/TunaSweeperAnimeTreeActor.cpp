#include "Environment/TunaSweeperAnimeTreeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ATunaSweeperAnimeTreeActor::ATunaSweeperAnimeTreeActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("TreeRoot")));
	Trunk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trunk"));
	Trunk->SetupAttachment(RootComponent);
	Trunk->SetCollisionProfileName(TEXT("BlockAll"));
	Leaves = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Leaves"));
	Leaves->SetupAttachment(RootComponent);
	Leaves->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Leaves->SetCanEverAffectNavigation(false);
	Leaves->SetCastShadow(false);
	Leaves->SetReceivesDecals(false);
	Leaves->SetBoundsScale(2.0f);
	ShadowProxy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShadowProxy"));
	ShadowProxy->SetupAttachment(RootComponent);
	ShadowProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowProxy->SetCanEverAffectNavigation(false);
	ShadowProxy->SetRenderInMainPass(false);
	ShadowProxy->SetRenderInDepthPass(false);
	ShadowProxy->SetCastShadow(true);
	ShadowProxy->bCastHiddenShadow = true;
	ShadowProxy->SetBoundsScale(2.0f);
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

void ATunaSweeperAnimeTreeActor::SetTreeParameters(float NewWindStrength, float NewLeafCardScale, float NewLeafDensity)
{
	WindStrength = NewWindStrength;
	LeafCardScale = NewLeafCardScale;
	LeafDensity = NewLeafDensity;
	RefreshTree();
}

void ATunaSweeperAnimeTreeActor::RefreshTree()
{
	WindStrength = FMath::Clamp(WindStrength, 0.0f, 1.5f);
	LeafCardScale = FMath::Clamp(LeafCardScale, 0.2f, 1.4f);
	LeafDensity = FMath::Clamp(LeafDensity, 0.0f, 1.0f);
	if (!LeafMaterial) return;
	if (!LeafInstance || LeafInstance->Parent != LeafMaterial || LeafInstance->GetOuter() != this)
		LeafInstance = UMaterialInstanceDynamic::Create(LeafMaterial, this);
	if (!ShadowInstance || ShadowInstance->Parent != LeafMaterial || ShadowInstance->GetOuter() != this)
		ShadowInstance = UMaterialInstanceDynamic::Create(LeafMaterial, this);
	Leaves->SetMaterial(0, LeafInstance);
	ShadowProxy->SetMaterial(0, ShadowInstance);
	for (UMaterialInstanceDynamic* Instance : {LeafInstance.Get(), ShadowInstance.Get()})
	{
		Instance->SetScalarParameterValue(TEXT("WindStrength"), WindStrength);
		Instance->SetScalarParameterValue(TEXT("LeafCardScale"), LeafCardScale);
		Instance->SetScalarParameterValue(TEXT("LeafDensity"), LeafDensity);
	}
	LeafInstance->SetScalarParameterValue(TEXT("StableShadowProxy"), 0.0f);
	ShadowInstance->SetScalarParameterValue(TEXT("StableShadowProxy"), 1.0f);
}
