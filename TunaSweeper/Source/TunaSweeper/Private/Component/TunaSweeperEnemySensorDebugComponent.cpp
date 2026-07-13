#include "Component/TunaSweeperEnemySensorDebugComponent.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace TunaSweeperEnemySensorDebug
{
	constexpr float MeshHeight = 4.0f;
	constexpr int32 VisionSegments = 48;
	constexpr int32 HearingArcSegments = 12;
	constexpr float HearingArcHalfWidth = 14.0f;
	constexpr float HearingGaugeWidth = 230.0f;
	constexpr float HearingGaugeHalfHeight = 15.0f;
	constexpr float HearingGaugeOffset = 38.0f;
	constexpr float HearingGaugeTickHalfWidth = 3.0f;
	constexpr float HearingGradientBandWidth = 300.0f;
	const TCHAR* MaterialPath = TEXT("/Game/Characters/Enemy/M_EnemySensorDebug.M_EnemySensorDebug");

	void AddTriangle(TArray<int32>& Triangles, int32 A, int32 B, int32 C)
	{
		Triangles.Add(A);
		Triangles.Add(B);
		Triangles.Add(C);
	}
}

UTunaSweeperEnemySensorDebugComponent::UTunaSweeperEnemySensorDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTunaSweeperEnemySensorDebugComponent::OnRegister()
{
	Super::OnRegister();
	EnsureMeshComponents();
	SetMeshesVisible(false);
}

void UTunaSweeperEnemySensorDebugComponent::OnUnregister()
{
	if (HearingMesh)
	{
		HearingMesh->DestroyComponent();
		HearingMesh = nullptr;
	}
	if (VisionMesh)
	{
		VisionMesh->DestroyComponent();
		VisionMesh = nullptr;
	}
	Super::OnUnregister();
}

void UTunaSweeperEnemySensorDebugComponent::SetSensorDebugVisible(bool bVisible)
{
	if (bSensorDebugVisible == bVisible)
	{
		return;
	}

	bSensorDebugVisible = bVisible;
	EnsureMeshComponents();
	CachedVisionRange = -1.0f;
	CachedVisionAngleDegrees = -1.0f;
	SetMeshesVisible(bVisible);
}

void UTunaSweeperEnemySensorDebugComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSensorDebugVisible)
	{
		return;
	}

	const ATunaSweeperEnemyAIController* EnemyController = GetOwner()
		? Cast<ATunaSweeperEnemyAIController>(GetOwner()->GetInstigatorController())
		: nullptr;
	if (!EnemyController)
	{
		if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			EnemyController = Cast<ATunaSweeperEnemyAIController>(OwnerPawn->GetController());
		}
	}
	if (!EnemyController)
	{
		return;
	}

	FTunaSweeperEnemyCombatDebugSnapshot Snapshot;
	if (!EnemyController->GetCombatDebugSnapshot(Snapshot))
	{
		return;
	}

	if (!FMath::IsNearlyEqual(CachedVisionRange, Snapshot.TrackingRange, 1.0f) ||
		!FMath::IsNearlyEqual(CachedVisionAngleDegrees, Snapshot.VisionAngleDegrees, 0.1f))
	{
		RebuildMeshes(Snapshot.TrackingRange, Snapshot.VisionAngleDegrees);
	}

	FVector LocalPlayerDirection;
	float HearingProgress = 0.0f;
	const bool bHasVisualAwareness = Snapshot.bHasDirectTargetSight || Snapshot.bIsCombatEngaged;
	if (!bHasVisualAwareness && TryGetPlayerHearingState(
		Snapshot.HearingRange > 0.0f ? Snapshot.HearingRange : HearingOuterRadius,
		LocalPlayerDirection,
		HearingProgress))
	{
		BuildHearingMesh(
			Snapshot.HearingRange > 0.0f ? Snapshot.HearingRange : HearingOuterRadius,
			LocalPlayerDirection,
			HearingProgress);
	}
	else if (HearingMesh)
	{
		HearingMesh->ClearAllMeshSections();
	}
}

void UTunaSweeperEnemySensorDebugComponent::EnsureMeshComponents()
{
	AActor* Owner = GetOwner();
	USceneComponent* RootComponent = Owner ? Owner->GetRootComponent() : nullptr;
	if (!Owner || !RootComponent)
	{
		return;
	}

	auto CreateMesh = [this, Owner, RootComponent](TObjectPtr<UProceduralMeshComponent>& Mesh, const FName Name, int32 SortPriority)
	{
		if (Mesh)
		{
			return;
		}
		Mesh = NewObject<UProceduralMeshComponent>(Owner, Name);
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
		Mesh->SetTranslucentSortPriority(SortPriority);
		Mesh->RegisterComponent();
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TunaSweeperEnemySensorDebug::MaterialPath))
		{
			for (int32 MaterialIndex = 0; MaterialIndex < 4; ++MaterialIndex)
			{
				Mesh->SetMaterial(MaterialIndex, Material);
			}
		}
	};

	CreateMesh(HearingMesh, TEXT("EnemyHearingSensorDebugMesh"), 20);
	CreateMesh(VisionMesh, TEXT("EnemyVisionSensorDebugMesh"), 21);
}

void UTunaSweeperEnemySensorDebugComponent::RebuildMeshes(float VisionRange, float VisionAngleDegrees)
{
	BuildVisionMesh(VisionRange, VisionAngleDegrees);
	CachedVisionRange = VisionRange;
	CachedVisionAngleDegrees = VisionAngleDegrees;
}

bool UTunaSweeperEnemySensorDebugComponent::TryGetPlayerHearingState(
	float HearingRange,
	FVector& OutLocalPlayerDirection,
	float& OutHearingProgress) const
{
	OutLocalPlayerDirection = FVector::ZeroVector;
	OutHearingProgress = 0.0f;

	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Owner || !PlayerPawn || HearingRange <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FVector WorldDirection = PlayerPawn->GetActorLocation() - Owner->GetActorLocation();
	WorldDirection.Z = 0.0f;
	const float PlayerDistance = WorldDirection.Length();
	if (PlayerDistance > HearingRange)
	{
		return false;
	}

	OutHearingProgress = FMath::Clamp(1.0f - PlayerDistance / HearingRange, 0.0f, 1.0f);
	if (OutHearingProgress < HearingActivationProgress)
	{
		return false;
	}

	if (PlayerDistance <= KINDA_SMALL_NUMBER)
	{
		WorldDirection = Owner->GetActorForwardVector();
	}
	OutLocalPlayerDirection = Owner->GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal2D();
	return !OutLocalPlayerDirection.IsNearlyZero();
}

void UTunaSweeperEnemySensorDebugComponent::BuildHearingMesh(
	float HearingRange,
	const FVector& LocalPlayerDirection,
	float HearingProgress)
{
	if (!HearingMesh)
	{
		return;
	}

	const FVector Direction = LocalPlayerDirection.GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		HearingMesh->ClearAllMeshSections();
		return;
	}

	const float PlayerDistance = FMath::Max(0.0f, HearingRange * (1.0f - HearingProgress));
	const float HalfArcRadians = FMath::DegreesToRadians(FMath::Clamp(HearingArcDegrees, 1.0f, 180.0f) * 0.5f);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	auto AddVertex = [&Vertices, &Normals, &UVs, &Colors, &Tangents](const FVector& Position, const FLinearColor& Color, const FVector2D& Uv)
	{
		Vertices.Add(Position);
		Normals.Add(FVector::UpVector);
		UVs.Add(Uv);
		Colors.Add(Color);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		return Vertices.Num() - 1;
	};

	// Keep the saturated gradient at a fixed world-space thickness instead of scaling it with player distance.
	const float FillInnerRadius = FMath::Max(0.0f, PlayerDistance - TunaSweeperEnemySensorDebug::HearingGradientBandWidth);
	const float FillBandWidth = PlayerDistance - FillInnerRadius;
	const float FillRadii[] = {
		FillInnerRadius,
		FillInnerRadius + FillBandWidth * 0.48f,
		FillInnerRadius + FillBandWidth * 0.78f,
		PlayerDistance
	};
	const FLinearColor FillColors[] = {
		FLinearColor(0.98f, 0.20f, 0.01f, 0.015f),
		FLinearColor(1.0f, 0.34f, 0.015f, 0.09f),
		FLinearColor(1.0f, 0.47f, 0.03f, 0.26f),
		FLinearColor(1.0f, 0.58f, 0.06f, 0.48f)
	};
	for (int32 RingIndex = 0; RingIndex < UE_ARRAY_COUNT(FillRadii); ++RingIndex)
	{
		const float RingRadius = FillRadii[RingIndex];
		for (int32 SegmentIndex = 0; SegmentIndex <= TunaSweeperEnemySensorDebug::HearingArcSegments; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / TunaSweeperEnemySensorDebug::HearingArcSegments;
			const float AngleRadians = FMath::Lerp(-HalfArcRadians, HalfArcRadians, Alpha);
			const FVector ArcDirection = Direction.RotateAngleAxis(FMath::RadiansToDegrees(AngleRadians), FVector::UpVector);
			AddVertex(
				ArcDirection * RingRadius + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 1.0f),
				FillColors[RingIndex],
				FVector2D(Alpha, static_cast<float>(RingIndex) / (UE_ARRAY_COUNT(FillRadii) - 1)));
		}
	}
	const int32 FillRingStride = TunaSweeperEnemySensorDebug::HearingArcSegments + 1;
	for (int32 RingIndex = 0; RingIndex < UE_ARRAY_COUNT(FillRadii) - 1; ++RingIndex)
	{
		for (int32 SegmentIndex = 0; SegmentIndex < TunaSweeperEnemySensorDebug::HearingArcSegments; ++SegmentIndex)
		{
			const int32 A = RingIndex * FillRingStride + SegmentIndex;
			const int32 B = A + 1;
			const int32 C = A + FillRingStride;
			const int32 D = C + 1;
			TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A, C, B);
			TunaSweeperEnemySensorDebug::AddTriangle(Triangles, B, C, D);
		}
	}

	for (int32 SegmentIndex = 0; SegmentIndex <= TunaSweeperEnemySensorDebug::HearingArcSegments; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / TunaSweeperEnemySensorDebug::HearingArcSegments;
		const float AngleRadians = FMath::Lerp(-HalfArcRadians, HalfArcRadians, Alpha);
		const FVector ArcDirection = Direction.RotateAngleAxis(FMath::RadiansToDegrees(AngleRadians), FVector::UpVector);
		AddVertex(
			ArcDirection * (PlayerDistance - TunaSweeperEnemySensorDebug::HearingArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f),
			FLinearColor(1.0f, 0.45f, 0.02f, 0.72f),
			FVector2D(Alpha, 0.0f));
		AddVertex(
			ArcDirection * (PlayerDistance + TunaSweeperEnemySensorDebug::HearingArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f),
			FLinearColor(1.0f, 0.72f, 0.12f, 0.96f),
			FVector2D(Alpha, 1.0f));
	}
	for (int32 SegmentIndex = 0; SegmentIndex < TunaSweeperEnemySensorDebug::HearingArcSegments; ++SegmentIndex)
	{
		const int32 A = SegmentIndex * 2;
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A, A + 2, A + 1);
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A + 1, A + 2, A + 3);
	}
	HearingMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Triangles.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Tangents.Reset();
	const FVector Tangent(Direction.Y, -Direction.X, 0.0f);
	const FVector GaugeStart = Direction * PlayerDistance + Tangent * TunaSweeperEnemySensorDebug::HearingGaugeOffset;
	auto AddQuad = [&AddVertex, &Triangles, &Direction](const FVector& Start, const FVector& End, float HalfHeight, float Height, const FLinearColor& StartColor, const FLinearColor& EndColor)
	{
		const FVector Vertical = FVector::UpVector * Height;
		const FVector Normal = Direction * HalfHeight;
		const int32 A = AddVertex(Start - Normal + Vertical, StartColor, FVector2D(0.0f, 0.0f));
		const int32 B = AddVertex(Start + Normal + Vertical, StartColor, FVector2D(0.0f, 1.0f));
		const int32 C = AddVertex(End - Normal + Vertical, EndColor, FVector2D(1.0f, 0.0f));
		const int32 D = AddVertex(End + Normal + Vertical, EndColor, FVector2D(1.0f, 1.0f));
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A, C, B);
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, B, C, D);
	};

	const FVector GaugeEnd = GaugeStart + Tangent * TunaSweeperEnemySensorDebug::HearingGaugeWidth;
	AddQuad(
		GaugeStart,
		GaugeEnd,
		TunaSweeperEnemySensorDebug::HearingGaugeHalfHeight,
		TunaSweeperEnemySensorDebug::MeshHeight + 3.0f,
		FLinearColor(0.18f, 0.07f, 0.01f, 0.52f),
		FLinearColor(0.26f, 0.10f, 0.01f, 0.52f));
	HearingMesh->CreateMeshSection_LinearColor(1, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Triangles.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Tangents.Reset();
	const FVector GaugeFillEnd = GaugeStart + Tangent * (TunaSweeperEnemySensorDebug::HearingGaugeWidth * FMath::Clamp(HearingProgress, 0.0f, 1.0f));
	AddQuad(
		GaugeStart,
		GaugeFillEnd,
		TunaSweeperEnemySensorDebug::HearingGaugeHalfHeight - 4.0f,
		TunaSweeperEnemySensorDebug::MeshHeight + 4.0f,
		FLinearColor(1.0f, 0.38f, 0.015f, 0.78f),
		FLinearColor(1.0f, 0.76f, 0.16f, 0.98f));
	const FVector ThresholdCenter = GaugeStart + Tangent * (TunaSweeperEnemySensorDebug::HearingGaugeWidth * 0.5f);
	AddQuad(
		ThresholdCenter - Tangent * TunaSweeperEnemySensorDebug::HearingGaugeTickHalfWidth,
		ThresholdCenter + Tangent * TunaSweeperEnemySensorDebug::HearingGaugeTickHalfWidth,
		TunaSweeperEnemySensorDebug::HearingGaugeHalfHeight + 6.0f,
		TunaSweeperEnemySensorDebug::MeshHeight + 5.0f,
		FLinearColor(1.0f, 0.84f, 0.38f, 1.0f),
		FLinearColor(1.0f, 0.84f, 0.38f, 1.0f));
	HearingMesh->CreateMeshSection_LinearColor(2, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void UTunaSweeperEnemySensorDebugComponent::BuildVisionMesh(float VisionRange, float VisionAngleDegrees)
{
	if (!VisionMesh)
	{
		return;
	}

	const float Range = FMath::Max(1.0f, VisionRange);
	const float HalfAngle = FMath::DegreesToRadians(FMath::Clamp(VisionAngleDegrees, 0.0f, 360.0f) * 0.5f);
	const int32 SegmentCount = FMath::Max(4, FMath::RoundToInt(TunaSweeperEnemySensorDebug::VisionSegments * FMath::Max(0.05f, VisionAngleDegrees / 360.0f)));
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	auto AddVertex = [&Vertices, &Normals, &UVs, &Colors, &Tangents](const FVector& Position, const FLinearColor& Color, const FVector2D& Uv)
	{
		Vertices.Add(Position);
		Normals.Add(FVector::UpVector);
		UVs.Add(Uv);
		Colors.Add(Color);
		Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		return Vertices.Num() - 1;
	};

	const int32 CenterIndex = AddVertex(FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 1.0f), FLinearColor(0.02f, 0.85f, 0.96f, 0.11f), FVector2D(0.5f, 0.0f));
	TArray<int32> OuterIndices;
	OuterIndices.Reserve(SegmentCount + 1);
	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / SegmentCount;
		const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
		OuterIndices.Add(AddVertex(
			FVector(FMath::Cos(Angle) * Range, FMath::Sin(Angle) * Range, TunaSweeperEnemySensorDebug::MeshHeight + 1.0f),
			FLinearColor(0.02f, 0.85f, 0.96f, 0.01f), FVector2D(Alpha, 1.0f)));
	}
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, CenterIndex, OuterIndices[SegmentIndex], OuterIndices[SegmentIndex + 1]);
	}
	VisionMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

	Vertices.Reset(); Triangles.Reset(); Normals.Reset(); UVs.Reset(); Colors.Reset(); Tangents.Reset();
	const float ArcHalfWidth = 10.0f;
	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / SegmentCount;
		const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		AddVertex(Direction * (Range - ArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f), FLinearColor(0.06f, 0.95f, 1.0f, 0.72f), FVector2D(Alpha, 0.0f));
		AddVertex(Direction * (Range + ArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f), FLinearColor(0.06f, 0.95f, 1.0f, 0.0f), FVector2D(Alpha, 1.0f));
	}
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const int32 A = SegmentIndex * 2;
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A, A + 2, A + 1);
		TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A + 1, A + 2, A + 3);
	}
	VisionMesh->CreateMeshSection_LinearColor(1, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void UTunaSweeperEnemySensorDebugComponent::SetMeshesVisible(bool bVisible) const
{
	for (UProceduralMeshComponent* Mesh : { HearingMesh.Get(), VisionMesh.Get() })
	{
		if (Mesh)
		{
			Mesh->SetVisibility(bVisible, true);
			Mesh->SetHiddenInGame(!bVisible, true);
		}
	}
}
