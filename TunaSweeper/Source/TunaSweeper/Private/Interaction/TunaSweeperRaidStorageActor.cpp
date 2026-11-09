#include "Interaction/TunaSweeperRaidStorageActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

ATunaSweeperRaidStorageActor::ATunaSweeperRaidStorageActor()
{
    bUseComponentMeshPresentation = true;
}

void ATunaSweeperRaidStorageActor::BeginPlay()
{
    Super::BeginPlay();
    StorageGame = GetGameInstance<UTunaSweeperGameInstance>();
    if (StorageGame.IsValid())
    {
        StorageGame->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshIndicators);
        StorageGame->OnActiveLootContainerUiClosed.AddUObject(this, &ThisClass::HandleContainerClosed);
    }
    RefreshIndicators();
}

void ATunaSweeperRaidStorageActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (StorageGame.IsValid())
    {
        StorageGame->CloseRaidStorage(this);
        StorageGame->OnInventoryStateChanged.RemoveAll(this);
        StorageGame->OnActiveLootContainerUiClosed.RemoveAll(this);
    }
    Super::EndPlay(EndPlayReason);
}

int32 ATunaSweeperRaidStorageActor::CountLoadedStorageActors() const
{
    int32 Count = 0;
    for (TActorIterator<ATunaSweeperRaidStorageActor> It(GetWorld()); It; ++It)
        if (!It->IsActorBeingDestroyed()) ++Count;
    return Count;
}

bool ATunaSweeperRaidStorageActor::OpenRuntimeContainer(UTunaSweeperGameInstance* Game,
    FTunaSweeperLootContainerInstance& OutInstance)
{
    if (!Game || Game->IsCombatTestSession()) return false;
    OutInstance = FTunaSweeperLootContainerInstance();
    OutInstance.Capacity = UTunaSweeperGameInstance::RaidStorageCapacity;
    OutInstance.DisplayName = Game->ResolveLocalizedText(TEXT("ui.raid_storage.name"), FText::GetEmpty());
    if (CountLoadedStorageActors() > 1)
    {
        const FText Warning = Game->ResolveLocalizedText(TEXT("ui.raid_storage.duplicate"), FText::GetEmpty());
        if (GEngine) GEngine->AddOnScreenDebugMessage(0x52414945, 8.0f, FColor::Orange, Warning.ToString());
        UE_LOG(LogTemp, Warning, TEXT("Multiple raid storage actors loaded; all use one shared warehouse."));
    }
    Game->OpenRaidStorage(this, OutInstance.DisplayName);
    RefreshIndicators();
    return true;
}

void ATunaSweeperRaidStorageActor::HandleContainerClosed()
{
    if (StorageGame.IsValid() && StorageGame->GetActiveLootContainerOwner() == this)
        PlayCloseAnimation();
}

FLinearColor ATunaSweeperRaidStorageActor::GetSlotIndicatorColor(int32 ItemId, int32 Quantity) const
{
    if (ItemId == INDEX_NONE || Quantity <= 0) return FLinearColor::Black;
    if (ItemId != 3004) return OtherItemColor;
    return Quantity == 10 ? FullTunaColor : FullTunaColor * PartialTunaBrightness;
}

void ATunaSweeperRaidStorageActor::RefreshIndicators()
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game) return;
    const auto& Slots = Game->GetRaidStorageSlots();
    TInlineComponentArray<UStaticMeshComponent*> Meshes(this);
    for (int32 Index = 0; Index < UTunaSweeperGameInstance::RaidStorageCapacity; ++Index)
    {
        FTunaSweeperItemInstance Item;
        const bool bHasItem = Slots.IsValidIndex(Index) && Game->TryGetItemInstance(Slots[Index].ItemUid, Item);
        const FLinearColor Color = GetSlotIndicatorColor(bHasItem ? Item.ItemId : INDEX_NONE, bHasItem ? Item.Quantity : 0);
        const FName Tag(*FString::Printf(TEXT("RaidStorageLED_%d"), Index));
        for (auto* Mesh : Meshes)
        {
            if (!Mesh->ComponentHasTag(Tag)) continue;
            auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
            if (!Material) Material = Mesh->CreateDynamicMaterialInstance(0);
            if (Material) Material->SetVectorParameterValue(TEXT("IndicatorColor"), Color);
        }
    }
}
