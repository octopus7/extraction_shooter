#include "Interaction/TunaSweeperSandbagCoverActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultSandbagMaterialPath = TEXT("/Game/Interaction/M_SandbagCover_Burlap.M_SandbagCover_Burlap");
	const TCHAR* DefaultSandbagOutlineMaterialPath = TEXT("/Game/Interaction/M_SandbagCover_Outline.M_SandbagCover_Outline");
	const TCHAR* FallbackVertexColorMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	struct FSandbagMeshPiece
	{
		FVector Center = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
	};

	FVector MakeSafeBoxExtent(const FVector& InBoxExtent)
	{
		return FVector(
			FMath::Max(1.0f, InBoxExtent.X),
			FMath::Max(1.0f, InBoxExtent.Y),
			FMath::Max(1.0f, InBoxExtent.Z));
	}

	void AddQuad(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& P0,
		const FVector& P1,
		const FVector& P2,
		const FVector& P3,
		const FLinearColor& VertexColor)
	{
		const int32 BaseIndex = Vertices.Num();
		Vertices.Add(P0);
		Vertices.Add(P1);
		Vertices.Add(P2);
		Vertices.Add(P3);

		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 3);

		const FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
		const FVector TangentDirection = (P1 - P0).GetSafeNormal();
		const FProcMeshTangent Tangent(TangentDirection.IsNearlyZero() ? FVector::ForwardVector : TangentDirection, false);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Normals.Add(Normal);
			VertexColors.Add(VertexColor);
			Tangents.Add(Tangent);
		}

		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));
	}

	void AddBox(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& Center,
		const FVector& Extent,
		const FLinearColor& VertexColor)
	{
		const FVector Min = Center - Extent;
		const FVector Max = Center + Extent;
		const FVector P000(Min.X, Min.Y, Min.Z);
		const FVector P001(Min.X, Min.Y, Max.Z);
		const FVector P010(Min.X, Max.Y, Min.Z);
		const FVector P011(Min.X, Max.Y, Max.Z);
		const FVector P100(Max.X, Min.Y, Min.Z);
		const FVector P101(Max.X, Min.Y, Max.Z);
		const FVector P110(Max.X, Max.Y, Min.Z);
		const FVector P111(Max.X, Max.Y, Max.Z);

		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P001, P101, P111, P011, VertexColor);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P000, P010, P110, P100, VertexColor);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P100, P110, P111, P101, VertexColor);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P000, P001, P011, P010, VertexColor);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P010, P011, P111, P110, VertexColor);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P000, P100, P101, P001, VertexColor);
	}
}

ATunaSweeperSandbagCoverActor::ATunaSweeperSandbagCoverActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.08f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	VisualMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);

	OutlineMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OutlineMesh"));
	OutlineMesh->SetupAttachment(RootComponent);
	OutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OutlineMesh->SetGenerateOverlapEvents(false);
	OutlineMesh->SetCastShadow(false);
	OutlineMesh->SetHiddenInGame(true);
	OutlineMesh->SetVisibility(false);

	VisualMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultSandbagMaterialPath));
	OutlineMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultSandbagOutlineMaterialPath));

	ApplyCollisionDefaults();
	RebuildMeshes();
}

void ATunaSweeperSandbagCoverActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BoxExtent = MakeSafeBoxExtent(BoxExtent);
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	PassthroughRadius = FMath::Max(0.0f, PassthroughRadius);
	PassthroughVerticalTolerance = FMath::Max(0.0f, PassthroughVerticalTolerance);
	OutlineThickness = FMath::Max(0.5f, OutlineThickness);

	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
}

void ATunaSweeperSandbagCoverActor::BeginPlay()
{
	Super::BeginPlay();

	BoxExtent = MakeSafeBoxExtent(BoxExtent);
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	bCoverDestroyed = false;

	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
	UpdatePassthroughOutline();
}

void ATunaSweeperSandbagCoverActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePassthroughOutline();
}

float ATunaSweeperSandbagCoverActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bCoverDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	UpdateDamageVisual();

	if (CurrentHealth <= 0.0f)
	{
		DestroyCover();
	}

	return AppliedDamage;
}

void ATunaSweeperSandbagCoverActor::ConfigureCoverDefaults(
	FName InCoverId,
	const FVector& InBoxExtent,
	float InMaxHealth,
	float InPassthroughRadius)
{
	CoverId = InCoverId;
	BoxExtent = MakeSafeBoxExtent(InBoxExtent);
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	PassthroughRadius = FMath::Max(0.0f, InPassthroughRadius);

	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
}

void ATunaSweeperSandbagCoverActor::ConfigureCoverVisualDefaults(
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
	TSoftObjectPtr<UMaterialInterface> InOutlineMaterial)
{
	if (!InVisualMaterial.IsNull())
	{
		VisualMaterial = InVisualMaterial;
	}
	if (!InOutlineMaterial.IsNull())
	{
		OutlineMaterial = InOutlineMaterial;
	}

	ApplyMaterials();
	UpdateDamageVisual();
}

bool ATunaSweeperSandbagCoverActor::ShouldAllowPlayerProjectilePassthrough(APawn* InstigatorPawn) const
{
	if (bCoverDestroyed || !InstigatorPawn || !InstigatorPawn->IsPlayerControlled())
	{
		return false;
	}

	const FVector LocalPawnLocation = GetActorTransform().InverseTransformPosition(InstigatorPawn->GetActorLocation());
	const float OutsideX = FMath::Max(FMath::Abs(LocalPawnLocation.X) - BoxExtent.X, 0.0f);
	const float OutsideY = FMath::Max(FMath::Abs(LocalPawnLocation.Y) - BoxExtent.Y, 0.0f);
	const float HorizontalDistanceSquared = OutsideX * OutsideX + OutsideY * OutsideY;
	const float SafePassthroughRadius = FMath::Max(0.0f, PassthroughRadius);
	if (HorizontalDistanceSquared > FMath::Square(SafePassthroughRadius))
	{
		return false;
	}

	const float MinZ = -PassthroughVerticalTolerance;
	const float MaxZ = BoxExtent.Z * 2.0f + PassthroughVerticalTolerance;
	return LocalPawnLocation.Z >= MinZ && LocalPawnLocation.Z <= MaxZ;
}

void ATunaSweeperSandbagCoverActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	BlockingCollision->SetRelativeLocation(FVector(0.0f, 0.0f, BoxExtent.Z));
	BlockingCollision->SetBoxExtent(BoxExtent);
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldStatic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
}

void ATunaSweeperSandbagCoverActor::RebuildMeshes()
{
	if (!VisualMesh || !OutlineMesh)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	TArray<FSandbagMeshPiece> MeshPieces;

	constexpr int32 LayerCount = 3;
	constexpr int32 DepthCount = 2;
	const float LayerHeight = BoxExtent.Z * 2.0f / static_cast<float>(LayerCount);
	const float DepthCenterOffset = BoxExtent.X * 0.34f;
	const float DepthExtent = BoxExtent.X * 0.43f;

	for (int32 LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
	{
		const int32 ColumnCount = (LayerIndex % 2 == 0) ? 4 : 5;
		const float SegmentLength = BoxExtent.Y * 2.0f / static_cast<float>(ColumnCount);
		const float ZCenter = LayerHeight * (static_cast<float>(LayerIndex) + 0.5f);
		const float LayerShade = 1.0f - static_cast<float>(LayerIndex) * 0.035f;

		for (int32 DepthIndex = 0; DepthIndex < DepthCount; ++DepthIndex)
		{
			const float XCenter = DepthIndex == 0 ? -DepthCenterOffset : DepthCenterOffset;
			const float DepthShade = DepthIndex == 0 ? 0.94f : 1.0f;
			for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
			{
				const float YCenter = -BoxExtent.Y + SegmentLength * (static_cast<float>(ColumnIndex) + 0.5f);
				const FVector BagCenter(XCenter, YCenter, ZCenter);
				const FVector BagExtent(DepthExtent, SegmentLength * 0.455f, LayerHeight * 0.42f);
				const float ColumnShade = 0.96f + 0.02f * static_cast<float>((ColumnIndex + LayerIndex) % 2);
				const float Shade = LayerShade * DepthShade * ColumnShade;
				const FLinearColor BagColor(
					0.92f * Shade,
					0.78f * Shade,
					0.52f * Shade,
					1.0f);
				AddBox(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, BagCenter, BagExtent, BagColor);
				MeshPieces.Add(FSandbagMeshPiece{ BagCenter, BagExtent });
			}
		}
	}

	VisualMesh->ClearAllMeshSections();
	VisualMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false);

	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UVs.Reset();
	VertexColors.Reset();
	Tangents.Reset();

	const float T = FMath::Max(0.5f, OutlineThickness);
	const FLinearColor OutlineColor(0.15f, 0.95f, 1.0f, 1.0f);

	for (const FSandbagMeshPiece& MeshPiece : MeshPieces)
	{
		AddBox(
			Vertices,
			Triangles,
			Normals,
			UVs,
			VertexColors,
			Tangents,
			MeshPiece.Center,
			MeshPiece.Extent + FVector(T),
			OutlineColor);
	}

	OutlineMesh->ClearAllMeshSections();
	OutlineMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false);
	SetOutlineActive(bOutlineActive);
}

void ATunaSweeperSandbagCoverActor::ApplyMaterials()
{
	if (VisualMesh)
	{
		DynamicVisualMaterial = nullptr;
		UMaterialInterface* LoadedVisualMaterial = VisualMaterial.LoadSynchronous();
		if (!LoadedVisualMaterial)
		{
			LoadedVisualMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackVertexColorMaterialPath);
		}
		if (LoadedVisualMaterial)
		{
			DynamicVisualMaterial = VisualMesh->CreateDynamicMaterialInstance(0, LoadedVisualMaterial);
		}
	}

	if (OutlineMesh)
	{
		DynamicOutlineMaterial = nullptr;
		if (UMaterialInterface* LoadedOutlineMaterial = OutlineMaterial.LoadSynchronous())
		{
			DynamicOutlineMaterial = OutlineMesh->CreateDynamicMaterialInstance(0, LoadedOutlineMaterial);
		}
	}
}

void ATunaSweeperSandbagCoverActor::UpdateDamageVisual()
{
	if (!DynamicVisualMaterial)
	{
		return;
	}

	const float DamageAlpha = 1.0f - FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("DamageAlpha"), DamageAlpha);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("DamageTint"), FLinearColor(0.46f, 0.37f, 0.26f, 1.0f));
}

void ATunaSweeperSandbagCoverActor::UpdatePassthroughOutline()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	SetOutlineActive(ShouldAllowPlayerProjectilePassthrough(PlayerPawn));
}

void ATunaSweeperSandbagCoverActor::SetOutlineActive(bool bEnabled)
{
	bOutlineActive = bEnabled && !bCoverDestroyed;

	if (OutlineMesh)
	{
		OutlineMesh->SetHiddenInGame(!bOutlineActive);
		OutlineMesh->SetVisibility(bOutlineActive, true);
	}

	if (VisualMesh)
	{
		VisualMesh->SetRenderCustomDepth(bOutlineActive);
		VisualMesh->SetCustomDepthStencilValue(3);
	}
}

void ATunaSweeperSandbagCoverActor::DestroyCover()
{
	if (bCoverDestroyed)
	{
		return;
	}

	bCoverDestroyed = true;
	SetOutlineActive(false);
	SetActorEnableCollision(false);

	if (BlockingCollision)
	{
		BlockingCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(true);
		VisualMesh->SetVisibility(false, true);
	}

	Destroy();
}
