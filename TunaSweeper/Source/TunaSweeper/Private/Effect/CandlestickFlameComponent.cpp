#include "Effect/CandlestickFlameComponent.h"

#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 FlameSection = 0;
	constexpr int32 SparkSection = 1;
	constexpr int32 FlameSides = 10;

	const FVector EmberGravity(0, 0, -24);

	struct FFlameProfileRing
	{
		float HeightFraction;
		float RadiusFraction;
		float Alpha;
	};

	constexpr FFlameProfileRing FlameProfile[] = {
		{0.00f, 0.14f, 0.24f},
		{0.08f, 0.58f, 0.52f},
		{0.23f, 0.91f, 0.78f},
		{0.39f, 1.00f, 0.76f},
		{0.57f, 0.80f, 0.60f},
		{0.74f, 0.54f, 0.46f},
		{0.89f, 0.25f, 0.30f},
		{1.00f, 0.02f, 0.00f}
	};

	struct FCandleMesh
	{
		TArray<FVector> Vertices;
		TArray<FLinearColor> Colors;
		TArray<int32> Triangles;

		void AddVertex(const FVector& Position, FLinearColor Color, float Alpha)
		{
			Vertices.Add(Position);
			Color.A = Alpha;
			Colors.Add(Color);
		}

		void AddFlame(float Height, float Radius, float Offset, const FVector& Lean,
			const FLinearColor& Color, float Alpha)
		{
			const int32 Start = Vertices.Num();
			for (const auto& Ring : FlameProfile)
			{
				const float Width = Radius * Ring.RadiusFraction;
				const FVector Center = Lean * FMath::Square(Ring.HeightFraction) +
					FVector(0, 0, Offset + Ring.HeightFraction * Height);
				for (int32 Side = 0; Side < FlameSides; ++Side)
				{
					const float Angle = Side * UE_TWO_PI / FlameSides;
					AddVertex(Center + FVector(Width * FMath::Cos(Angle), Width * FMath::Sin(Angle), 0),
						Color, Ring.Alpha * Alpha);
				}
			}
			for (int32 Ring = 1; Ring < UE_ARRAY_COUNT(FlameProfile); ++Ring)
			{
				for (int32 Side = 0; Side < FlameSides; ++Side)
				{
					const int32 Lower = Start + (Ring - 1) * FlameSides;
					const int32 Upper = Start + Ring * FlameSides;
					const int32 Next = (Side + 1) % FlameSides;
					Triangles.Append({Lower + Side, Upper + Side, Upper + Next, Lower + Side, Upper + Next, Lower + Next});
				}
			}
		}

		void AddSpark(const FVector& Center, const FVector& Direction, float Alpha)
		{
			const FVector Axis = Direction.GetSafeNormal();
			FVector Side = FVector::CrossProduct(Axis, FVector::UpVector).GetSafeNormal();
			if (Side.IsNearlyZero()) Side = FVector::RightVector;
			const FVector Across = FVector::CrossProduct(Axis, Side);
			const FLinearColor HeadColor(1.0f, 0.70f, 0.22f);
			const int32 Start = Vertices.Num();
			AddVertex(Center + Axis * 0.50f, HeadColor, Alpha);
			AddVertex(Center - Axis * 2.8f, FLinearColor(1.0f, 0.20f, 0.015f), 0.0f);
			for (const FVector& Edge : {Side, Across, -Side, -Across})
			{
				AddVertex(Center + Edge * 0.42f, HeadColor, Alpha * 0.78f);
			}
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const int32 Rim = Start + 2 + Index;
				const int32 Next = Start + 2 + (Index + 1) % 4;
				Triangles.Append({Start, Next, Rim, Start + 1, Rim, Next});
			}
		}

		void Submit(UProceduralMeshComponent& Mesh, int32 Section) const
		{
			const FProcMeshSection* Existing = Mesh.GetProcMeshSection(Section);
			if (Vertices.IsEmpty())
			{
				if (Existing && !Existing->ProcVertexBuffer.IsEmpty()) Mesh.ClearMeshSection(Section);
			}
			else if (Existing && Existing->ProcVertexBuffer.Num() == Vertices.Num())
			{
				Mesh.UpdateMeshSection_LinearColor(Section, Vertices, {}, {}, Colors, {}, false);
			}
			else
			{
				Mesh.CreateMeshSection_LinearColor(Section, Vertices, Triangles, {}, {}, Colors, {}, false, false);
			}
		}
	};
}

UCandlestickFlameComponent::UCandlestickFlameComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	SetMobility(EComponentMobility::Movable);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);
	SetReceivesDecals(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Glow(
		TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Glow.M_CP_Glow"));
	GlowMaterial = Glow.Object;
}

void UCandlestickFlameComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetWorld()->GetNetMode() == NM_DedicatedServer || !GlowMaterial) return;

	auto* Glow = UMaterialInstanceDynamic::Create(GlowMaterial, this);
	Glow->SetFlags(RF_Transient | RF_DuplicateTransient);
	Glow->SetScalarParameterValue(TEXT("Intensity"), 1.25f);
	SetMaterial(FlameSection, Glow);
	SetMaterial(SparkSection, Glow);

	CandleLight = GetOwner()->FindComponentByClass<UPointLightComponent>();
	if (CandleLight) BaseLightIntensity = CandleLight->Intensity;

	Random.GenerateNewSeed();
	AnimationTime = 0.0f;
	LastSparkBurstTime = -1.0f;
	NextSparkBurstTime = static_cast<float>(Random.FRandRange(1.8, 3.4));
	UpdateVisuals();
	SetComponentTickEnabled(true);
}

void UCandlestickFlameComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetComponentTickEnabled(false);
	CandleLight = nullptr;
	Sparks.Reset();
	ClearAllMeshSections();
	Super::EndPlay(EndPlayReason);
}

void UCandlestickFlameComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AnimationTime += DeltaTime;
	UpdateVisuals();
}

void UCandlestickFlameComponent::EmitSparks(float FlameHeight)
{
	LastSparkBurstTime = AnimationTime;
	NextSparkBurstTime = AnimationTime + static_cast<float>(Random.FRandRange(3.0, 6.2));
	const int32 Count = Random.RandRange(2, 4);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Angle = Random.FRandRange(0.0, UE_DOUBLE_TWO_PI);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		Sparks.Add({
			Direction * Random.FRandRange(0.0f, 0.8f) + FVector(0, 0, FlameHeight * 0.85f),
			Direction * Random.FRandRange(10.0f, 26.0f) + FVector(0, 0, Random.FRandRange(28.0f, 43.0f)),
			AnimationTime + static_cast<float>(Random.FRandRange(0.0, 0.12)),
			static_cast<float>(Random.FRandRange(0.23, 0.40))});
	}
}

void UCandlestickFlameComponent::UpdateVisuals()
{
	const float Flicker = FMath::Sin(AnimationTime * 14.0f) * 0.08f +
		FMath::Sin(AnimationTime * 31.0f) * 0.07f + FMath::Sin(AnimationTime * 53.0f) * 0.03f;
	const float Crackle = FMath::Clamp(1.0f - (AnimationTime - LastSparkBurstTime) * 6.0f, 0.0f, 1.0f);
	const float Height = 6.4f * (1.0f + Flicker + Crackle * 0.08f);
	const FVector Lean(0.72f * FMath::Sin(AnimationTime * 4.0f), 0.61f * FMath::Sin(AnimationTime * 6.3f), 0.0f);

	FCandleMesh Flame;
	Flame.AddFlame(Height, 1.55f * (1.0f - Flicker * 0.5f), 0.0f, Lean,
		FLinearColor(1.0f, 0.24f, 0.045f), 0.75f);
	Flame.AddFlame(Height * 0.71f, 0.85f, 0.21f, Lean * 0.60f,
		FLinearColor(1.0f, 0.81f, 0.36f), 0.88f);
	Flame.Submit(*this, FlameSection);

	if (AnimationTime >= NextSparkBurstTime) EmitSparks(Height);
	Sparks.RemoveAll([this](const FSpark& Spark)
	{
		return AnimationTime - Spark.StartTime >= Spark.Lifetime;
	});
	FCandleMesh Embers;
	for (const FSpark& Spark : Sparks)
	{
		const float Age = AnimationTime - Spark.StartTime;
		if (Age < 0.0f) continue;
		const float Remaining = 1.0f - Age / Spark.Lifetime;
		const FVector Center = Spark.Origin + (Spark.Velocity + EmberGravity * (0.5f * Age)) * Age;
		Embers.AddSpark(Center, Spark.Velocity + EmberGravity * Age, FMath::Square(Remaining));
	}
	Embers.Submit(*this, SparkSection);
	if (CandleLight)
	{
		CandleLight->SetIntensity(BaseLightIntensity * (1.0f + 0.5f * Flicker + Crackle * 0.05f));
	}
}
