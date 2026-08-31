#include "TunaSweeperDebugRifleSupplyActor.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace TunaSweeperDebugRifleSupplyVisual
{
	constexpr int32 HeartOutlinePointCount = 32;
	constexpr float HeartWidth = 46.0f;
	constexpr float HeartHeight = 40.0f;
	constexpr float HeartThickness = 10.0f;
	constexpr float HeartBottomHeight = 105.0f;
}

ATunaSweeperDebugRifleSupplyActor::ATunaSweeperDebugRifleSupplyActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::ItemSpawn,
		FText::FromString(TEXT("디버그 소총 보급")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 1.0f));
	}

	HeartMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HeartMesh"));
	HeartMesh->SetupAttachment(RootComponent);
	HeartMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeartMesh->bUseAsyncCooking = true;

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	}
}

void ATunaSweeperDebugRifleSupplyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildHeartMesh();
	ApplyVisualMaterials();
}

bool ATunaSweeperDebugRifleSupplyActor::SupplyRifleAndAmmo(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || !GetWorld())
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetWorld()->GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(InstigatorPawn);
	int32 WeaponSlotNumber = TunaCharacter ? TunaCharacter->GetSelectedWeaponSlotNumber() : 0;
	if (!TunaGameInstance->IsEquipmentWeaponSlotOccupied(WeaponSlotNumber))
	{
		WeaponSlotNumber = 0;
		for (int32 CandidateSlotNumber = 1; CandidateSlotNumber <= 2; ++CandidateSlotNumber)
		{
			if (TunaGameInstance->IsEquipmentWeaponSlotOccupied(CandidateSlotNumber))
			{
				WeaponSlotNumber = CandidateSlotNumber;
				break;
			}
		}
	}

	// Condition 1: only add the configured rifle when no gun is equipped.
	if (WeaponSlotNumber == 0)
	{
		if (!TunaGameInstance->AddItemToPreferredAvailableSlot(RifleItemId, 1))
		{
			return false;
		}

		for (int32 CandidateSlotNumber = 1; CandidateSlotNumber <= 2; ++CandidateSlotNumber)
		{
			FTunaSweeperItemInstance WeaponInstance;
			FTunaSweeperItemDefinition WeaponDefinition;
			if (TunaGameInstance->TryGetEquipmentWeaponSlotItem(CandidateSlotNumber, WeaponInstance, WeaponDefinition) &&
				WeaponInstance.ItemId == RifleItemId)
			{
				WeaponSlotNumber = CandidateSlotNumber;
				break;
			}
		}
	}

	if (WeaponSlotNumber == 0)
	{
		return false;
	}

	TArray<int32> CompatibleAmmoItemIds;
	TunaGameInstance->GetCompatibleAmmoItemIdsForWeaponSlot(WeaponSlotNumber, CompatibleAmmoItemIds, false);
	int32 AmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(WeaponSlotNumber);
	if (!CompatibleAmmoItemIds.Contains(AmmoItemId))
	{
		CompatibleAmmoItemIds.Sort();
		AmmoItemId = CompatibleAmmoItemIds.Contains(RifleAmmoItemId)
			? RifleAmmoItemId
			: (CompatibleAmmoItemIds.IsEmpty() ? INDEX_NONE : CompatibleAmmoItemIds[0]);
	}
	if (AmmoItemId == INDEX_NONE ||
		!TunaGameInstance->SetSelectedAmmoItemForWeaponSlot(WeaponSlotNumber, AmmoItemId))
	{
		return false;
	}

	// Condition 2: add only the rounds needed for this reload, then fill the magazine.
	const int32 MagazineCapacity = TunaGameInstance->GetWeaponMagazineCapacity(WeaponSlotNumber);
	const int32 LoadedAmmoCount = TunaGameInstance->GetWeaponLoadedAmmoCount(WeaponSlotNumber);
	const int32 MissingMagazineAmmo = FMath::Max(0, MagazineCapacity - LoadedAmmoCount);
	if (MissingMagazineAmmo > 0)
	{
		const int32 CurrentReserveAmmo = TunaGameInstance->GetWeaponInventoryAmmoCount(WeaponSlotNumber);
		const int32 ReloadSupplyDeficit = FMath::Max(0, MissingMagazineAmmo - CurrentReserveAmmo);
		if (ReloadSupplyDeficit > 0 &&
			!TunaGameInstance->AddItemToFirstAvailableInventorySlot(AmmoItemId, ReloadSupplyDeficit))
		{
			return false;
		}

		int32 ReloadedAmmoCount = LoadedAmmoCount;
		if (!TunaGameInstance->TryReloadWeaponSlot(WeaponSlotNumber, AmmoItemId, ReloadedAmmoCount) ||
			ReloadedAmmoCount < MagazineCapacity)
		{
			return false;
		}
	}

	// Condition 3: after the independent magazine check, top reserve ammo up to 60.
	const int32 ReserveAmmoCount = TunaGameInstance->GetWeaponInventoryAmmoCount(WeaponSlotNumber);
	const int32 MissingReserveAmmo = FMath::Max(0, MinimumReserveAmmo - ReserveAmmoCount);
	if (MissingReserveAmmo > 0 &&
		!TunaGameInstance->AddItemToFirstAvailableInventorySlot(AmmoItemId, MissingReserveAmmo))
	{
		return false;
	}

	if (TunaCharacter)
	{
		TunaCharacter->SelectWeaponSlot(WeaponSlotNumber);
	}

	return TunaGameInstance->GetWeaponLoadedAmmoCount(WeaponSlotNumber) >= MagazineCapacity &&
		TunaGameInstance->GetWeaponInventoryAmmoCount(WeaponSlotNumber) >= MinimumReserveAmmo;
}

void ATunaSweeperDebugRifleSupplyActor::BuildHeartMesh()
{
	if (!HeartMesh)
	{
		return;
	}

	using namespace TunaSweeperDebugRifleSupplyVisual;
	TArray<FVector2D> Outline;
	Outline.Reserve(HeartOutlinePointCount);
	float MinX = TNumericLimits<float>::Max();
	float MaxX = TNumericLimits<float>::Lowest();
	float MinZ = TNumericLimits<float>::Max();
	float MaxZ = TNumericLimits<float>::Lowest();
	for (int32 PointIndex = 0; PointIndex < HeartOutlinePointCount; ++PointIndex)
	{
		const float T = 2.0f * PI * static_cast<float>(PointIndex) / static_cast<float>(HeartOutlinePointCount);
		const float SinT = FMath::Sin(T);
		const FVector2D Point(
			16.0f * SinT * SinT * SinT,
			13.0f * FMath::Cos(T) - 5.0f * FMath::Cos(2.0f * T) - 2.0f * FMath::Cos(3.0f * T) - FMath::Cos(4.0f * T));
		Outline.Add(Point);
		MinX = FMath::Min(MinX, Point.X);
		MaxX = FMath::Max(MaxX, Point.X);
		MinZ = FMath::Min(MinZ, Point.Y);
		MaxZ = FMath::Max(MaxZ, Point.Y);
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(HeartOutlinePointCount * 6 + 2);
	Normals.Reserve(HeartOutlinePointCount * 6 + 2);
	UVs.Reserve(HeartOutlinePointCount * 6 + 2);
	VertexColors.Reserve(HeartOutlinePointCount * 6 + 2);
	Tangents.Reserve(HeartOutlinePointCount * 6 + 2);

	auto AddVertex = [&](const FVector& Position, const FVector& Normal, const FVector2D& UV)
	{
		Vertices.Add(Position);
		Normals.Add(Normal);
		UVs.Add(UV);
		VertexColors.Add(FLinearColor(0.85f, 0.015f, 0.025f, 1.0f));
		Tangents.Emplace(FVector::XAxisVector, false);
	};

	const float HalfThickness = HeartThickness * 0.5f;
	for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
	{
		const float Y = SideIndex == 0 ? -HalfThickness : HalfThickness;
		const FVector Normal = SideIndex == 0 ? -FVector::YAxisVector : FVector::YAxisVector;
		for (const FVector2D& Point : Outline)
		{
			const float X = ((Point.X - MinX) / (MaxX - MinX) - 0.5f) * HeartWidth;
			const float Z = HeartBottomHeight + ((Point.Y - MinZ) / (MaxZ - MinZ)) * HeartHeight;
			AddVertex(
				FVector(X, Y, Z),
				Normal,
				FVector2D((Point.X - MinX) / (MaxX - MinX), (Point.Y - MinZ) / (MaxZ - MinZ)));
		}
	}

	const int32 FrontCenterIndex = Vertices.Num();
	AddVertex(FVector(0.0f, -HalfThickness, HeartBottomHeight + HeartHeight * 0.48f), -FVector::YAxisVector, FVector2D(0.5f, 0.48f));
	const int32 BackCenterIndex = Vertices.Num();
	AddVertex(FVector(0.0f, HalfThickness, HeartBottomHeight + HeartHeight * 0.48f), FVector::YAxisVector, FVector2D(0.5f, 0.48f));

	for (int32 PointIndex = 0; PointIndex < HeartOutlinePointCount; ++PointIndex)
	{
		const int32 NextPointIndex = (PointIndex + 1) % HeartOutlinePointCount;
		Triangles.Append({ FrontCenterIndex, NextPointIndex, PointIndex });
		Triangles.Append({ BackCenterIndex, HeartOutlinePointCount + PointIndex, HeartOutlinePointCount + NextPointIndex });

		const FVector FrontA = Vertices[PointIndex];
		const FVector FrontB = Vertices[NextPointIndex];
		const FVector BackA = Vertices[HeartOutlinePointCount + PointIndex];
		const FVector BackB = Vertices[HeartOutlinePointCount + NextPointIndex];
		const FVector EdgeNormal = FVector::CrossProduct(FrontB - FrontA, BackA - FrontA).GetSafeNormal();
		const int32 SideVertexStart = Vertices.Num();
		AddVertex(FrontA, EdgeNormal, FVector2D(0.0f, 0.0f));
		AddVertex(FrontB, EdgeNormal, FVector2D(1.0f, 0.0f));
		AddVertex(BackB, EdgeNormal, FVector2D(1.0f, 1.0f));
		AddVertex(BackA, EdgeNormal, FVector2D(0.0f, 1.0f));
		Triangles.Append({ SideVertexStart, SideVertexStart + 1, SideVertexStart + 2 });
		Triangles.Append({ SideVertexStart, SideVertexStart + 2, SideVertexStart + 3 });
	}

	HeartMesh->ClearAllMeshSections();
	HeartMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false);
}

void ATunaSweeperDebugRifleSupplyActor::ApplyVisualMaterials()
{
	if (VisualMesh)
	{
		VisualMesh->SetMaterial(
			0,
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	}
	if (HeartMesh)
	{
		HeartMesh->SetMaterial(
			0,
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial")));
	}
}
