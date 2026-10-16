#if WITH_EDITOR
#include "Game/TunaSweeperSaveToolInternal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace TunaSweeperSaveTool::Internal
{
    namespace
    {
        bool IsExcluded(FName Id, FName InfoId)
        {
            return Id.ToString().StartsWith(TEXT("demo.water_intake."), ESearchCase::IgnoreCase) ||
                InfoId == TEXT("world_progress.blocked_intake_screen") || InfoId == TEXT("world_progress.water_intake_valve");
        }

        FString BuildRows(const UTunaSweeperSaveGame& Save, TArray<FObject>& Rows)
        {
            FString Source;
            FObject Catalog;
            if (!FFileHelper::LoadFileToString(Source, *(FPaths::ProjectConfigDir() / TEXT("WorldProgressCheats.json"))) ||
                !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source), Catalog) || !Catalog)
                return TEXT("world_catalog_unavailable");
            int32 Version = 0;
            const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
            if (!Integer(Catalog, TEXT("version"), Version, 1, 1) || !Catalog->TryGetArrayField(TEXT("entries"), Entries))
                return TEXT("world_catalog_invalid");
            TSet<FName> CatalogIds;
            for (const auto& Value : *Entries)
            {
                if (Value->Type != EJson::Object) return TEXT("world_catalog_invalid");
                const auto Entry = Value->AsObject();
                FString Id, Info, NameKey, Flavor;
                int32 Required = 0;
                if (!Entry->TryGetStringField(TEXT("objectId"), Id) || FName(*Id).IsNone() ||
                    !Entry->TryGetStringField(TEXT("infoId"), Info) || !Entry->TryGetStringField(TEXT("nameKey"), NameKey) || NameKey.IsEmpty() ||
                    !Entry->TryGetStringField(TEXT("flavor"), Flavor) || (Flavor != TEXT("Main") && Flavor != TEXT("Demo") && Flavor != TEXT("Both")) ||
                    !Integer(Entry, TEXT("requiredQuantity"), Required, 1, MAX_int32) || CatalogIds.Contains(FName(*Id)) || IsExcluded(FName(*Id), FName(*Info)))
                    return TEXT("world_catalog_invalid");
                CatalogIds.Add(FName(*Id));
                if (Flavor != TEXT("Both") && Flavor != Save.BuildFlavor.ToString()) continue;
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("objectId"), Id);
                Row->SetStringField(TEXT("infoId"), Info);
                Row->SetStringField(TEXT("nameKey"), NameKey);
                Row->SetNumberField(TEXT("requiredQuantity"), Required);
                Row->SetNumberField(TEXT("maxQuantity"), Required);
                Row->SetBoolField(TEXT("recorded"), false);
                Row->SetStringField(TEXT("state"), TEXT("InProgress"));
                Row->SetNumberField(TEXT("quantity"), 0);
                Rows.Add(Row);
            }
            TSet<FName> SavedIds;
            for (const auto& State : Save.WorldProgressStates)
            {
                if (IsExcluded(State.ObjectId, State.InfoId)) continue;
                if (State.ObjectId.IsNone() || SavedIds.Contains(State.ObjectId) || State.ProgressQuantity < 0 ||
                    (State.State != ETunaSweeperWorldProgressState::InProgress && State.State != ETunaSweeperWorldProgressState::Completed))
                    return TEXT("world_state_invalid");
                SavedIds.Add(State.ObjectId);
                const auto* Existing = Rows.FindByPredicate([&](const FObject& Row) { return FName(*Row->GetStringField(TEXT("objectId"))) == State.ObjectId; });
                FObject Row = Existing ? *Existing : MakeShared<FJsonObject>();
                if (!Existing)
                {
                    Row->SetStringField(TEXT("objectId"), State.ObjectId.ToString());
                    Row->SetStringField(TEXT("nameKey"), TEXT("ui.world_cheat.saved_object"));
                    // Old/custom saved objects remain individually editable without inventing new IDs.
                    Row->SetNumberField(TEXT("requiredQuantity"), FMath::Max(1, State.ProgressQuantity));
                    Row->SetNumberField(TEXT("maxQuantity"), MAX_int32);
                    Rows.Add(Row);
                }
                Row->SetStringField(TEXT("infoId"), State.InfoId.ToString());
                Row->SetBoolField(TEXT("recorded"), true);
                Row->SetStringField(TEXT("state"), State.State == ETunaSweeperWorldProgressState::Completed ? TEXT("Completed") : TEXT("InProgress"));
                Row->SetNumberField(TEXT("quantity"), State.ProgressQuantity);
            }
            Rows.Sort([](const FObject& A, const FObject& B) { return A->GetStringField(TEXT("objectId")) < B->GetStringField(TEXT("objectId")); });
            return FString();
        }
    }

    FObject InspectWorldProgress(const UTunaSweeperSaveGame& Save)
    {
        TArray<FObject> Rows;
        const FString Error = BuildRows(Save, Rows);
        if (!Error.IsEmpty()) return Result(Error);
        TArray<TSharedPtr<FJsonValue>> Values;
        for (const auto& Row : Rows) Values.Add(MakeShared<FJsonValueObject>(Row));
        auto R = Result(); R->SetArrayField(TEXT("worldProgress"), Values); return R;
    }

    FString SetWorldProgress(UTunaSweeperSaveGame& Save, const FObject& Request)
    {
        FString Id, DesiredState;
        int32 Quantity = 0;
        if (!Request->TryGetStringField(TEXT("objectId"), Id) || FName(*Id).IsNone() ||
            !Request->TryGetStringField(TEXT("state"), DesiredState) ||
            (DesiredState != TEXT("InProgress") && DesiredState != TEXT("Completed")) ||
            !Integer(Request, TEXT("quantity"), Quantity, 0, MAX_int32)) return TEXT("invalid_request");
        TArray<FObject> Rows;
        const FString Error = BuildRows(Save, Rows);
        if (!Error.IsEmpty()) return Error;
        const auto* Definition = Rows.FindByPredicate([&](const FObject& Row) { return FName(*Row->GetStringField(TEXT("objectId"))) == FName(*Id); });
        if (!Definition) return TEXT("world_object_unavailable");
        if (Quantity > (*Definition)->GetIntegerField(TEXT("maxQuantity")) ||
            (DesiredState == TEXT("Completed") && Quantity < (*Definition)->GetIntegerField(TEXT("requiredQuantity"))))
            return TEXT("world_quantity_invalid");
        auto* State = Save.WorldProgressStates.FindByPredicate([&](const auto& Row) { return Row.ObjectId == FName(*Id); });
        if (!State)
        {
            State = &Save.WorldProgressStates.AddDefaulted_GetRef();
            State->ObjectId = FName(*Id);
            State->InfoId = FName(*(*Definition)->GetStringField(TEXT("infoId")));
        }
        // Keep an explicit incomplete record: production facilities migrate missing records from quest rewards.
        State->State = DesiredState == TEXT("Completed") ? ETunaSweeperWorldProgressState::Completed : ETunaSweeperWorldProgressState::InProgress;
        State->ProgressQuantity = Quantity;
        return FString();
    }
}
#endif
