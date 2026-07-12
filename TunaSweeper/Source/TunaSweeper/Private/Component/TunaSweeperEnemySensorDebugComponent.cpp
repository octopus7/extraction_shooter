#include "Component/TunaSweeperEnemySensorDebugComponent.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace TunaSweeperEnemySensorDebug
{
	constexpr float MeshHeight = 4.0f;
	constexpr int32 HearingSegments = 72;
	constexpr int32 VisionSegments = 48;
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
			Mesh->SetMaterial(0, Material);
		}
	};

	CreateMesh(HearingMesh, TEXT("EnemyHearingSensorDebugMesh"), 20);
	CreateMesh(VisionMesh, TEXT("EnemyVisionSensorDebugMesh"), 21);
}

void UTunaSweeperEnemySensorDebugComponent::RebuildMeshes(float VisionRange, float VisionAngleDegrees)
{
	BuildHearingMesh();
	BuildVisionMesh(VisionRange, VisionAngleDegrees);
	CachedVisionRange = VisionRange;
	CachedVisionAngleDegrees = VisionAngleDegrees;
}

void UTunaSweeperEnemySensorDebugComponent::BuildHearingMesh()
{
	if (!HearingMesh)
	{
		return;
	}

	const float OuterRadius = FMath::Max(1.0f, HearingOuterRadius);
	const float Radii[] = { 0.0f, OuterRadius * 0.31f, OuterRadius / 3.0f, OuterRadius * 0.64f, OuterRadius * 2.0f / 3.0f, OuterRadius * 0.97f, OuterRadius };
	const float Alphas[] = { 0.045f, 0.12f, 0.025f, 0.12f, 0.025f, 0.11f, 0.0f };
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(UE_ARRAY_COUNT(Radii) * (TunaSweeperEnemySensorDebug::HearingSegments + 1));

	for (int32 RingIndex = 0; RingIndex < UE_ARRAY_COUNT(Radii); ++RingIndex)
	{
		for (int32 SegmentIndex = 0; SegmentIndex <= TunaSweeperEnemySensorDebug::HearingSegments; ++SegmentIndex)
		{
			const float Angle = (2.0f * PI * SegmentIndex) / TunaSweeperEnemySensorDebug::HearingSegments;
			Vertices.Add(FVector(FMath::Cos(Angle) * Radii[RingIndex], FMath::Sin(Angle) * Radii[RingIndex], TunaSweeperEnemySensorDebug::MeshHeight));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(static_cast<float>(SegmentIndex) / TunaSweeperEnemySensorDebug::HearingSegments, static_cast<float>(RingIndex) / (UE_ARRAY_COUNT(Radii) - 1)));
			Colors.Add(FLinearColor(1.0f, 0.27f, 0.02f, Alphas[RingIndex]));
			Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
		}
	}

	const int32 RingStride = TunaSweeperEnemySensorDebug::HearingSegments + 1;
	for (int32 RingIndex = 0; RingIndex < UE_ARRAY_COUNT(Radii) - 1; ++RingIndex)
	{
		for (int32 SegmentIndex = 0; SegmentIndex < TunaSweeperEnemySensorDebug::HearingSegments; ++SegmentIndex)
		{
			const int32 A = RingIndex * RingStride + SegmentIndex;
			const int32 B = A + 1;
			const int32 C = A + RingStride;
			const int32 D = C + 1;
			TunaSweeperEnemySensorDebug::AddTriangle(Triangles, A, C, B);
			TunaSweeperEnemySensorDebug::AddTriangle(Triangles, B, C, D);
		}
	}

	HearingMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
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

	const int32 CenterIndex = AddVertex(FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 1.0f), FLinearColor(1.0f, 0.27f, 0.02f, 0.11f), FVector2D(0.5f, 0.0f));
	TArray<int32> OuterIndices;
	OuterIndices.Reserve(SegmentCount + 1);
	for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / SegmentCount;
		const float Angle = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
		OuterIndices.Add(AddVertex(
			FVector(FMath::Cos(Angle) * Range, FMath::Sin(Angle) * Range, TunaSweeperEnemySensorDebug::MeshHeight + 1.0f),
			FLinearColor(1.0f, 0.27f, 0.02f, 0.01f), FVector2D(Alpha, 1.0f)));
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
		AddVertex(Direction * (Range - ArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f), FLinearColor(1.0f, 0.47f, 0.08f, 0.72f), FVector2D(Alpha, 0.0f));
		AddVertex(Direction * (Range + ArcHalfWidth) + FVector(0.0f, 0.0f, TunaSweeperEnemySensorDebug::MeshHeight + 2.0f), FLinearColor(1.0f, 0.47f, 0.08f, 0.0f), FVector2D(Alpha, 1.0f));
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
