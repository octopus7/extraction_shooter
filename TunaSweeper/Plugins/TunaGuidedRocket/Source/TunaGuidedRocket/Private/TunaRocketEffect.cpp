#include "TunaRocketEffect.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TunaRocketFlight.h"

ATunaRocketEffect::ATunaRocketEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    SetActorEnableCollision(false);
}

void ATunaRocketEffect::Initialize(bool bExplosion, float Duration, float Radius)
{
    if (!Particles.IsEmpty()) return;
    bBurst = bExplosion;
    Life = TunaRocketFlight::FiniteClamp(Duration, .5f, .01f, 5);
    Size = TunaRocketFlight::FiniteClamp(Radius, 100, .1f, 10000);
    SetLifeSpan(Life);
    if (!ParticleMesh || !ParticleMaterial) return;
    Material = UMaterialInstanceDynamic::Create(ParticleMaterial, this);
    Material->SetVectorParameterValue(TEXT("Color"), bBurst ? ExplosionColor : TrailColor);
    Material->SetScalarParameterValue(TEXT("Opacity"), bBurst ? .9f : .3f);
    Material->SetScalarParameterValue(TEXT("Glow"), bBurst ? 6.f : .4f);
    const int32 Count = bBurst ? 9 : 1;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        auto* Mesh = NewObject<UStaticMeshComponent>(this);
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetStaticMesh(ParticleMesh);
        Mesh->SetMaterial(0, Material);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->RegisterComponent();
        Particles.Add(Mesh);
    }
    Tick(0);
}

void ATunaRocketEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += FMath::Max(0.f, DeltaSeconds);
    const float T = FMath::Clamp(Age / Life, 0.f, 1.f);
    if (Material) Material->SetScalarParameterValue(TEXT("Opacity"), (bBurst ? .9f : .3f) * (1 - T) * (1 - T));
    for (int32 Index = 0; Index < Particles.Num(); ++Index)
    {
        // Particle mesh radius is one centimetre; scales are physical centimetres.
        const float Scale = bBurst ? (Index == 0 ? Size * (.08f + .35f * T) : Size * .06f * (1 - T)) : Size * (.5f + T);
        Particles[Index]->SetRelativeScale3D(FVector(Scale));
        if (bBurst && Index > 0)
        {
            const float Angle = (Index - 1) * UE_TWO_PI / 8;
            const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), Index % 2 ? .35f : -.2f);
            Particles[Index]->SetRelativeLocation(Direction.GetSafeNormal() * Size * T);
        }
        else if (!bBurst) Particles[Index]->SetRelativeLocation(FVector(0, 0, 20 * Age));
    }
    if (Age >= Life) Destroy();
}
