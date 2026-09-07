#include "Effect/TunaSweeperCombatPatternEffectActor.h"

#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperCombatPatternEffect
{
	constexpr int32 RingSegments = 48;
	const FLinearColor Ember(1.0f, 0.24f, 0.035f);
	const FLinearColor Hot(1.0f, 0.76f, 0.30f);
	const FLinearColor Cyan(0.05f, 0.83f, 1.0f);

	FLinearColor WithAlpha(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, FMath::Clamp(Alpha, 0.0f, 1.0f));
	}

	float Fade(float Time, float Start, float End)
	{
		const float A = FMath::Clamp((Time - Start) / FMath::Max(0.001f, End - Start), 0.0f, 1.0f);
		return 1.0f - A * A * (3.0f - 2.0f * A);
	}

	struct FGeometry
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		void Triangle(const FVector& A, const FVector& B, const FVector& C, const FLinearColor& Color)
		{
			const int32 First = Vertices.Num();
			Vertices.Append({A, B, C});
			Triangles.Append({First, First + 1, First + 2});
			const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Normals.Add(Normal);
				UVs.Add(FVector2D::ZeroVector);
				Colors.Add(Color);
				Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
			}
		}

		void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FLinearColor& Color)
		{
			Triangle(A, B, C, Color);
			Triangle(A, C, D, Color);
		}

		void Ring(float Radius, float Width, float Height, const FLinearColor& Color)
		{
			const float InnerRadius = FMath::Max(0.0f, Radius - Width);
			const FVector Up(0.0f, 0.0f, Height);
			for (int32 Index = 0; Index < RingSegments; ++Index)
			{
				const float AngleA = Index * 2.0f * UE_PI / RingSegments;
				const float AngleB = (Index + 1) * 2.0f * UE_PI / RingSegments;
				const FVector A(FMath::Cos(AngleA), FMath::Sin(AngleA), 0.0f);
				const FVector B(FMath::Cos(AngleB), FMath::Sin(AngleB), 0.0f);
				Quad(Up + A * Radius, Up + B * Radius, Up + B * InnerRadius, Up + A * InnerRadius, Color);
			}
		}

		/** A compact faceted volume, avoiding camera-facing cards that intersect the walking robots. */
		void Puff(const FVector& Center, const FVector& Size, float Rotation, const FLinearColor& Color)
		{
			constexpr int32 Sides = 6;
			const FVector Top = Center + FVector(0.0f, 0.0f, Size.Z);
			const FVector Bottom = Center - FVector(0.0f, 0.0f, Size.Z * 0.65f);
			for (int32 Index = 0; Index < Sides; ++Index)
			{
				const float A = Rotation + Index * 2.0f * UE_PI / Sides;
				const float B = Rotation + (Index + 1) * 2.0f * UE_PI / Sides;
				const FVector Left = Center + FVector(FMath::Cos(A) * Size.X, FMath::Sin(A) * Size.Y, 0.0f);
				const FVector Right = Center + FVector(FMath::Cos(B) * Size.X, FMath::Sin(B) * Size.Y, 0.0f);
				const float Shade = 0.80f + 0.20f * FMath::Cos(A - 0.6f);
				const FLinearColor FaceColor(Color.R * Shade, Color.G * Shade, Color.B * Shade, Color.A);
				Triangle(Top, Left, Right, FaceColor);
				Triangle(Bottom, Right, Left, FaceColor);
			}
		}

		/** Crossed tapered ribbons stay visible from the game's elevated camera. */
		void Streak(const FVector& Center, const FVector& Direction, float Length, float Width, const FLinearColor& Color)
		{
			const FVector Axis = Direction.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
			FVector Side = FVector::CrossProduct(Axis, FVector::UpVector).GetSafeNormal();
			if (Side.IsNearlyZero()) { Side = FVector::RightVector; }
			const FVector OtherSide = FVector::CrossProduct(Axis, Side).GetSafeNormal();
			const FVector Tail = Center - Axis * Length;
			const FVector Tip = Center + Axis * Width;
			Triangle(Tip, Center + Side * Width, Tail, Color);
			Triangle(Tip, Tail, Center - Side * Width, Color);
			Triangle(Tip, Center + OtherSide * Width, Tail, Color);
			Triangle(Tip, Tail, Center - OtherSide * Width, Color);
		}

		void Commit(UProceduralMeshComponent* Mesh, int32 Section) const
		{
			if (Vertices.IsEmpty()) { Mesh->ClearMeshSection(Section); return; }
			const FProcMeshSection* Existing = Mesh->GetProcMeshSection(Section);
			if (Existing && Existing->ProcVertexBuffer.Num() == Vertices.Num())
			{
				Mesh->UpdateMeshSection_LinearColor(Section, Vertices, Normals, UVs, Colors, Tangents);
			}
			else
			{
				Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
			}
		}
	};
}

ATunaSweeperCombatPatternEffectActor::ATunaSweeperCombatPatternEffectActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	bReplicates = false;
	InitialLifeSpan = 2.0f;
	SetActorEnableCollision(false);
	EffectMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("EffectMesh"));
	SetRootComponent(EffectMesh);
	EffectMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EffectMesh->SetGenerateOverlapEvents(false);
	EffectMesh->SetCastShadow(false);
	EffectMesh->SetReceivesDecals(false);
	EffectMesh->SetCanEverAffectNavigation(false);
	EffectMesh->SetTranslucentSortPriority(20);
	EffectMesh->SetCullDistance(16000.0f);
	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubjectComponent"));

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> GlowAsset(
		TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Glow.M_CP_Glow"));
	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SmokeAsset(
		TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Smoke.M_CP_Smoke"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackAsset(
		TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive"));
	GlowMaterial = GlowAsset.Get() ? GlowAsset.Get() : FallbackAsset.Object.Get();
	SmokeMaterial = SmokeAsset.Get() ? SmokeAsset.Get() : FallbackAsset.Object.Get();
}

ATunaSweeperCombatPatternEffectActor* ATunaSweeperCombatPatternEffectActor::Spawn(UWorld* World,
	ETunaSweeperCombatPatternEffect Kind, const FVector& Location, float Radius, const FVector& Direction, AActor* Owner)
{
	if (!World || World->GetNetMode() == NM_DedicatedServer || Location.ContainsNaN()) { return nullptr; }
	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATunaSweeperCombatPatternEffectActor* Effect = World->SpawnActor<ATunaSweeperCombatPatternEffectActor>(
		StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Effect) { Effect->InitializeEffect(Kind, Radius, Direction); }
	return Effect;
}

void ATunaSweeperCombatPatternEffectActor::InitializeEffect(ETunaSweeperCombatPatternEffect Kind,
	float Radius, FVector Direction)
{
	EffectKind = Kind;
	EffectRadius = FMath::IsFinite(Radius) ? FMath::Clamp(Radius, 1.0f, 1200.0f) : 100.0f;
	EffectDirection = Direction.ContainsNaN() ? FVector::UpVector :
		GetActorTransform().InverseTransformVectorNoScale(Direction).GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	Elapsed = 0.0f;
	switch (EffectKind)
	{
	case ETunaSweeperCombatPatternEffect::Summon: Duration = 0.85f; break;
	case ETunaSweeperCombatPatternEffect::MissileLaunch: Duration = 0.42f; break;
	case ETunaSweeperCombatPatternEffect::MissileTrail: Duration = 0.24f; break;
	case ETunaSweeperCombatPatternEffect::Impact: Duration = 0.95f; break;
	case ETunaSweeperCombatPatternEffect::ChargeTrail: Duration = 0.34f; break;
	case ETunaSweeperCombatPatternEffect::RobotUnfold: Duration = 0.60f; break;
	case ETunaSweeperCombatPatternEffect::RobotDeath: Duration = 0.78f; break;
	default: Duration = 0.5f; break;
	}
	if (GlowMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(GlowMaterial, this);
		Material->SetScalarParameterValue(TEXT("Intensity"), 2.0f);
		EffectMesh->SetMaterial(0, Material);
	}
	if (SmokeMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SmokeMaterial, this);
		Material->SetScalarParameterValue(TEXT("Intensity"), 1.0f);
		EffectMesh->SetMaterial(1, Material);
	}
	SeedParticles();
	bInitialized = true;
	SetActorTickEnabled(true);
	SetLifeSpan(Duration + 0.05f);
	UpdateGeometry();
}

void ATunaSweeperCombatPatternEffectActor::SeedParticles()
{
	const bool bTrail = EffectKind == ETunaSweeperCombatPatternEffect::MissileTrail ||
		EffectKind == ETunaSweeperCombatPatternEffect::ChargeTrail;
	const bool bCool = EffectKind == ETunaSweeperCombatPatternEffect::Summon ||
		EffectKind == ETunaSweeperCombatPatternEffect::RobotUnfold;
	const int32 SparkCount = bTrail ? 3 : (bCool ? 10 : 16);
	const int32 SmokeCount = bTrail ? 3 : (bCool ? 5 : 9);
	Sparks.Reset(SparkCount);
	Smoke.Reset(SmokeCount);
	FRandomStream Random(static_cast<int32>(GetUniqueID()));
	for (int32 Index = 0; Index < SparkCount; ++Index)
	{
		const float Angle = (Index + Random.FRandRange(-0.25f, 0.25f)) * 2.0f * UE_PI / SparkCount;
		const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		FParticle& Particle = Sparks.AddDefaulted_GetRef();
		Particle.Origin = Radial * EffectRadius * Random.FRandRange(0.02f, bCool ? 0.45f : 0.12f);
		Particle.Origin.Z = EffectRadius * 0.06f;
		Particle.Velocity = (Radial * Random.FRandRange(0.5f, 1.1f) +
			FVector::UpVector * Random.FRandRange(0.3f, 1.2f)) * EffectRadius * (bTrail ? 1.1f : 2.2f);
		if (bTrail) { Particle.Velocity += EffectDirection * EffectRadius * 1.5f; }
		Particle.Size = EffectRadius * Random.FRandRange(0.008f, 0.018f);
		Particle.Phase = Random.FRandRange(0.0f, 1.0f);
	}
	for (int32 Index = 0; Index < SmokeCount; ++Index)
	{
		const float Angle = (Index + Random.FRand()) * 2.0f * UE_PI / SmokeCount;
		const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		FParticle& Particle = Smoke.AddDefaulted_GetRef();
		Particle.Origin = Radial * EffectRadius * Random.FRandRange(0.05f, 0.25f);
		Particle.Origin.Z = EffectRadius * 0.08f;
		Particle.Velocity = Radial * EffectRadius * Random.FRandRange(0.45f, 0.80f) +
			FVector::UpVector * EffectRadius * Random.FRandRange(0.16f, 0.50f);
		if (bTrail) { Particle.Velocity += EffectDirection * EffectRadius * 0.65f; }
		Particle.Size = EffectRadius * Random.FRandRange(0.09f, 0.20f);
		Particle.Phase = Random.FRandRange(0.0f, 2.0f * UE_PI);
	}
}

void ATunaSweeperCombatPatternEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInitialized || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) { return; }
	// Game delta and the actor life span both respect pause/time dilation.
	Elapsed += DeltaSeconds;
	if (Elapsed >= Duration) { Destroy(); return; }
	UpdateGeometry();
}

void ATunaSweeperCombatPatternEffectActor::UpdateGeometry()
{
	using namespace TunaSweeperCombatPatternEffect;
	TunaSweeperCombatPatternEffect::FGeometry Glow;
	TunaSweeperCombatPatternEffect::FGeometry Dust;
	const float Time = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	const float FadeOut = Fade(Time, 0.18f, 1.0f);
	const bool bMissileTrail = EffectKind == ETunaSweeperCombatPatternEffect::MissileTrail;
	const bool bChargeTrail = EffectKind == ETunaSweeperCombatPatternEffect::ChargeTrail;
	const bool bTrail = bMissileTrail || bChargeTrail;
	const bool bCool = EffectKind == ETunaSweeperCombatPatternEffect::Summon ||
		EffectKind == ETunaSweeperCombatPatternEffect::RobotUnfold;
	const bool bLaunch = EffectKind == ETunaSweeperCombatPatternEffect::MissileLaunch;
	const FLinearColor Accent = bCool ? Cyan : Ember;

	if (!bTrail && !bLaunch)
	{
		const float RingTravel = 1.0f - FMath::Square(1.0f - FMath::Min(1.0f, Time * 2.0f));
		const float RingRadius = EffectRadius * FMath::Lerp(bCool ? 0.55f : 0.18f, 1.0f, RingTravel);
		Glow.Ring(RingRadius, EffectRadius * FMath::Lerp(0.06f, 0.016f, RingTravel), 5.0f,
			WithAlpha(Accent, Fade(Time, 0.08f, 0.65f) * 0.78f));
		Glow.Ring(RingRadius * 0.84f, EffectRadius * 0.008f, 6.0f,
			WithAlpha(bCool ? Cyan : Hot, Fade(Time, 0.0f, 0.42f) * 0.4f));
		if (bCool)
		{
			// A broken rising collar gives deployment a mechanical scanner silhouette.
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const float Angle = Index * 2.0f * UE_PI / 8.0f + Time * 0.3f;
				const FVector Center(FMath::Cos(Angle) * RingRadius * 0.75f,
					FMath::Sin(Angle) * RingRadius * 0.75f, EffectRadius * (0.12f + Time * 0.65f));
				Glow.Streak(Center, FVector::UpVector, EffectRadius * 0.14f, EffectRadius * 0.011f,
					WithAlpha(Cyan, FadeOut * 0.70f));
			}
		}
	}

	if (!bCool && !bChargeTrail)
	{
		const float CoreFade = Fade(Time, 0.02f, bMissileTrail ? 0.80f : 0.38f);
		const float CoreSize = EffectRadius * (bMissileTrail ? 0.38f : 0.30f) * (1.0f + Time * 0.4f);
		const FVector Center = bLaunch || bMissileTrail ? EffectDirection * CoreSize * 0.25f :
			FVector(0.0f, 0.0f, CoreSize * 0.8f);
		Glow.Puff(Center, FVector(CoreSize, CoreSize, CoreSize * (bLaunch ? 1.4f : 0.9f)), Time,
			WithAlpha(Ember, CoreFade * 0.65f));
		Glow.Streak(Center, bLaunch || bMissileTrail ? -EffectDirection : FVector::UpVector,
			CoreSize * (bMissileTrail ? 4.8f : (bLaunch ? 2.6f : 0.8f)), CoreSize * 0.32f,
			WithAlpha(Hot, CoreFade * 0.90f));
	}

	for (const FParticle& Particle : Sparks)
	{
		const float SparkTime = FMath::Min(Elapsed, Duration * 0.75f);
		FVector Position = Particle.Origin + Particle.Velocity * SparkTime;
		FVector Velocity = Particle.Velocity;
		if (!bCool && !bMissileTrail)
		{
			const float Gravity = EffectRadius * 2.8f;
			Position.Z -= 0.5f * Gravity * SparkTime * SparkTime;
			Velocity.Z -= Gravity * SparkTime;
			Position.Z = FMath::Max(3.0f, Position.Z);
		}
		Glow.Streak(Position, Velocity, Particle.Size * (bCool ? 4.0f : 8.0f), Particle.Size,
			WithAlpha(bCool ? Cyan : Hot, Fade(Time, 0.12f + Particle.Phase * 0.18f, 0.76f)));
	}

	for (const FParticle& Particle : Smoke)
	{
		const FVector Position = Particle.Origin + Particle.Velocity * Elapsed;
		const float Growth = Particle.Size * FMath::Lerp(0.45f, 1.65f, Time);
		// Cool deployment steam is sparse; impact smoke clears quickly enough to keep feet readable.
		const FLinearColor SmokeColor = bCool ? FLinearColor(0.25f, 0.35f, 0.38f) :
			(bChargeTrail ? FLinearColor(0.22f, 0.17f, 0.12f) : FLinearColor(0.09f, 0.075f, 0.06f));
		const float Opacity = (bTrail ? 0.25f : (bCool ? 0.20f : 0.43f)) *
			FMath::Min(1.0f, 0.5f + Time * 8.0f) * FadeOut;
		Dust.Puff(Position, FVector(Growth, Growth * 0.85f, Growth * (bChargeTrail ? 0.45f : 0.8f)),
			Particle.Phase + Time * 0.45f, WithAlpha(SmokeColor, Opacity));
	}
	Glow.Commit(EffectMesh, 0);
	Dust.Commit(EffectMesh, 1);
}

void ATunaSweeperCombatPatternEffectActor::PreviewAtNormalizedAge(float NormalizedAge)
{
	if (!bInitialized || !FMath::IsFinite(NormalizedAge)) { return; }
	const float RuntimeElapsed = Elapsed;
	Elapsed = FMath::Clamp(NormalizedAge, 0.0f, 1.0f) * Duration;
	UpdateGeometry();
	Elapsed = RuntimeElapsed;
}
