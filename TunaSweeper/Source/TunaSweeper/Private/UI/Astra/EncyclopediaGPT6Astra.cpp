// Authored by gpt-6-astra; reasoning effort: medium. All hierarchy lives in saved WBPs.
#include "UI/Astra/EncyclopediaGPT6Astra.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

namespace
{
void ShowPortrait(UImage* Image, UEncyclopediaEntryData* Entry)
{
    if (!Image) return;
    Image->SetBrushFromTexture(Entry ? Entry->Portrait.LoadSynchronous() : nullptr);
    // Authored portrait alpha preserves the actual enemy outline, not a rectangular placeholder.
    Image->SetColorAndOpacity(Entry && Entry->bUnlocked ? FLinearColor::White : FLinearColor(0, 0, 0, 1));
}
FText Metadata(UTunaSweeperGameInstance* Game, UEncyclopediaEntryData* Entry)
{
    if (!Entry || !Entry->bUnlocked || Entry->Category == EEncyclopediaCategory::Enemy) return FText::GetEmpty();
    return FText::Format(TunaSweeperEncyclopedia::Text(Game, TEXT("codex.meta")), Entry->GroupName, Entry->GradeName);
}
}

void UEncyclopediaEntryGPT6Astra::NativeOnListItemObjectSet(UObject* Object)
{
    auto* Entry = Cast<UEncyclopediaEntryData>(Object);
    if (!Entry) return;
    TunaSweeperUIFont::ApplyFontToWidgetTree(this);
    NameText->SetText(Entry->Name);
    MetaText->SetText(Metadata(GetGameInstance<UTunaSweeperGameInstance>(), Entry));
    ShowPortrait(PortraitImage, Entry);
    NativeOnItemSelectionChanged(false);
}

void UEncyclopediaEntryGPT6Astra::NativeOnItemSelectionChanged(bool bSelected)
{
    if (RowBorder) RowBorder->SetBrushColor(bSelected ? FLinearColor(0.12, 0.27, 0.3, 1) : FLinearColor(0.045, 0.065, 0.08, 1));
}

UEncyclopediaPanelGPT6Astra::UEncyclopediaPanelGPT6Astra(const FObjectInitializer& Initializer) : Super(Initializer)
{
    SetIsFocusable(true);
}

void UEncyclopediaPanelGPT6Astra::NativeConstruct()
{
    Super::NativeConstruct();
    EnemyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowEnemies);
    WeaponButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowWeapons);
    AmmoButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowAmmo);
    CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Close);
    TypeFilter->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::FilterChanged);
    EntriesList->OnItemSelectionChanged().AddUObject(this, &ThisClass::SelectionChanged);
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
        LanguageHandle = Game->OnLanguageChanged.AddUObject(this, &ThisClass::Refresh);
    Refresh();
}

void UEncyclopediaPanelGPT6Astra::NativeDestruct()
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>()) Game->OnLanguageChanged.Remove(LanguageHandle);
    EntriesList->OnItemSelectionChanged().RemoveAll(this);
    Super::NativeDestruct();
}

void UEncyclopediaPanelGPT6Astra::Refresh()
{
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game) return;
    auto Text = [Game](const TCHAR* Key) { return TunaSweeperEncyclopedia::Text(Game, Key); };
    TitleText->SetText(Text(TEXT("codex.title")));
    EnemyText->SetText(Text(TEXT("codex.enemies")));
    WeaponText->SetText(Text(TEXT("codex.weapons")));
    AmmoText->SetText(Text(TEXT("codex.ammunition")));
    CloseText->SetText(Text(TEXT("codex.close")));
    TunaSweeperUIStyle::ApplyButton(CloseButton, TunaSweeperUIStyle::EButtonRole::Tab);
    TunaSweeperUIFont::ApplyFontToWidgetTree(this);
    TunaSweeperUIStyle::ApplyButton(EnemyButton, TunaSweeperUIStyle::EButtonRole::Tab, Category == EEncyclopediaCategory::Enemy);
    TunaSweeperUIStyle::ApplyButton(WeaponButton, TunaSweeperUIStyle::EButtonRole::Tab, Category == EEncyclopediaCategory::Weapon);
    TunaSweeperUIStyle::ApplyButton(AmmoButton, TunaSweeperUIStyle::EButtonRole::Tab, Category == EEncyclopediaCategory::Ammunition);
    TArray<UEncyclopediaEntryData*> Built;
    TunaSweeperEncyclopedia::BuildEntries(Game, this, Category, Built);
    Entries.Reset();
    for (auto* Entry : Built) Entries.Add(Entry);
    const int32 PreviousIndex = TypeFilter->GetSelectedIndex();
    const FName PreviousGroup = FilterGroups.IsValidIndex(PreviousIndex) ? FilterGroups[PreviousIndex] : NAME_None;
    bUpdatingFilter = true;
    TypeFilter->ClearOptions();
    FilterGroups = {NAME_None};
    TypeFilter->AddOption(Text(TEXT("codex.all_types")).ToString());
    for (auto* Entry : Built) if (!Entry->Group.IsNone() && !FilterGroups.Contains(Entry->Group))
    {
        FilterGroups.Add(Entry->Group);
        TypeFilter->AddOption(Entry->GroupName.ToString());
    }
    TypeFilter->SetSelectedIndex(FMath::Max(0, FilterGroups.IndexOfByKey(PreviousGroup)));
    TypeFilter->SetVisibility(Category == EEncyclopediaCategory::Enemy ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    bUpdatingFilter = false;
    ApplyFilter();
}

void UEncyclopediaPanelGPT6Astra::ApplyFilter()
{
    const int32 Index = TypeFilter->GetSelectedIndex();
    const FName Filter = FilterGroups.IsValidIndex(Index) ? FilterGroups[Index] : NAME_None;
    TArray<UObject*> Visible;
    TSet<FString> Total, Unlocked;
    for (const auto& Entry : Entries)
    {
        if (!Filter.IsNone() && Entry->Group != Filter) continue;
        Visible.Add(Entry);
        const FString Key = Entry->Category == EEncyclopediaCategory::Enemy ? Entry->EnemyKind.ToString() : FString::FromInt(Entry->ItemId);
        Total.Add(Key);
        if (Entry->bUnlocked) Unlocked.Add(Key);
    }
    EntriesList->SetListItems(Visible);
    ProgressText->SetText(FText::Format(TunaSweeperEncyclopedia::Text(GetGameInstance<UTunaSweeperGameInstance>(), TEXT("codex.progress")),
        FText::AsNumber(Unlocked.Num()), FText::AsNumber(Total.Num())));
    if (Visible.Num()) EntriesList->SetSelectedItem(Visible[0]);
    SelectionChanged(Visible.Num() ? Visible[0] : nullptr);
}

void UEncyclopediaPanelGPT6Astra::SelectionChanged(UObject* Object)
{
    auto* Entry = Cast<UEncyclopediaEntryData>(Object);
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    DetailName->SetText(Entry ? Entry->Name : TunaSweeperEncyclopedia::Text(Game, TEXT("codex.empty")));
    DetailMeta->SetText(Metadata(Game, Entry));
    DetailDescription->SetText(Entry ? Entry->Description : FText::GetEmpty());
    DetailBehavior->SetText(Entry ? Entry->Details : FText::GetEmpty());
    ShowPortrait(DetailPortrait, Entry);
}

void UEncyclopediaPanelGPT6Astra::ShowEnemies() { Category = EEncyclopediaCategory::Enemy; Refresh(); }
void UEncyclopediaPanelGPT6Astra::ShowWeapons() { Category = EEncyclopediaCategory::Weapon; Refresh(); }
void UEncyclopediaPanelGPT6Astra::ShowAmmo() { Category = EEncyclopediaCategory::Ammunition; Refresh(); }
void UEncyclopediaPanelGPT6Astra::FilterChanged(FString, ESelectInfo::Type) { if (!bUpdatingFilter) ApplyFilter(); }
void UEncyclopediaPanelGPT6Astra::Close()
{
    if (auto* Player = Cast<ATunaSweeperPlayerController>(GetOwningPlayer())) Player->CloseEncyclopedia();
}
FReply UEncyclopediaPanelGPT6Astra::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::J) { Close(); return FReply::Handled(); }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UEncyclopediaLauncherGPT6Astra::NativeConstruct()
{
    Super::NativeConstruct();
    OpenButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Open);
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
        LanguageHandle = Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshText);
    RefreshText();
}
void UEncyclopediaLauncherGPT6Astra::NativeDestruct()
{
    if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>()) Game->OnLanguageChanged.Remove(LanguageHandle);
    Super::NativeDestruct();
}
void UEncyclopediaLauncherGPT6Astra::RefreshText()
{
    OpenText->SetText(TunaSweeperEncyclopedia::Text(GetGameInstance<UTunaSweeperGameInstance>(), TEXT("codex.launcher")));
    TunaSweeperUIFont::ApplyFontToWidgetTree(this);
    TunaSweeperUIStyle::ApplyButton(OpenButton, TunaSweeperUIStyle::EButtonRole::Tab);
}
void UEncyclopediaLauncherGPT6Astra::Open()
{
    if (auto* Player = Cast<ATunaSweeperPlayerController>(GetOwningPlayer())) Player->ToggleEncyclopedia();
}
