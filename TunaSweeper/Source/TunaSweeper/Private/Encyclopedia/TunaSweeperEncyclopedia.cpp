#include "Encyclopedia/TunaSweeperEncyclopedia.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

UEncyclopediaEnemyCatalog* UEncyclopediaEnemyCatalog::Get()
{
    return LoadObject<UEncyclopediaEnemyCatalog>(nullptr,
        TEXT("/Game/UI/Encyclopedia/DA_EncyclopediaEnemies.DA_EncyclopediaEnemies"));
}

FName UEncyclopediaEnemyCatalog::ResolveKind(const UClass* Class) const
{
    // Compare the already-loaded ancestry; a kill must not synchronously load every boss class.
    for (const UClass* Ancestor = Class; Ancestor; Ancestor = Ancestor->GetSuperClass())
    {
        const FSoftObjectPath Path(Ancestor);
        for (const auto& Entry : Enemies)
            if (Entry.EnemyClass.ToSoftObjectPath() == Path) return Entry.Kind;
    }
    return NAME_None;
}

EEncyclopediaCategory TunaSweeperEncyclopedia::Classify(const FTunaSweeperItemDefinition& Item)
{
    // Exact categories, never compatibility metadata: attachments also have weapon type tags.
    if (!Item.AttachmentSlotTag.IsNone()) return EEncyclopediaCategory::Excluded;
    if (Item.CategoryTag == TEXT("item.category.weapon.gun") || Item.CategoryTag == TEXT("item.category.weapon.melee"))
        return EEncyclopediaCategory::Weapon;
    if (Item.CategoryTag == TEXT("item.category.ammo")) return EEncyclopediaCategory::Ammunition;
    return EEncyclopediaCategory::Excluded;
}

FText TunaSweeperEncyclopedia::Text(UTunaSweeperGameInstance* Game, FName Key)
{
    FText Result;
    if (Game) if (auto* Strings = Game->GetSubsystem<UTunaSweeperTextSubsystem>())
        Strings->TryGetTextByKey(Key, Game->GetCurrentTextLanguage(), Result);
    return Result;
}

void TunaSweeperEncyclopedia::BuildEntries(UTunaSweeperGameInstance* Game, UObject* Outer,
    EEncyclopediaCategory Category, TArray<UEncyclopediaEntryData*>& Out)
{
    Out.Reset();
    if (!Game || !Outer) return;
    if (Category == EEncyclopediaCategory::Enemy)
    {
        if (auto* Catalog = UEncyclopediaEnemyCatalog::Get()) for (const auto& Definition : Catalog->Enemies)
        {
            auto* Entry = NewObject<UEncyclopediaEntryData>(Outer);
            Entry->Category = Category;
            Entry->EnemyKind = Definition.Kind;
            Entry->bUnlocked = Game->HasDefeatedEnemyKind(Definition.Kind);
            Entry->Portrait = Definition.Portrait;
            Entry->Name = Text(Game, Entry->bUnlocked ? Definition.NameKey : FName(TEXT("codex.unknown")));
            Entry->Description = Text(Game, Entry->bUnlocked ? Definition.DescriptionKey : FName(TEXT("codex.unlock.enemy")));
            if (Entry->bUnlocked) Entry->Details = Text(Game, Definition.BehaviorKey);
            Out.Add(Entry);
        }
        return;
    }
    auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
    TArray<FTunaSweeperItemDefinition> Definitions;
    if (!Items || !Items->GetAllItemDefinitions(Definitions)) return;
    for (const auto& Definition : Definitions)
    {
        if (Classify(Definition) != Category) continue;
        TArray<FName> Groups;
        if (Category == EEncyclopediaCategory::Weapon)
            Groups.Add(Definition.WeaponTypeTag.IsNone() ? FName(TEXT("weapon.type.melee")) : Definition.WeaponTypeTag);
        else
        {
            // A cartridge may be usable by multiple weapon types (pistol + SMG).
            for (const auto& Weapon : Definitions)
                if (Classify(Weapon) == EEncyclopediaCategory::Weapon &&
                    Weapon.CompatibleAmmoTypeTags.Contains(Definition.AmmoTypeTag))
                    Groups.AddUnique(Weapon.WeaponTypeTag);
            if (Groups.IsEmpty()) Groups.Add(TEXT("weapon.type.other"));
        }
        for (FName Group : Groups)
        {
            auto* Entry = NewObject<UEncyclopediaEntryData>(Outer);
            Entry->Category = Category;
            Entry->ItemId = Definition.Id;
            Entry->Group = Group;
            Entry->GroupName = Text(Game, FName(*(TEXT("codex.") + Group.ToString())));
            Entry->Grade = Definition.ItemGrade;
            const TCHAR* GradeKeys[] = {TEXT("codex.grade.common"), TEXT("codex.grade.uncommon"),
                TEXT("codex.grade.rare"), TEXT("codex.grade.epic"), TEXT("codex.grade.legendary")};
            Entry->GradeName = Text(Game, GradeKeys[FMath::Clamp(int32(Entry->Grade), 0, 4)]);
            Entry->bUnlocked = Game->HasEverAcquiredItem(Definition.Id);
            FText ActualName;
            Items->TryGetItemNameText(Definition.Id, Game->GetCurrentTextLanguage(), ActualName);
            Entry->SortName = ActualName.ToString();
            Entry->Name = Entry->bUnlocked ? ActualName : Text(Game, TEXT("codex.unknown"));
            if (Entry->bUnlocked)
                Items->TryGetItemDescriptionText(Definition.Id, Game->GetCurrentTextLanguage(), Entry->Description);
            else Entry->Description = Text(Game, TEXT("codex.unlock.item"));
            if (Entry->bUnlocked && Category == EEncyclopediaCategory::Ammunition)
                Entry->Details = FText::Format(Text(Game, TEXT("codex.ammo_stats")), FText::AsNumber(Definition.PenetrationTier),
                    FText::AsNumber(Definition.ProjectileDamageMultiplier / 100.0));
            else if (Entry->bUnlocked && Definition.MagazineCapacity > 0)
                Entry->Details = FText::Format(Text(Game, TEXT("codex.gun_stats")), FText::AsNumber(Definition.MagazineCapacity),
                    FText::AsNumber(Definition.ReloadSeconds));
            Entry->Portrait = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Items->BuildItemIconObjectPath(Definition)));
            Out.Add(Entry);
        }
    }
    Out.Sort([](const auto& A, const auto& B)
    {
        if (A.Group != B.Group) return A.Group.LexicalLess(B.Group);
        if (A.Grade != B.Grade) return A.Grade < B.Grade;
        const int32 Order = A.SortName.Compare(B.SortName);
        return Order == 0 ? A.ItemId < B.ItemId : Order < 0;
    });
}
