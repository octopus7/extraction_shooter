#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "AssetCompilingManager.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperQuadrupedEnemyCharacter.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Encyclopedia/TunaSweeperEncyclopedia.h"
#include "Game/TunaSweeperGameInstance.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/Astra/EncyclopediaGPT6Astra.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncyclopediaContractTest, "TunaSweeper.Encyclopedia.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEncyclopediaContractTest::RunTest(const FString&)
{
    TestNotNull(TEXT("Enemy discoveries are serialized per save slot"),
        FindFProperty<FArrayProperty>(UTunaSweeperSaveGame::StaticClass(), TEXT("DefeatedEnemyKinds")));
    for (const TCHAR* Name : {TEXT("Panel"), TEXT("Entry"), TEXT("Launcher")})
    {
        const FString Path = FString::Printf(TEXT("/Game/UI/Encyclopedia/WBP_Encyclopedia%s_GPT6Astra"), Name);
        auto* BP = LoadObject<UWidgetBlueprint>(nullptr, *Path, nullptr, LOAD_NoWarn);
        if (TestNotNull(*Path, BP))
        {
            TestNotNull(TEXT("Editable authored hierarchy"), BP->WidgetTree.Get());
            TestNotNull(TEXT("Saved root"), BP->WidgetTree->RootWidget.Get());
            TestNotNull(TEXT("Compiled Blueprint class"), BP->GeneratedClass.Get());
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncyclopediaRuntimeTest, "TunaSweeper.Encyclopedia.Runtime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEncyclopediaRuntimeTest::RunTest(const FString&)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context); Context.OwningGameInstance = Game; World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never write a user's save.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        World->DestroyWorld(false); Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
    TArray<FTunaSweeperItemDefinition> Definitions;
    if (!TestTrue(TEXT("Live item SSOT loads"), Items->GetAllItemDefinitions(Definitions))) return false;
    int32 Weapons = 0, Ammo = 0, Attachments = 0;
    for (const auto& Item : Definitions)
    {
        const auto Category = TunaSweeperEncyclopedia::Classify(Item);
        if (Category == EEncyclopediaCategory::Weapon) ++Weapons;
        if (Category == EEncyclopediaCategory::Ammunition) ++Ammo;
        if (Item.CategoryTag == TEXT("item.category.attachment"))
        { ++Attachments; TestTrue(TEXT("Attachment excluded despite compatibility tags"), Category == EEncyclopediaCategory::Excluded); }
    }
    TestTrue(TEXT("Production fixture has attachments to exercise exclusion"), Attachments > 0);
    TArray<UEncyclopediaEntryData*> Entries;
    TunaSweeperEncyclopedia::BuildEntries(Game, Game, EEncyclopediaCategory::Weapon, Entries);
    TestEqual(TEXT("Every weapon represented"), Entries.Num(), Weapons);
    for (int32 I = 1; I < Entries.Num(); ++I)
        if (Entries[I-1]->Group == Entries[I]->Group)
            TestTrue(TEXT("Within each type grades ascend"), Entries[I-1]->Grade <= Entries[I]->Grade);
    for (auto* Entry : Entries) TestFalse(TEXT("Unacquired weapon stays locked"), Entry->bUnlocked);
    Game->MarkItemEverAcquired(1002);
    TunaSweeperEncyclopedia::BuildEntries(Game, Game, EEncyclopediaCategory::Weapon, Entries);
    auto** Rifle = Entries.FindByPredicate([](const auto* E) { return E->ItemId == 1002; });
    if (TestNotNull(TEXT("Rifle entry"), Rifle)) TestTrue(TEXT("One acquisition unlocks"), (*Rifle)->bUnlocked);
    Game->ItemInstancesByUid.Reset(); // Losing the physical item does not erase history.
    TestTrue(TEXT("Acquisition survives loss"), Game->HasEverAcquiredItem(1002));
    TunaSweeperEncyclopedia::BuildEntries(Game, Game, EEncyclopediaCategory::Ammunition, Entries);
    TSet<int32> AmmoIds; TSet<FName> PistolAmmoGroups;
    for (auto* Entry : Entries) { AmmoIds.Add(Entry->ItemId); if (Entry->ItemId == 2001) PistolAmmoGroups.Add(Entry->Group); }
    TestEqual(TEXT("All cartridges represented without inflating unique count"), AmmoIds.Num(), Ammo);
    TestTrue(TEXT("Pistol rounds also listed for SMG"), PistolAmmoGroups.Contains(TEXT("weapon.type.pistol")) && PistolAmmoGroups.Contains(TEXT("weapon.type.smg")));

    auto* Catalog = UEncyclopediaEnemyCatalog::Get();
    if (!TestNotNull(TEXT("Enemy catalog"), Catalog)) return false;
    UClass* Blue = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/Enemies/QuadrupedVariants/BP_QuadrupedGunEnemy_Blue.BP_QuadrupedGunEnemy_Blue_C"));
    TestEqual(TEXT("Color variant groups with quadruped"), Catalog->ResolveKind(Blue), FName(TEXT("quadruped")));
    UClass* Boss = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/Enemies/Bosses/BP_Region1Boss_Red.BP_Region1Boss_Red_C"));
    TestEqual(TEXT("Specific boss beats parent kind"), Catalog->ResolveKind(Boss), FName(TEXT("red_boss")));
    TestEqual(TEXT("Carrier is not mislabeled as charger"), Catalog->ResolveKind(LoadClass<AActor>(nullptr,
        TEXT("/Script/TunaSweeper.TunaSweeperRobotTeachingMiniboss"))), FName(TEXT("carrier")));
    TestEqual(TEXT("Rolling minion is not mislabeled as guard"), Catalog->ResolveKind(LoadClass<AActor>(nullptr,
        TEXT("/Script/TunaSweeper.TunaSweeperRollingRobotMinion"))), FName(TEXT("minion")));
    TestFalse(TEXT("Invalid kill ignored"), Game->RecordEncyclopediaKill(nullptr, false));
    TestTrue(TEXT("First kill unlocks"), Game->RecordEncyclopediaKill(Blue, false));
    TestFalse(TEXT("Repeat kill is idempotent"), Game->RecordEncyclopediaKill(Blue, false));
    TestTrue(TEXT("Same-kind native enemy unlocked"), Game->HasDefeatedEnemyKind(TEXT("quadruped")));
    Game->BeginCombatTestSession();
    TestFalse(TEXT("Laboratory kill cannot unlock boss"), Game->RecordEncyclopediaKill(Boss, false));
    Game->EndCombatTestSession();
    TestFalse(TEXT("Laboratory left discovery clean"), Game->HasDefeatedEnemyKind(TEXT("red_boss")));
    TestFalse(TEXT("Isolated death-save cannot touch user files"), Game->ClearInventoryAndSaveInternal(false));
    TestTrue(TEXT("Death clearing retains enemy discovery"), Game->HasDefeatedEnemyKind(TEXT("quadruped")));
    TestTrue(TEXT("Death clearing retains item discovery"), Game->HasEverAcquiredItem(1002));

    auto* Save = NewObject<UTunaSweeperSaveGame>(); Save->DefeatedEnemyKinds = Game->DefeatedEnemyKinds.Array();
    Save->EverAcquiredItemIds = Game->EverAcquiredItemIds.Array();
    TArray<uint8> Bytes; TestTrue(TEXT("Save serialization"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reload = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Reloaded save"), Reload))
    { TestTrue(TEXT("Enemy persists"), Reload->DefeatedEnemyKinds.Contains(TEXT("quadruped"))); TestTrue(TEXT("Weapon persists"), Reload->EverAcquiredItemIds.Contains(1002)); }
    TestTrue(TEXT("Old/additive save default is empty"), NewObject<UTunaSweeperSaveGame>()->DefeatedEnemyKinds.IsEmpty());

    UClass* PanelClass = LoadClass<UEncyclopediaPanelGPT6Astra>(nullptr, TEXT("/Game/UI/Encyclopedia/WBP_EncyclopediaPanel_GPT6Astra.WBP_EncyclopediaPanel_GPT6Astra_C"));
    if (!TestNotNull(TEXT("Panel generated class"), PanelClass)) return false;
    TStrongObjectPtr<UEncyclopediaPanelGPT6Astra> Panel(CreateWidget<UEncyclopediaPanelGPT6Astra>(Game, PanelClass));
    auto Slate = Panel->TakeWidget();
    UWidget* Root = Panel->WidgetTree->RootWidget;
    auto* List = Cast<UListView>(Panel->GetWidgetFromName(TEXT("EntriesList")));
    auto* DetailImage = Cast<UImage>(Panel->GetWidgetFromName(TEXT("DetailPortrait")));
    auto* DetailName = Cast<UTextBlock>(Panel->GetWidgetFromName(TEXT("DetailName")));
    TestEqual(TEXT("All enemy kinds listed"), List->GetNumItems(), Catalog->Enemies.Num());
    List->SetSelectedIndex(0);
    TestEqual(TEXT("Unknown portrait is black with opaque tint"), DetailImage->GetColorAndOpacity(), FLinearColor(0,0,0,1));
    TestTrue(TEXT("Unknown name hidden"), DetailName->GetText().EqualTo(TunaSweeperEncyclopedia::Text(Game, TEXT("codex.unknown"))));

    for (const auto& Definition : Catalog->Enemies)
    {
        auto* Portrait = Definition.Portrait.LoadSynchronous();
        if (TestNotNull(TEXT("Every enemy has an authored portrait"), Portrait))
        {
            TArray64<uint8> Pixels; Portrait->Source.GetMipData(Pixels, 0);
            int32 Opaque = 0, Clear = 0;
            for (int64 I = 3; I < Pixels.Num(); I += 4) { if (Pixels[I] > 128) ++Opaque; if (Pixels[I] == 0) ++Clear; }
            TestTrue(*FString::Printf(TEXT("%s portrait foreground=%d transparent=%d"), *Definition.Kind.ToString(), Opaque, Clear), Opaque > 100 && Clear > 100);
        }
    }
    const TCHAR* Buttons[] = {TEXT("EnemyButton"), TEXT("WeaponButton"), TEXT("AmmoButton")};
    const TCHAR* Captures[] = {TEXT("enemies"), TEXT("weapons"), TEXT("ammunition")};
    Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
    for (int32 I = 0; I < 3; ++I)
    {
        Cast<UButton>(Panel->GetWidgetFromName(Buttons[I]))->OnClicked.Broadcast();
        if (I == 1)
            for (UObject* Object : List->GetListItems())
                if (auto* E = Cast<UEncyclopediaEntryData>(Object); E && E->ItemId == 1002)
                { List->SetSelectedItem(E); List->RequestScrollItemIntoView(E); break; }
        Panel->ForceLayoutPrepass();
        TestTrue(TEXT("Category switching retains serialized root"), Root == Panel->WidgetTree->RootWidget);
        if (FApp::CanEverRender())
        {
            FAssetCompilingManager::Get().FinishAllCompilation(); FWidgetRenderer Renderer(true);
            TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(1280,720)));
            FAssetCompilingManager::Get().FinishAllCompilation(); FlushRenderingCommands();
            Panel->InvalidateLayoutAndVolatility(); Panel->ForceLayoutPrepass();
            Renderer.DrawWidget(Target.Get(), Slate, FVector2D(1280,720), 0);
            FlushRenderingCommands(); FImage Image;
            if (FImageUtils::GetRenderTargetImage(Target.Get(), Image))
                TestTrue(TEXT("Rendered preview saved"), FImageUtils::SaveImageByExtension(
                    *FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Encyclopedia"), FString(Captures[I]) + TEXT(".png")), Image));
        }
    }
    for (int32 Language = 0; Language < 6; ++Language)
    {
        Game->SetCurrentTextLanguage(static_cast<ETunaSweeperItemTextLanguage>(Language), false);
        for (const auto& Definition : Catalog->Enemies)
            for (FName Key : {Definition.NameKey, Definition.DescriptionKey, Definition.BehaviorKey})
                TestFalse(TEXT("All enemy text resolves in every language"), TunaSweeperEncyclopedia::Text(Game, Key).IsEmpty());
        TestFalse(TEXT("Localized panel title"), Cast<UTextBlock>(Panel->GetWidgetFromName(TEXT("TitleText")))->GetText().IsEmpty());
    }
    Panel->RemoveFromParent(); Panel.Reset();
    Game->ResetRuntimeStateForSaveSlotSelection();
    TestTrue(TEXT("Slot change clears enemy history"), Game->DefeatedEnemyKinds.IsEmpty());
    TestTrue(TEXT("Slot change clears acquisition history"), Game->EverAcquiredItemIds.IsEmpty());
    Game->DefeatedEnemyKinds.Add(TEXT("guard"));
    Game->GenerateDefaultInventoryState();
    TestTrue(TEXT("New-game initialization clears enemy discovery"), Game->DefeatedEnemyKinds.IsEmpty());
    return true;
}
