#include "Component/TunaSweeperQuestMarkerComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperQuestMarker
{
	constexpr int32 MedalSection = 0;
	constexpr int32 RimSection = 1;
	constexpr int32 FaceSection = 2;
	constexpr int32 ExclamationSection = 3;
	constexpr int32 HaloSection = 4;
	constexpr int32 CircleSegments = 48;

	struct FMeshSectionBuilder
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		int32 AddVertex(const FVector& Position, const FVector& Normal, const FVector2D& UV,
			const FLinearColor& Color = FLinearColor::White)
		{
			const int32 Index = Vertices.Add(Position);
			Normals.Add(Normal);
			UVs.Add(UV);
			Colors.Add(Color);
			Tangents.Emplace(FVector(1.0f, 0.0f, 0.0f), false);
			return Index;
		}

		void AddTriangle(int32 A, int32 B, int32 C)
		{
			Triangles.Add(A);
			Triangles.Add(B);
			Triangles.Add(C);
		}
	};

	void AppendDisc(FMeshSectionBuilder& Mesh, float Radius, float Z, bool bFaceForward)
	{
		const FVector Normal = bFaceForward ? FVector::UpVector : FVector::DownVector;
		const int32 Center = Mesh.AddVertex(FVector(0.0f, 0.0f, Z), Normal, FVector2D(0.5f, 0.5f));
		for (int32 Segment = 0; Segment <= CircleSegments; ++Segment)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(CircleSegments);
			const float X = FMath::Cos(Angle);
			const float Y = FMath::Sin(Angle);
			Mesh.AddVertex(FVector(X * Radius, Y * Radius, Z), Normal,
				FVector2D(0.5f + X * 0.5f, 0.5f + Y * 0.5f));
		}

		for (int32 Segment = 0; Segment < CircleSegments; ++Segment)
		{
			if (bFaceForward)
			{
				Mesh.AddTriangle(Center, Center + Segment + 1, Center + Segment + 2);
			}
			else
			{
				Mesh.AddTriangle(Center, Center + Segment + 2, Center + Segment + 1);
			}
		}
	}

	void AppendCylinder(FMeshSectionBuilder& Mesh, float Radius, float HalfThickness, const FVector& Offset = FVector::ZeroVector)
	{
		const int32 StartVertex = Mesh.Vertices.Num();
		AppendDisc(Mesh, Radius, HalfThickness, true);
		AppendDisc(Mesh, Radius, -HalfThickness, false);

		for (int32 Segment = 0; Segment < CircleSegments; ++Segment)
		{
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(CircleSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Segment + 1) / static_cast<float>(CircleSegments);
			const FVector Radial0(FMath::Cos(Angle0), FMath::Sin(Angle0), 0.0f);
			const FVector Radial1(FMath::Cos(Angle1), FMath::Sin(Angle1), 0.0f);
			const int32 Base = Mesh.Vertices.Num();
			Mesh.AddVertex(Radial0 * Radius + FVector(0.0f, 0.0f, -HalfThickness), Radial0, FVector2D(static_cast<float>(Segment) / CircleSegments, 0.0f));
			Mesh.AddVertex(Radial0 * Radius + FVector(0.0f, 0.0f, HalfThickness), Radial0, FVector2D(static_cast<float>(Segment) / CircleSegments, 1.0f));
			Mesh.AddVertex(Radial1 * Radius + FVector(0.0f, 0.0f, HalfThickness), Radial1, FVector2D(static_cast<float>(Segment + 1) / CircleSegments, 1.0f));
			Mesh.AddVertex(Radial1 * Radius + FVector(0.0f, 0.0f, -HalfThickness), Radial1, FVector2D(static_cast<float>(Segment + 1) / CircleSegments, 0.0f));
			Mesh.AddTriangle(Base, Base + 1, Base + 2);
			Mesh.AddTriangle(Base, Base + 2, Base + 3);
		}

		for (int32 VertexIndex = StartVertex; VertexIndex < Mesh.Vertices.Num(); ++VertexIndex)
		{
			Mesh.Vertices[VertexIndex] += Offset;
		}
	}

	void AppendAnnulus(FMeshSectionBuilder& Mesh, float InnerRadius, float OuterRadius, float Z)
	{
		for (int32 Segment = 0; Segment < CircleSegments; ++Segment)
		{
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(CircleSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Segment + 1) / static_cast<float>(CircleSegments);
			const FVector2D Unit0(FMath::Cos(Angle0), FMath::Sin(Angle0));
			const FVector2D Unit1(FMath::Cos(Angle1), FMath::Sin(Angle1));
			const int32 Base = Mesh.Vertices.Num();
			Mesh.AddVertex(FVector(Unit0.X * InnerRadius, Unit0.Y * InnerRadius, Z), FVector::UpVector, FVector2D(0.5f + Unit0.X * 0.5f, 0.5f + Unit0.Y * 0.5f));
			Mesh.AddVertex(FVector(Unit0.X * OuterRadius, Unit0.Y * OuterRadius, Z), FVector::UpVector, FVector2D(0.5f + Unit0.X * 0.5f, 0.5f + Unit0.Y * 0.5f));
			Mesh.AddVertex(FVector(Unit1.X * OuterRadius, Unit1.Y * OuterRadius, Z), FVector::UpVector, FVector2D(0.5f + Unit1.X * 0.5f, 0.5f + Unit1.Y * 0.5f));
			Mesh.AddVertex(FVector(Unit1.X * InnerRadius, Unit1.Y * InnerRadius, Z), FVector::UpVector, FVector2D(0.5f + Unit1.X * 0.5f, 0.5f + Unit1.Y * 0.5f));
			Mesh.AddTriangle(Base, Base + 1, Base + 2);
			Mesh.AddTriangle(Base, Base + 2, Base + 3);
		}
	}

	void AppendHalo(FMeshSectionBuilder& Mesh, float InnerRadius, float OuterRadius, float Z)
	{
		for (int32 Segment = 0; Segment < CircleSegments; ++Segment)
		{
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(CircleSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Segment + 1) / static_cast<float>(CircleSegments);
			const FVector2D Unit0(FMath::Cos(Angle0), FMath::Sin(Angle0));
			const FVector2D Unit1(FMath::Cos(Angle1), FMath::Sin(Angle1));
			const int32 Base = Mesh.Vertices.Num();
			Mesh.AddVertex(FVector(Unit0.X * InnerRadius, Unit0.Y * InnerRadius, Z), FVector::UpVector, FVector2D::ZeroVector, FLinearColor(1.0f, 1.0f, 1.0f, 0.42f));
			Mesh.AddVertex(FVector(Unit0.X * OuterRadius, Unit0.Y * OuterRadius, Z), FVector::UpVector, FVector2D::ZeroVector, FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
			Mesh.AddVertex(FVector(Unit1.X * OuterRadius, Unit1.Y * OuterRadius, Z), FVector::UpVector, FVector2D::ZeroVector, FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
			Mesh.AddVertex(FVector(Unit1.X * InnerRadius, Unit1.Y * InnerRadius, Z), FVector::UpVector, FVector2D::ZeroVector, FLinearColor(1.0f, 1.0f, 1.0f, 0.42f));
			Mesh.AddTriangle(Base, Base + 1, Base + 2);
			Mesh.AddTriangle(Base, Base + 2, Base + 3);
		}
	}

	void AppendBox(FMeshSectionBuilder& Mesh, const FVector& Min, const FVector& Max)
	{
		const FVector Corners[8] = {
			FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z),
			FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
			FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z),
			FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z)
		};
		const int32 Faces[6][4] = {
			{4, 5, 6, 7}, {1, 0, 3, 2}, {0, 4, 7, 3},
			{5, 1, 2, 6}, {3, 7, 6, 2}, {0, 1, 5, 4}
		};
		const FVector FaceNormals[6] = {
			FVector(0.0f, 0.0f, 1.0f), FVector(0.0f, 0.0f, -1.0f),
			FVector(-1.0f, 0.0f, 0.0f), FVector(1.0f, 0.0f, 0.0f),
			FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, -1.0f, 0.0f)
		};
		for (int32 Face = 0; Face < 6; ++Face)
		{
			const int32 Base = Mesh.Vertices.Num();
			Mesh.AddVertex(Corners[Faces[Face][0]], FaceNormals[Face], FVector2D(0.0f, 0.0f));
			Mesh.AddVertex(Corners[Faces[Face][1]], FaceNormals[Face], FVector2D(1.0f, 0.0f));
			Mesh.AddVertex(Corners[Faces[Face][2]], FaceNormals[Face], FVector2D(1.0f, 1.0f));
			Mesh.AddVertex(Corners[Faces[Face][3]], FaceNormals[Face], FVector2D(0.0f, 1.0f));
			Mesh.AddTriangle(Base, Base + 1, Base + 2);
			Mesh.AddTriangle(Base, Base + 2, Base + 3);
		}
	}
}

UTunaSweeperQuestMarkerComponent::UTunaSweeperQuestMarkerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetMobility(EComponentMobility::Movable);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	bUseAsyncCooking = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SurfaceMaterialFinder(
		TEXT("/Game/UI/WorldQuestMarker/M_QuestMarkerSurface.M_QuestMarkerSurface"));
	if (SurfaceMaterialFinder.Succeeded())
	{
		SurfaceMaterial = SurfaceMaterialFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HaloMaterialFinder(
		TEXT("/Game/UI/WorldQuestMarker/M_QuestMarkerHalo.M_QuestMarkerHalo"));
	if (HaloMaterialFinder.Succeeded())
	{
		HaloMaterial = HaloMaterialFinder.Object;
	}
}

void UTunaSweeperQuestMarkerComponent::OnRegister()
{
	Super::OnRegister();
	AnimationPhase = static_cast<float>(GetTypeHash(GetOwner() ? GetOwner()->GetFName() : GetFName()) % 1024) / 1024.0f * UE_TWO_PI;
	SetMarkerHeight(MarkerHeight);
	RebuildMarker();
}

void UTunaSweeperQuestMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsVisible() || DeltaTime <= 0.0f)
	{
		return;
	}

	AnimationTime += DeltaTime;
	const float BobOffset = BobbingAmplitude * FMath::Sin(AnimationTime * UE_TWO_PI * BobbingSpeed + AnimationPhase);
	const FVector CurrentLocation = GetRelativeLocation();
	SetRelativeLocation(FVector(CurrentLocation.X, CurrentLocation.Y, MarkerHeight + BobOffset));
	FaceLocalPlayerCamera();

	const float GlowMultiplier = 1.0f + GlowVariationAmount * FMath::Sin(
		AnimationTime * UE_TWO_PI * GlowVariationSpeed + AnimationPhase * 0.5f);
	UpdateMaterialParameters(GlowStrength * GlowMultiplier);
}

void UTunaSweeperQuestMarkerComponent::SetMarkerHeight(float InHeight)
{
	MarkerHeight = FMath::Max(0.0f, InHeight);
	const FVector CurrentLocation = GetRelativeLocation();
	SetRelativeLocation(FVector(CurrentLocation.X, CurrentLocation.Y, MarkerHeight));
}

void UTunaSweeperQuestMarkerComponent::RebuildMarker()
{
	ClearAllMeshSections();
	const float Radius = FMath::Max(0.5f, MarkerDiameter * 0.5f);
	const float HalfThickness = FMath::Max(0.1f, MarkerThickness * 0.5f);
	const float FaceRadius = Radius * FMath::Clamp(FaceDiameterRatio, 0.1f, 0.98f);
	const float FrontZ = HalfThickness + 0.08f;

	TunaSweeperQuestMarker::FMeshSectionBuilder Medal;
	TunaSweeperQuestMarker::AppendCylinder(Medal, Radius, HalfThickness);
	CreateMeshSection_LinearColor(TunaSweeperQuestMarker::MedalSection, Medal.Vertices, Medal.Triangles, Medal.Normals, Medal.UVs, Medal.Colors, Medal.Tangents, false);

	TunaSweeperQuestMarker::FMeshSectionBuilder Rim;
	TunaSweeperQuestMarker::AppendAnnulus(Rim, FaceRadius, Radius * 0.94f, FrontZ);
	CreateMeshSection_LinearColor(TunaSweeperQuestMarker::RimSection, Rim.Vertices, Rim.Triangles, Rim.Normals, Rim.UVs, Rim.Colors, Rim.Tangents, false);

	TunaSweeperQuestMarker::FMeshSectionBuilder Face;
	TunaSweeperQuestMarker::AppendDisc(Face, FaceRadius, FrontZ + 0.06f, true);
	CreateMeshSection_LinearColor(TunaSweeperQuestMarker::FaceSection, Face.Vertices, Face.Triangles, Face.Normals, Face.UVs, Face.Colors, Face.Tangents, false);

	const float SymbolFrontZ = FrontZ + 0.24f;
	const float SymbolDepth = FMath::Max(0.18f, MarkerThickness * 0.12f);
	const float BarHeight = FMath::Max(2.0f, ExclamationHeight * 0.70f);
	const float DotSize = FMath::Max(1.0f, ExclamationWidth * 0.92f);
	const float DotCenterY = -ExclamationHeight * 0.38f;
	const float BarCenterY = ExclamationHeight * 0.11f;
	TunaSweeperQuestMarker::FMeshSectionBuilder Symbol;
	TunaSweeperQuestMarker::AppendBox(Symbol,
		FVector(-ExclamationWidth * 0.5f, BarCenterY - BarHeight * 0.5f, SymbolFrontZ),
		FVector(ExclamationWidth * 0.5f, BarCenterY + BarHeight * 0.5f, SymbolFrontZ + SymbolDepth));
	TunaSweeperQuestMarker::AppendCylinder(Symbol, DotSize * 0.5f, SymbolDepth * 0.5f,
		FVector(0.0f, DotCenterY, SymbolFrontZ + SymbolDepth * 0.5f));
	CreateMeshSection_LinearColor(TunaSweeperQuestMarker::ExclamationSection, Symbol.Vertices, Symbol.Triangles, Symbol.Normals, Symbol.UVs, Symbol.Colors, Symbol.Tangents, false);

	TunaSweeperQuestMarker::FMeshSectionBuilder Halo;
	TunaSweeperQuestMarker::AppendHalo(Halo, Radius * 0.88f,
		Radius * FMath::Max(1.0f, HaloDiameterScale), -HalfThickness - 0.08f);
	CreateMeshSection_LinearColor(TunaSweeperQuestMarker::HaloSection, Halo.Vertices, Halo.Triangles, Halo.Normals, Halo.UVs, Halo.Colors, Halo.Tangents, false);

	CreateMarkerMaterials();
}

void UTunaSweeperQuestMarkerComponent::CreateMarkerMaterials()
{
	GoldMaterialInstance = SurfaceMaterial ? UMaterialInstanceDynamic::Create(SurfaceMaterial, this) : nullptr;
	RimMaterialInstance = SurfaceMaterial ? UMaterialInstanceDynamic::Create(SurfaceMaterial, this) : nullptr;
	ExclamationMaterialInstance = SurfaceMaterial ? UMaterialInstanceDynamic::Create(SurfaceMaterial, this) : nullptr;
	HaloMaterialInstance = HaloMaterial ? UMaterialInstanceDynamic::Create(HaloMaterial, this) : nullptr;

	SetMaterial(TunaSweeperQuestMarker::MedalSection, GoldMaterialInstance);
	SetMaterial(TunaSweeperQuestMarker::RimSection, RimMaterialInstance);
	SetMaterial(TunaSweeperQuestMarker::FaceSection, GoldMaterialInstance);
	SetMaterial(TunaSweeperQuestMarker::ExclamationSection, ExclamationMaterialInstance);
	SetMaterial(TunaSweeperQuestMarker::HaloSection, HaloMaterialInstance);
	UpdateMaterialParameters(GlowStrength);
}

void UTunaSweeperQuestMarkerComponent::UpdateMaterialParameters(float AnimatedGlowStrength)
{
	if (GoldMaterialInstance)
	{
		GoldMaterialInstance->SetVectorParameterValue(TEXT("MarkerColor"), GoldColor);
		GoldMaterialInstance->SetScalarParameterValue(TEXT("Metallic"), 0.58f);
		GoldMaterialInstance->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
		GoldMaterialInstance->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.14f * AnimatedGlowStrength);
	}
	if (RimMaterialInstance)
	{
		RimMaterialInstance->SetVectorParameterValue(TEXT("MarkerColor"), CreamRimColor);
		RimMaterialInstance->SetScalarParameterValue(TEXT("Metallic"), 0.08f);
		RimMaterialInstance->SetScalarParameterValue(TEXT("Roughness"), 0.42f);
		RimMaterialInstance->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.08f * AnimatedGlowStrength);
	}
	if (ExclamationMaterialInstance)
	{
		ExclamationMaterialInstance->SetVectorParameterValue(TEXT("MarkerColor"), ExclamationColor);
		ExclamationMaterialInstance->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
		ExclamationMaterialInstance->SetScalarParameterValue(TEXT("Roughness"), 0.62f);
		ExclamationMaterialInstance->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.0f);
	}
	if (HaloMaterialInstance)
	{
		HaloMaterialInstance->SetVectorParameterValue(TEXT("GlowColor"), GlowColor);
		HaloMaterialInstance->SetScalarParameterValue(TEXT("GlowStrength"), AnimatedGlowStrength);
		HaloMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), 0.55f);
	}
}

void UTunaSweeperQuestMarkerComponent::FaceLocalPlayerCamera()
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}
	const FVector ToCamera = PlayerController->PlayerCameraManager->GetCameraLocation() - GetComponentLocation();
	if (!ToCamera.IsNearlyZero())
	{
		SetWorldRotation(FRotationMatrix::MakeFromZY(ToCamera.GetSafeNormal(), FVector::UpVector).Rotator());
	}
}

#if WITH_EDITOR
void UTunaSweeperQuestMarkerComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SetMarkerHeight(MarkerHeight);
	RebuildMarker();
}
#endif
