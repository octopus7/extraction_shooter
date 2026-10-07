#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperDebugArmoryActor.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperDebugArmoryPanelWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Widgets/SOverlay.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#endif

namespace TunaDebugArmoryTests
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Game->*Member = Context;
		}
	};

	struct FHudTickAccess : UTunaSweeperGameHudWidget
	{
		static void Advance(UTunaSweeperGameHudWidget* Hud, float DeltaSeconds)
		{
			auto TickFunctionPointer = &FHudTickAccess::NativeTick;
			(Hud->*TickFunctionPointer)(Hud->GetCachedGeometry(), DeltaSeconds);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDebugArmoryTest,
	"TunaSweeper.DebugArmory.InteractionAndSupply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaDebugArmoryTest::RunTest(const FString& Parameters)
{
	// Deliberately does not contain Bunker: placing an armory in another test map must work.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("DebugArmoryTestMap"),
		CreatePackage(TEXT("/Temp/DebugArmoryTestMap")));
	if (!TestNotNull(TEXT("Transient test world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	TunaDebugArmoryTests::FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Real subsystems, inventory and HUD, without loading or overwriting player saves.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->bOutfitUnlocksLoaded = true;
	Game->UGameInstance::Init();
	Game->ResetPlayerSlotArrays();
	Context.GameViewport = NewObject<UGameViewportClient>(GEngine);
	TSharedRef<SOverlay> ViewportOverlay = SNew(SOverlay);
	Context.GameViewport->SetViewportOverlayWidget(nullptr, ViewportOverlay);
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		Context.GameViewport = nullptr;
		TunaDebugArmoryTests::FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Controller = World->SpawnActor<ATunaSweeperPlayerController>(Spawn);
	UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
		TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
	if (!TestNotNull(TEXT("Actual player Blueprint loads"), PlayerClass)) return false;
	auto* Pawn = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(100, 0, 0), FRotator::ZeroRotator, Spawn);
	auto* Armory = World->SpawnActor<ATunaSweeperDebugArmoryActor>(Spawn);
	if (!TestNotNull(TEXT("Controller exists"), Controller) || !TestNotNull(TEXT("Pawn exists"), Pawn) ||
		!TestNotNull(TEXT("Armory exists"), Armory)) return false;
	auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->ViewportClient = Context.GameViewport;
	Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Pawn);
	Pawn->DispatchBeginPlay();
	Armory->SetActorLocation(FVector::ZeroVector);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TArray<FTunaSweeperItemDefinition> Catalog;

#if UE_BUILD_SHIPPING
	TestFalse(TEXT("Shipping disables the armory"), Armory->IsArmoryEnabled());
	TestFalse(TEXT("Shipping exposes no supply catalog"), Armory->GetSupplyCatalog(Catalog));
	TestEqual(TEXT("Shipping catalog is empty"), Catalog.Num(), 0);
	TestFalse(TEXT("Shipping refuses nearby use"), Armory->CanUseArmory(Pawn));
	TestFalse(TEXT("Shipping refuses direct supplies"), Armory->TrySupplyItem(Pawn, 1002, 1));
	TestFalse(TEXT("Shipping refuses the interaction route"), Armory->RequestInteraction(Pawn));
	TestFalse(TEXT("Shipping refuses the HUD route"), Controller->OpenDebugArmoryPanel(Armory));
	TestFalse(TEXT("Shipping removes the interactable and its marker"), IsValid(Armory->GetInteractableComponent()));
	return true;
#else
	TestTrue(TEXT("Development enables the armory"), Armory->IsArmoryEnabled());
	TestEqual(TEXT("Armory has a dedicated interaction"), Armory->GetInteractionType(), ETunaSweeperInteractionType::DebugArmoryOpen);
	// Keep the process culture unchanged while checking the fixture's localized text.
	Game->CurrentTextLanguage = ETunaSweeperItemTextLanguage::Korean;
	TestEqual(TEXT("Interaction resolves the authored localization key"), Armory->GetInteractionDisplayName().ToString(),
		Game->ResolveLocalizedText(TEXT("ui.interaction.debug_armory_open"), FText::GetEmpty()).ToString());
	TestEqual(TEXT("Korean interaction reads armory"), Armory->GetInteractionDisplayName().ToString(), FString(TEXT("무기고")));
	TestFalse(TEXT("Missing pawn cannot use the armory"), Armory->CanUseArmory(nullptr));
	TestFalse(TEXT("Missing pawn cannot receive supplies"), Armory->TrySupplyItem(nullptr, 1002, 1));
	TestTrue(TEXT("Nearby living local pawn can use an armory on any map"), Armory->CanUseArmory(Pawn));
	if (!TestTrue(TEXT("Real supply catalog loads"), Armory->GetSupplyCatalog(Catalog))) return false;
	auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	TArray<FTunaSweeperItemDefinition> Definitions;
	if (!TestTrue(TEXT("Production item definitions load"), Items && Items->GetAllItemDefinitions(Definitions))) return false;
	const TSet<FName> AllowedCategories = {
		TEXT("item.category.weapon.gun"), TEXT("item.category.weapon.melee"), TEXT("item.category.attachment"),
		TEXT("item.category.ammo"), TEXT("item.category.head"), TEXT("item.category.body"),
		TEXT("item.category.face"), TEXT("item.category.ear")};
	TMap<FName, int32> RepresentativeIds;
	TSet<int32> CatalogIds;
	for (const FTunaSweeperItemDefinition& Definition : Catalog)
	{
		TestTrue(TEXT("Every catalog entry is in an allowed category"), AllowedCategories.Contains(Definition.CategoryTag));
		TestFalse(TEXT("Catalog has no duplicate item IDs"), CatalogIds.Contains(Definition.Id));
		CatalogIds.Add(Definition.Id);
	}
	for (const FTunaSweeperItemDefinition& Definition : Definitions)
	{
		const bool bExpectedAllowed = AllowedCategories.Contains(Definition.CategoryTag);
		TestEqual(*FString::Printf(TEXT("Category policy for authored item %d"), Definition.Id),
			Armory->IsAllowedItem(Definition), bExpectedAllowed);
		TestEqual(*FString::Printf(TEXT("Catalog membership for authored item %d"), Definition.Id),
			CatalogIds.Contains(Definition.Id), bExpectedAllowed);
		RepresentativeIds.FindOrAdd(Definition.CategoryTag, Definition.Id);
	}
	for (const FName Category : AllowedCategories)
	{
		if (!TestTrue(TEXT("Every supported equipment category has real authored data"), RepresentativeIds.Contains(Category))) return false;
	}
	const int32 GunId = RepresentativeIds.FindChecked(TEXT("item.category.weapon.gun"));
	const int32 AmmoId = RepresentativeIds.FindChecked(TEXT("item.category.ammo"));
	if (!TestTrue(TEXT("Production currency exists"), RepresentativeIds.Contains(TEXT("item.category.currency")))) return false;
	const int32 CoinId = RepresentativeIds.FindChecked(TEXT("item.category.currency"));
	if (!TestTrue(TEXT("Fixture receives currency"), Game->AddItemToFirstAvailableInventorySlot(CoinId, 5))) return false;
	TestFalse(TEXT("No source cannot open an armory"), Controller->OpenDebugArmoryPanel(nullptr));
	TestFalse(TEXT("Controller cannot grant without an active interaction"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	if (!TestTrue(TEXT("Real interaction opens the armory"), Armory->RequestInteraction(Pawn))) return false;
	auto* Hud = Controller->GetGameHudWidget();
	if (!TestNotNull(TEXT("Opening creates the actual HUD"), Hud)) return false;
	TestTrue(TEXT("Armory is attached to the viewport"), Hud->IsInViewport() && ViewportOverlay->GetChildren()->Num() > 0);
	TestEqual(TEXT("Opening activates armory mode"), Hud->GetHudMode(), ETunaSweeperHudMode::DebugArmory);
	TestTrue(TEXT("Armory suppresses gameplay input"), Controller->IsInventoryUiOpen());
	UTunaSweeperDebugArmoryPanelWidget* Panel = nullptr;
	Hud->WidgetTree->ForEachWidget([&Panel](UWidget* Widget)
	{
		if (auto* ArmoryPanel = Cast<UTunaSweeperDebugArmoryPanelWidget>(Widget)) Panel = ArmoryPanel;
	});
	if (!TestNotNull(TEXT("Actual HUD tree contains the armory panel"), Panel)) return false;
	Panel->TakeWidget();
	auto* CategoryCombo = Cast<UComboBoxString>(Panel->WidgetTree->FindWidget(TEXT("DebugArmoryCategory")));
	auto* QuantityInput = Cast<USpinBox>(Panel->WidgetTree->FindWidget(TEXT("DebugArmoryQuantity")));
	auto* Supply = Cast<UButton>(Panel->WidgetTree->FindWidget(TEXT("DebugArmorySupplyButton")));
	auto* Status = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("DebugArmoryStatus")));
	auto* SelectedName = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("DebugArmorySelectedItem")));
	auto* Grid = Cast<UUniformGridPanel>(Panel->WidgetTree->FindWidget(TEXT("DebugArmoryItems")));
	auto* CloseLabel = Cast<UTextBlock>(Panel->WidgetTree->FindWidget(TEXT("DebugArmoryCloseText")));
	if (!TestNotNull(TEXT("Category dropdown exists"), CategoryCombo) || !TestNotNull(TEXT("Quantity input exists"), QuantityInput) ||
		!TestNotNull(TEXT("Supply button exists"), Supply) || !TestNotNull(TEXT("Status label exists"), Status) ||
		!TestNotNull(TEXT("Selected item label exists"), SelectedName) || !TestNotNull(TEXT("Item grid exists"), Grid) ||
		!TestNotNull(TEXT("Close label exists"), CloseLabel)) return false;
	auto* Close = Cast<UButton>(CloseLabel->GetParent());
	if (!TestNotNull(TEXT("Close label belongs to an actionable button"), Close)) return false;
	CategoryCombo->SetSelectedOption(TEXT("ui.debug_armory.ammo"));
	int32 ExpectedAmmoCards = 0;
	const FTunaSweeperItemDefinition* FirstDisplayedAmmo = nullptr;
	for (const FTunaSweeperItemDefinition& Item : Catalog)
	{
		if (Item.CategoryTag != TEXT("item.category.ammo")) continue;
		if (!FirstDisplayedAmmo) FirstDisplayedAmmo = &Item;
		++ExpectedAmmoCards;
	}
	if (!TestNotNull(TEXT("Authored catalog contains ammunition for the UI"), FirstDisplayedAmmo)) return false;
	TestEqual(TEXT("Ammo category displays exactly the available ammunition"), Grid->GetChildrenCount(), ExpectedAmmoCards);
	FText ExpectedAmmoName;
	TestTrue(TEXT("First ammo has a localized name"), Items->TryGetItemNameTextByKey(
		FirstDisplayedAmmo->NameStringKey, Game->GetCurrentTextLanguage(), ExpectedAmmoName));
	TestEqual(TEXT("Changing category selects its first ammunition"), SelectedName->GetText().ToString(), ExpectedAmmoName.ToString());
	TestTrue(TEXT("Selected ammunition enables supply"), Supply->GetIsEnabled());
	const int32 BeforeButtonSupply = Game->CountInventoryItemById(FirstDisplayedAmmo->Id);
	QuantityInput->SetValue(3);
	Supply->OnClicked.Broadcast();
	TestEqual(TEXT("Actual supply button grants the requested three rounds"),
		Game->CountInventoryItemById(FirstDisplayedAmmo->Id), BeforeButtonSupply + 3);
	TestEqual(TEXT("Successful button supply shows the localized result"), Status->GetText().ToString(),
		Game->ResolveLocalizedText(TEXT("ui.debug_armory.success"), FText::GetEmpty()).ToString());
	TestFalse(TEXT("Successful supply feedback is not blank"), Status->GetText().IsEmpty());
	TestEqual(TEXT("UI supply leaves currency unchanged"), Game->CountInventoryItemById(CoinId), 5);
	if (FParse::Param(FCommandLine::Get(), TEXT("DebugArmoryCapture")) && FApp::CanEverRender())
	{
		// The synchronous fixture does not naturally tick the HUD fade-in.
		TunaDebugArmoryTests::FHudTickAccess::Advance(Hud, 1.0f);
		TestTrue(TEXT("Capture waits for the actual panel fade-in"), Panel->GetRenderOpacity() > 0.99f);
#if WITH_EDITOR
		FAssetCompilingManager::Get().FinishAllCompilation();
#endif
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("DebugArmoryPreview");
		IFileManager::Get().MakeDirectory(*Directory, true);
		const FIntPoint Size(1280, 720);
		FWidgetRenderer Renderer(false);
		TSharedRef<SWidget> Slate = Panel->TakeWidget();
		UTextureRenderTarget2D* Target = nullptr;
		for (int32 Frame = 0; Frame < 3; ++Frame)
		{
			Target = Renderer.DrawWidget(Slate, FVector2D(Size.X, Size.Y));
		}
		if (!TestNotNull(TEXT("Actual armory panel renders to a target"), Target)) return false;
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags;
		ReadFlags.SetLinearToGamma(false);
		if (!TestTrue(TEXT("Armory preview pixels can be read"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags))) return false;
		TestEqual(TEXT("Armory preview captures the full requested resolution"), Pixels.Num(), Size.X * Size.Y);
		TestTrue(TEXT("Armory capture contains visible UI pixels"), Pixels.ContainsByPredicate(
			[](const FColor& Pixel) { return Pixel.A > 0 && (Pixel.R > 50 || Pixel.G > 50 || Pixel.B > 50); }));
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
		TestTrue(TEXT("Actual armory panel PNG is saved"), FFileHelper::SaveArrayToFile(Png,
			*(Directory / TEXT("DebugArmory_1280x720_Korean.png"))));
	}
	Close->OnClicked.Broadcast();
	TestEqual(TEXT("Actual close button dismisses the panel"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Actual close button clears the supply session"), Controller->IsDebugArmoryInteractionValid());
	if (!TestTrue(TEXT("Armory reopens after its close button"), Controller->OpenDebugArmoryPanel(Armory))) return false;
	for (const FName Category : AllowedCategories)
	{
		const int32 ItemId = RepresentativeIds.FindChecked(Category);
		const int32 Before = Game->CountInventoryItemById(ItemId);
		TestTrue(TEXT("Every supported category can be supplied"), Controller->TrySupplyDebugArmoryItem(ItemId, 1));
		TestEqual(TEXT("Supply adds exactly one requested item"), Game->CountInventoryItemById(ItemId), Before + 1);
	}
	const int32 AmmoBefore = Game->CountInventoryItemById(AmmoId);
	TestTrue(TEXT("Inclusive maximum quantity is accepted when it fits"), Controller->TrySupplyDebugArmoryItem(AmmoId, 999));
	TestEqual(TEXT("Maximum grant adds exactly the requested ammunition"), Game->CountInventoryItemById(AmmoId), AmmoBefore + 999);
	TestEqual(TEXT("Free supplies preserve currency"), Game->CountInventoryItemById(CoinId), 5);
	for (const int32 Quantity : {0, -1, 1000})
	{
		TestFalse(TEXT("Direct supply rejects invalid quantities"), Armory->TrySupplyItem(Pawn, AmmoId, Quantity));
		TestFalse(TEXT("HUD supply rejects invalid quantities"), Controller->TrySupplyDebugArmoryItem(AmmoId, Quantity));
	}
	TestFalse(TEXT("Unknown item is rejected"), Controller->TrySupplyDebugArmoryItem(MAX_int32, 1));
	TestFalse(TEXT("Invalid item sentinel is rejected"), Armory->TrySupplyItem(Pawn, INDEX_NONE, 1));
	for (const FTunaSweeperItemDefinition& Definition : Definitions)
	{
		if (AllowedCategories.Contains(Definition.CategoryTag)) continue;
		const int32 Before = Game->CountInventoryItemById(Definition.Id);
		TestFalse(TEXT("Disallowed authored items cannot be supplied directly"), Armory->TrySupplyItem(Pawn, Definition.Id, 1));
		TestFalse(TEXT("Disallowed authored items cannot be supplied by HUD"), Controller->TrySupplyDebugArmoryItem(Definition.Id, 1));
		TestEqual(TEXT("Rejected item leaves inventory unchanged"), Game->CountInventoryItemById(Definition.Id), Before);
	}
	TestEqual(TEXT("Rejected quantities leave ammunition unchanged"), Game->CountInventoryItemById(AmmoId), AmmoBefore + 999);
	TestEqual(TEXT("Ordinary grant failure keeps the valid panel open"), Hud->GetHudMode(), ETunaSweeperHudMode::DebugArmory);

	// Leave one round of stack space but no empty slots. A two-round grant must roll back that partial fill.
	Game->ItemInstancesByUid.Reset();
	Game->ResetPlayerSlotArrays();
	FTunaSweeperItemDefinition AmmoDefinition;
	if (!TestTrue(TEXT("Ammo definition resolves"), Items->TryGetItemDefinition(AmmoId, AmmoDefinition))) return false;
	const int32 AmmoStackSize = Items->ResolveItemMaxStackQuantity(AmmoDefinition);
	if (!TestTrue(TEXT("Ammo supports a partial stack"), AmmoStackSize > 1)) return false;
	if (!TestTrue(TEXT("Fixture creates a nearly full ammo stack"), Game->AddItemToFirstAvailableInventorySlot(AmmoId, AmmoStackSize - 1))) return false;
	const int32 SlotCapacity = Game->GetCurrentInventorySlotCapacity();
	if (!TestTrue(TEXT("Fixture fills remaining slots with separate guns"), Game->AddItemToFirstAvailableInventorySlot(GunId, SlotCapacity - 1))) return false;
	const int32 InstancesBeforeFailure = Game->ItemInstancesByUid.Num();
	const TArray<FTunaSweeperInventorySlot> SlotsBeforeFailure = Game->PlayerInventorySlots;
	TestFalse(TEXT("Insufficient room rejects the entire requested supply"), Controller->TrySupplyDebugArmoryItem(AmmoId, 2));
	TestEqual(TEXT("Failed supply rolls back partial stack filling"), Game->CountInventoryItemById(AmmoId), AmmoStackSize - 1);
	TestEqual(TEXT("Failed supply creates no orphan item instances"), Game->ItemInstancesByUid.Num(), InstancesBeforeFailure);
	for (int32 Index = 0; Index < SlotsBeforeFailure.Num(); ++Index)
	{
		TestEqual(TEXT("Failed supply preserves slot identities"), Game->PlayerInventorySlots[Index].ItemUid, SlotsBeforeFailure[Index].ItemUid);
	}
	TestTrue(TEXT("The remaining one round of capacity can still be used"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	TestEqual(TEXT("Successful final round fills the stack exactly"), Game->CountInventoryItemById(AmmoId), AmmoStackSize);
	TestFalse(TEXT("Full inventory rejects a new gun"), Armory->TrySupplyItem(Pawn, GunId, 1));
	Game->ItemInstancesByUid.Reset();
	Game->ResetPlayerSlotArrays();

	Controller->TogglePauseMenu();
	TestEqual(TEXT("Escape closes armory mode"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Escape does not stack the pause menu"), Controller->IsPauseMenuOpen());
	TestFalse(TEXT("Escape invalidates the supply session"), Controller->IsDebugArmoryInteractionValid());
	TestFalse(TEXT("Closed panel cannot grant items"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	Hud->SetHudMode(ETunaSweeperHudMode::DebugArmory);
	TestFalse(TEXT("Setting the visual mode cannot fabricate a supply session"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	TestTrue(TEXT("Armory reopens through its source"), Controller->OpenDebugArmoryPanel(Armory));
	Hud->SetHudMode(ETunaSweeperHudMode::Research);
	TestFalse(TEXT("Changing HUD mode releases the source"), Controller->IsDebugArmoryInteractionValid());
	TestFalse(TEXT("Another HUD mode cannot supply items"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	TestTrue(TEXT("Armory reopens before explicit close"), Controller->OpenDebugArmoryPanel(Armory));
	Controller->CloseDebugArmoryPanel();
	TestFalse(TEXT("Explicit close clears the session"), Controller->IsDebugArmoryInteractionValid());
	Controller->UnPossess();
	TestFalse(TEXT("Unpossessed pawn cannot use the armory"), Armory->CanUseArmory(Pawn));
	TestFalse(TEXT("Unpossessed pawn cannot receive supplies"), Armory->TrySupplyItem(Pawn, AmmoId, 1));
	Controller->Possess(Pawn);
	// This synchronous test has no Slate frame between opening and closing modes.
	// Let the real HUD finish its hide transitions before attempting to mount.
	Hud->SetHudMode(ETunaSweeperHudMode::None);
	TunaDebugArmoryTests::FHudTickAccess::Advance(Hud, 1.0f);
	TestFalse(TEXT("HUD hide transitions finish before boarding"), Controller->IsInventoryUiOpen());
	TestFalse(TEXT("No pause screen blocks the vehicle fixture"), Controller->IsPauseMenuOpen());
	TestFalse(TEXT("No dialogue blocks the vehicle fixture"), Controller->IsDialogueSequenceActive());
	TestFalse(TEXT("No housing mode blocks the vehicle fixture"), Controller->IsHousingModeOpen());
	TestNull(TEXT("Player is unattached before boarding"), Pawn->GetAttachParentActor());
	auto* Vehicle = World->SpawnActor<AActor>(Spawn);
	if (!TestNotNull(TEXT("Transient vehicle fixture exists"), Vehicle)) return false;
	auto* Mount = NewObject<UTunaSweeperVehicleMountComponent>(Vehicle);
	Vehicle->AddInstanceComponent(Mount);
	Vehicle->SetRootComponent(Mount);
	Mount->VehicleHudClass.Reset();
	Mount->RegisterComponent();
	Vehicle->SetActorLocation(Pawn->GetActorLocation());
	TestTrue(TEXT("Vehicle fixture is inside mount interaction distance"), Mount->IsWithinInteractionDistance(Pawn));
	TestTrue(TEXT("Vehicle fixture seat height matches the player"),
		FMath::Abs(Pawn->GetActorLocation().Z - Mount->GetComponentLocation().Z) <= Mount->GetInteractionDistance());
	if (!TestTrue(TEXT("Fixture mounts through the real vehicle component"), Mount->TryMount(Pawn))) return false;
	TestTrue(TEXT("Player is actually mounted"), Pawn->IsMountedInVehicle());
	TestFalse(TEXT("Mounted player cannot use the armory"), Armory->CanUseArmory(Pawn));
	TestFalse(TEXT("Mounted player cannot receive supplies"), Armory->TrySupplyItem(Pawn, AmmoId, 1));
	TestFalse(TEXT("Mounted player cannot open the panel"), Controller->OpenDebugArmoryPanel(Armory));
	Mount->ReleaseRiderForEndPlay();
	Vehicle->Destroy();
	TestFalse(TEXT("Vehicle fixture releases the rider"), Pawn->IsMountedInVehicle());
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("Armory opens before range check"), Controller->OpenDebugArmoryPanel(Armory));
	Pawn->SetActorLocation(FVector(Armory->GetInteractionDistance() + 1.0f, 0, 0));
	TestFalse(TEXT("Distance invalidates direct use"), Armory->CanUseArmory(Pawn));
	TestFalse(TEXT("Distance prevents direct grants"), Armory->TrySupplyItem(Pawn, AmmoId, 1));
	TestFalse(TEXT("Distance invalidates the open session"), Controller->IsDebugArmoryInteractionValid());
	Controller->UpdateDebugArmoryInteraction();
	TestEqual(TEXT("Range validation closes the HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Outside interaction range cannot reopen"), Controller->OpenDebugArmoryPanel(Armory));
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("Armory reopens before grant revalidation"), Controller->OpenDebugArmoryPanel(Armory));
	Pawn->SetActorLocation(FVector(Armory->GetInteractionDistance() + 1.0f, 0, 0));
	TestFalse(TEXT("Grant itself revalidates distance"), Controller->TrySupplyDebugArmoryItem(AmmoId, 1));
	TestEqual(TEXT("Invalid grant closes the session immediately"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("Armory opens before being hidden"), Controller->OpenDebugArmoryPanel(Armory));
	Armory->SetActorHiddenInGame(true);
	TestFalse(TEXT("Hidden armory cannot supply directly"), Armory->TrySupplyItem(Pawn, AmmoId, 1));
	Controller->UpdateDebugArmoryInteraction();
	TestEqual(TEXT("Hidden source closes the HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Armory->SetActorHiddenInGame(false);
	TestTrue(TEXT("Armory opens before source deletion"), Controller->OpenDebugArmoryPanel(Armory));
	Armory->Destroy();
	TestFalse(TEXT("Source deletion invalidates session"), Controller->IsDebugArmoryInteractionValid());
	Controller->UpdateDebugArmoryInteraction();
	TestEqual(TEXT("Source deletion closes the HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Armory = World->SpawnActor<ATunaSweeperDebugArmoryActor>(Spawn);
	if (!TestNotNull(TEXT("Replacement armory exists"), Armory)) return false;
	TestTrue(TEXT("Armory opens before player death"), Controller->OpenDebugArmoryPanel(Armory));
	auto* Vitals = Pawn->GetVitalsComponent();
	if (!TestNotNull(TEXT("Real health component exists"), Vitals)) return false;
	FTunaSweeperVitalsDelta LethalDamage;
	LethalDamage.Health = -100000.0f;
	Vitals->ApplyVitalsDelta(LethalDamage);
	TestTrue(TEXT("Lethal damage causes actual player death"), Pawn->IsDead());
	TestFalse(TEXT("Dead player cannot receive direct supplies"), Armory->TrySupplyItem(Pawn, AmmoId, 1));
	TestFalse(TEXT("Death invalidates the open session"), Controller->IsDebugArmoryInteractionValid());
	Controller->UpdateDebugArmoryInteraction();
	TestEqual(TEXT("Death closes the armory HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestTrue(TEXT("Closing after death retains movement lock"), Controller->IsMoveInputIgnored());
	TestTrue(TEXT("Closing after death retains look lock"), Controller->IsLookInputIgnored());
	TestEqual(TEXT("All invalid sessions grant zero ammunition"), Game->CountInventoryItemById(AmmoId), 0);
	return true;
#endif
}

#endif
