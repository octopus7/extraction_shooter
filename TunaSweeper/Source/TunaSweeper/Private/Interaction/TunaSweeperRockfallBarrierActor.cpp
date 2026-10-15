#include "Interaction/TunaSweeperRockfallBarrierActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Effect/TunaSweeperLocalExplosionEffectActor.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"
#include "TimerManager.h"

ATunaSweeperRockfallBarrierActor::ATunaSweeperRockfallBarrierActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .1f;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")); SetRootComponent(SceneRoot);
    CentralRocks = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CentralRocks"));
    LeftCliff = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftCliff"));
    RightCliff = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightCliff"));
    for (auto* Mesh : {CentralRocks.Get(), LeftCliff.Get(), RightCliff.Get()})
    {
        Mesh->SetupAttachment(SceneRoot); Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->SetCanEverAffectNavigation(true); Mesh->SetReceivesDecals(false);
    }
    ExplosiveBottle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplosiveBottle"));
    ExplosiveBottle->SetupAttachment(SceneRoot); ExplosiveBottle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExplosiveBottle->SetRelativeLocation(FVector(-150,0,70));
    BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
    BlockingCollision->SetupAttachment(SceneRoot); BlockingCollision->SetBoxExtent(FVector(140,400,200));
    BlockingCollision->SetRelativeLocation(FVector(0,0,200)); BlockingCollision->SetCollisionProfileName(TEXT("BlockAll"));
    BlockingCollision->SetCanEverAffectNavigation(true);
    Interactable = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
    Interactable->SetupAttachment(SceneRoot); Interactable->SetRelativeLocation(FVector(-170,0,100));
    CountdownText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CountdownText"));
    CountdownText->SetupAttachment(SceneRoot); CountdownText->SetRelativeLocation(FVector(-170,0,230));
    CountdownText->SetHorizontalAlignment(EHTA_Center); CountdownText->SetWorldSize(40);
    CountdownText->SetTextRenderColor(FColor(255,190,40)); CountdownText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CountdownText->SetVisibility(false);
    WarningDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("WarningDecal"));
    WarningDecal->SetupAttachment(SceneRoot); WarningDecal->SetRelativeRotation(FRotator(-90,0,0));
    WarningDecal->SetRelativeLocation(FVector(0,0,5)); WarningDecal->SetVisibility(false);
    WarningDecal->DecalSize = FVector(30,ExplosionRadiusCm,ExplosionRadiusCm);
    WarningMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Interaction/Rockfall/M_RockfallDanger.M_RockfallDanger")));
}

void ATunaSweeperRockfallBarrierActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (GetWorld() && !GetWorld()->IsGameWorld()) { bCleared = bPreviewCleared; bArmed = false; ApplyPresentation(); }
}
void ATunaSweeperRockfallBarrierActor::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshSavedState);
        Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshSavedState);
        Game->OnLanguageChanged.AddUObject(this, &ThisClass::ApplyPresentation);
    }
    RefreshSavedState();
}
void ATunaSweeperRockfallBarrierActor::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(DetonationTimer);
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
    {
        Game->OnWorldProgressChanged.RemoveAll(this); Game->OnInventoryStateChanged.RemoveAll(this);
        Game->OnLanguageChanged.RemoveAll(this);
    }
    Super::EndPlay(Reason);
}
void ATunaSweeperRockfallBarrierActor::RefreshSavedState()
{
    if (bMutating) return;
    FTunaSweeperWorldProgressSaveData State;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    const bool bHasState = Game && !ProgressObjectId.IsNone() && Game->TryGetWorldProgressState(ProgressObjectId, State);
    bCleared = bHasState && State.State == ETunaSweeperWorldProgressState::Completed;
    bArmed = bHasState && !bCleared && State.ProgressQuantity >= 1;
    if (bCleared) bSaveFailed = false;
    if (bCleared || !bArmed) GetWorldTimerManager().ClearTimer(DetonationTimer);
    else if (HasActorBegunPlay() && !GetWorldTimerManager().TimerExists(DetonationTimer)) StartCountdown();
    ApplyPresentation();
}
bool ATunaSweeperRockfallBarrierActor::ArmExplosive(bool bSaveImmediately)
{
    if (bMutating) return false;
    RefreshSavedState();
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game || ProgressObjectId.IsNone() || RequiredItemId <= 0 || bArmed || bCleared) return false;
    TGuardValue<bool> Guard(bMutating, true);
    bSaveChanges = bSaveImmediately;
    const bool bSuccess = Game->CommitRockfallProgress(ProgressObjectId, RequiredItemId, false, bSaveImmediately);
    bSaveFailed = !bSuccess && Game->CountInventoryItemById(RequiredItemId) > 0;
    if (bSaveFailed)
        if (auto* Toast = Game->GetSubsystem<UTunaSweeperToastSubsystem>())
            Toast->ShowLocalizedToast(TEXT("ui.rockfall.save_failed"),FText::GetEmpty());
    if (bSuccess) { bArmed = true; StartCountdown(); }
    ApplyPresentation();
    return bSuccess;
}
void ATunaSweeperRockfallBarrierActor::StartCountdown()
{
    CountdownStartedAt = GetWorld()->GetTimeSeconds();
    GetWorldTimerManager().SetTimer(DetonationTimer, this, &ThisClass::Detonate, FMath::Max(.1f,CountdownSeconds), false);
}
float ATunaSweeperRockfallBarrierActor::GetCountdownRemaining() const
{
    return bArmed && GetWorld() ? FMath::Max(0.f, GetWorld()->GetTimerManager().GetTimerRemaining(DetonationTimer)) : 0.f;
}
void ATunaSweeperRockfallBarrierActor::Detonate()
{
    if (bMutating || !bArmed || bCleared) return;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game) return;
    TGuardValue<bool> Guard(bMutating,true);
    if (!Game->CommitRockfallProgress(ProgressObjectId, 0, true, bSaveChanges))
    {
        if (!bSaveFailed)
            if (auto* Toast = Game->GetSubsystem<UTunaSweeperToastSubsystem>())
                Toast->ShowLocalizedToast(TEXT("ui.rockfall.save_failed"),FText::GetEmpty());
        bSaveFailed = true;
        GetWorldTimerManager().SetTimer(DetonationTimer, this, &ThisClass::Detonate, 1.f, false);
        ApplyPresentation(); return;
    }
    bCleared = true; bArmed = false; bSaveFailed = false;
    GetWorldTimerManager().ClearTimer(DetonationTimer);
    ApplyPresentation();
    ApplyExplosionDamage();
    const FTransform EffectTransform(GetActorRotation(),GetActorTransform().TransformPosition(ExplosionOffset));
    if (auto* Effect = GetWorld()->SpawnActorDeferred<ATunaSweeperLocalExplosionEffectActor>(
        ATunaSweeperLocalExplosionEffectActor::StaticClass(),EffectTransform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
    {
        Effect->ConfigureExplosion(ExplosionRadiusCm,.9f);
        Effect->FinishSpawning(EffectTransform);
    }
}
void ATunaSweeperRockfallBarrierActor::ApplyPresentation()
{
    // Descendant meshes authored in the BP belong to the same removable obstruction.
    TArray<USceneComponent*> RockParts; CentralRocks->GetChildrenComponents(true, RockParts); RockParts.Add(CentralRocks);
    for (auto* Part : RockParts)
    {
        Part->SetVisibility(!bCleared); Part->SetHiddenInGame(bCleared);
        if (auto* Primitive = Cast<UPrimitiveComponent>(Part))
            Primitive->SetCollisionEnabled(bCleared ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    }
    BlockingCollision->SetCollisionEnabled(bCleared ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    ExplosiveBottle->SetVisibility(bArmed, true); ExplosiveBottle->SetHiddenInGame(!bArmed,true);
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    const bool bAvailable = !ProgressObjectId.IsNone() && RequiredItemId > 0 && !bArmed && !bCleared;
    const FName Key(TEXT("ui.interaction.install_concentrated_explosive"));
    Interactable->SetInteractionTypeDisplayNameAndStringKey(bAvailable ? ETunaSweeperInteractionType::WorldProgress : ETunaSweeperInteractionType::None,
        Game && bAvailable ? Game->ResolveLocalizedText(Key,FText::GetEmpty()) : FText::GetEmpty(), bAvailable ? Key : NAME_None);
    UTexture2D* Icon = nullptr;
    if (auto* Items = Game && bAvailable ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr)
    {
        FTunaSweeperItemDefinition Item;
        if (Items->TryGetItemDefinition(RequiredItemId,Item))
        {
            const FString Path = Items->BuildItemIconObjectPath(Item);
            if (!Path.IsEmpty()) Icon = LoadObject<UTexture2D>(nullptr,*Path);
        }
    }
    Interactable->SetInteractionRequirementPreview(Icon,1,bAvailable);
    CountdownText->SetVisibility(bArmed && !bCleared);
    RefreshWarning();
    if (Game)
        CountdownText->SetText(FText::Format(Game->ResolveLocalizedText(TEXT("ui.rockfall.countdown"),FText::GetEmpty()),
            FText::AsNumber(bSaveFailed ? 0 : FMath::CeilToInt(GetCountdownRemaining()))));
}
void ATunaSweeperRockfallBarrierActor::RefreshWarning()
{
    WarningDecal->SetVisibility(bArmed && !bCleared);
    WarningDecal->DecalSize = FVector(30,FMath::Max(1.f,ExplosionRadiusCm),FMath::Max(1.f,ExplosionRadiusCm));
    if (!bArmed || bCleared) return;
    if (!WarningMID)
    {
        if (UMaterialInterface* Material = WarningMaterial.LoadSynchronous())
        {
            WarningMID = UMaterialInstanceDynamic::Create(Material,this);
            WarningDecal->SetDecalMaterial(WarningMID);
        }
    }
    if (WarningMID)
    {
        const float Elapsed = FMath::Max(0.f,static_cast<float>(GetWorld()->GetTimeSeconds()-CountdownStartedAt));
        WarningMID->SetScalarParameterValue(TEXT("FillProgress"),FMath::Clamp(Elapsed,0.f,1.f));
        WarningMID->SetScalarParameterValue(TEXT("WarningPhase"),Elapsed);
    }
}
float ATunaSweeperRockfallBarrierActor::GetExplosionDamageAtLocation(FVector Location) const
{
    const float Radius = FMath::Max(1.f,ExplosionRadiusCm);
    const float Distance = FVector::Dist2D(Location,GetActorLocation());
    // Match the danger circle's ground footprint, while excluding actors on distant floors.
    if (Distance > Radius || FMath::Abs(Location.Z-GetActorLocation().Z) > Radius) return 0.f;
    const float Alpha = FMath::Clamp((Distance-70.f)/FMath::Max(1.f,Radius-70.f),0.f,1.f);
    return FMath::Lerp(FMath::Max(0.f,ExplosionDamage),FMath::Clamp(EdgeExplosionDamage,0.f,FMath::Max(0.f,ExplosionDamage)),Alpha);
}
void ATunaSweeperRockfallBarrierActor::ApplyExplosionDamage()
{
    // An environmental source intentionally has no player owner/instigator: its blast can hurt the installer too.
    TArray<TWeakObjectPtr<APawn>> Targets;
    for (TActorIterator<APawn> It(GetWorld()); It; ++It) Targets.Add(*It);
    for (const auto& Target : Targets)
        if (APawn* Pawn = Target.Get())
        {
            const float Damage = GetExplosionDamageAtLocation(Pawn->GetActorLocation());
            if (Damage > 0) UGameplayStatics::ApplyDamage(Pawn,Damage,nullptr,this,UDamageType::StaticClass());
        }
}
void ATunaSweeperRockfallBarrierActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bArmed && !bSaveFailed) return;
    RefreshWarning();
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
        CountdownText->SetText(FText::Format(Game->ResolveLocalizedText(TEXT("ui.rockfall.countdown"),FText::GetEmpty()),
            FText::AsNumber(bSaveFailed ? 0 : FMath::CeilToInt(GetCountdownRemaining()))));
    if (auto* Camera = UGameplayStatics::GetPlayerCameraManager(this,0))
        CountdownText->SetWorldRotation((Camera->GetCameraLocation()-CountdownText->GetComponentLocation()).Rotation());
}
