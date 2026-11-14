#include "Game/TunaSweeperGameInstance.h"
#include "Encyclopedia/TunaSweeperEncyclopedia.h"

bool UTunaSweeperGameInstance::HasDefeatedEnemyKind(FName Kind)
{
    EnsureInventoryStateInitialized();
    return !Kind.IsNone() && DefeatedEnemyKinds.Contains(Kind);
}

bool UTunaSweeperGameInstance::RecordEncyclopediaKill(const UClass* EnemyClass, bool bSaveImmediately)
{
    // Practice/laboratory kills must not change the player's real discoveries.
    if (!EnemyClass || IsCombatTestSession()) return false;
    auto* Catalog = UEncyclopediaEnemyCatalog::Get();
    const FName Kind = Catalog ? Catalog->ResolveKind(EnemyClass) : NAME_None;
    if (Kind.IsNone()) return false;
    EnsureInventoryStateInitialized();
    if (DefeatedEnemyKinds.Contains(Kind)) return false;
    DefeatedEnemyKinds.Add(Kind);
    // Keep the in-memory discovery on disk failure; subsequent ordinary saves retry it.
    if (bSaveImmediately) SaveGameState();
    return true;
}
