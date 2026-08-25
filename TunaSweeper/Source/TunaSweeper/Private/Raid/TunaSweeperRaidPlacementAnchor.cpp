#include "Raid/TunaSweeperRaidPlacementAnchor.h"

#include "Components/SceneComponent.h"

#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#endif

ATunaSweeperRaidPlacementAnchor::ATunaSweeperRaidPlacementAnchor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

#if WITH_EDITORONLY_DATA
	EditorPreviewArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("EditorPreviewArrow"));
	EditorPreviewArrow->SetupAttachment(SceneRoot);
	EditorPreviewArrow->SetIsVisualizationComponent(true);
	EditorPreviewArrow->ArrowSize = 1.25f;
	EditorPreviewArrow->ArrowLength = 120.0f;
	EditorPreviewArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EditorPreviewBillboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorPreviewBillboard"));
	EditorPreviewBillboard->SetupAttachment(SceneRoot);
	EditorPreviewBillboard->SetIsVisualizationComponent(true);
	EditorPreviewBillboard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorPreviewBillboard->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
	static ConstructorHelpers::FObjectFinderOptional<UTexture2D> ActorSprite(TEXT("/Engine/EditorResources/S_Actor.S_Actor"));
	if (ActorSprite.Succeeded())
	{
		EditorPreviewBillboard->SetSprite(ActorSprite.Get());
	}

	EditorLootBoxPreview = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("EditorLootBoxPreview"));
	EditorLootBoxPreview->SetupAttachment(SceneRoot);
	EditorLootBoxPreview->SetIsVisualizationComponent(true);
	EditorLootBoxPreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorLootBoxPreview->SetCastShadow(false);
	EditorLootBoxPreview->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	EditorLootBoxPreview->SetRelativeScale3D(FVector(1.0f, 0.7f, 0.6f));
	static ConstructorHelpers::FObjectFinderOptional<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		EditorLootBoxPreview->SetStaticMesh(CubeMesh.Get());
	}

	EditorPreviewLabel = CreateEditorOnlyDefaultSubobject<UTextRenderComponent>(TEXT("EditorPreviewLabel"));
	EditorPreviewLabel->SetupAttachment(SceneRoot);
	EditorPreviewLabel->SetIsVisualizationComponent(true);
	EditorPreviewLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EditorPreviewLabel->SetHorizontalAlignment(EHTA_Center);
	EditorPreviewLabel->SetWorldSize(24.0f);
	EditorPreviewLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
#endif
}

void ATunaSweeperRaidPlacementAnchor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEditorPreview();
}

void ATunaSweeperRaidPlacementAnchor::RefreshEditorPreview()
{
#if WITH_EDITORONLY_DATA
	const bool bIsEnemy = AnchorKind == ETunaSweeperRaidPlacementAnchorKind::Enemy;
	const FColor PreviewColor = bIsEnemy ? FColor(215, 78, 66) : FColor(224, 175, 62);
	if (EditorPreviewArrow)
	{
		EditorPreviewArrow->ArrowColor = PreviewColor;
	}
	if (EditorPreviewBillboard)
	{
		EditorPreviewBillboard->SetVisibility(bIsEnemy);
	}
	if (EditorLootBoxPreview)
	{
		EditorLootBoxPreview->SetVisibility(!bIsEnemy);
	}
	if (EditorPreviewLabel)
	{
		const TCHAR* KindName = bIsEnemy ? TEXT("ENEMY") : TEXT("LOOT BOX");
		EditorPreviewLabel->SetText(FText::FromString(FString::Printf(TEXT("%s #%d"), KindName, PlacementId)));
		EditorPreviewLabel->SetTextRenderColor(PreviewColor);
	}
#endif
}
