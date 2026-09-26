#include "BossLab/TunaSweeperBossDefinition.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	bool Fail(FName& Error, const TCHAR* Key) { Error = FName(Key); return false; }
	bool ReadInteger(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& Value)
	{
		double Number;
		if (!Object->TryGetNumberField(Field, Number) || !FMath::IsFinite(Number) || Number < MIN_int32 || Number > MAX_int32 || FMath::FloorToDouble(Number) != Number) return false;
		Value = static_cast<int32>(Number); return true;
	}
	FVector RotatedExtent(const FVector& Extent, const FQuat& Rotation)
	{
		return Rotation.RotateVector(FVector(Extent.X, 0, 0)).GetAbs()
			+ Rotation.RotateVector(FVector(0, Extent.Y, 0)).GetAbs()
			+ Rotation.RotateVector(FVector(0, 0, Extent.Z)).GetAbs();
	}
}

const TArray<FTunaSweeperBossModuleDefinition>& TunaSweeperBossDefinition::GetCatalog()
{
	static const TArray<FTunaSweeperBossModuleDefinition> Catalog = {
		{TEXT("core"), TEXT("ui.boss_lab.module.core"), FVector(160,140,110), FLinearColor(.18f,.55f,.65f), 1600.f, 0.f, false},
		{TEXT("frame"), TEXT("ui.boss_lab.module.frame"), FVector(85,65,65), FLinearColor(.30f,.35f,.40f), 300.f, 0.f, false},
		{TEXT("armor"), TEXT("ui.boss_lab.module.armor"), FVector(40,140,110), FLinearColor(.65f,.55f,.30f), 650.f, 0.f, false},
		{TEXT("drive"), TEXT("ui.boss_lab.module.drive"), FVector(140,110,65), FLinearColor(.25f,.30f,.34f), 450.f, 0.f, false},
		{TEXT("cannon"), TEXT("ui.boss_lab.module.cannon"), FVector(110,45,45), FLinearColor(.80f,.38f,.20f), 300.f, 12.f, true},
		{TEXT("missile"), TEXT("ui.boss_lab.module.missile"), FVector(85,60,60), FLinearColor(.65f,.25f,.28f), 260.f, 22.f, true},
		{TEXT("laser"), TEXT("ui.boss_lab.module.laser"), FVector(105,40,40), FLinearColor(.25f,.65f,.80f), 240.f, 18.f, true}
	};
	return Catalog;
}

const FTunaSweeperBossModuleDefinition* TunaSweeperBossDefinition::FindModule(FName Id)
{
	return GetCatalog().FindByPredicate([Id](const auto& Entry) { return Entry.Id == Id; });
}

FTunaSweeperBossDefinition TunaSweeperBossDefinition::MakeDefault()
{
	FTunaSweeperBossDefinition Definition;
	Definition.Parts = {
		{1, TEXT("core"), INDEX_NONE, 0, 0},
		{2, TEXT("cannon"), 1, 2, 0},
		{3, TEXT("cannon"), 1, 3, 0},
		{4, TEXT("armor"), 1, 0, 0},
		{5, TEXT("drive"), 1, 5, 0},
		{6, TEXT("missile"), 1, 4, 0}
	};
	return Definition;
}

bool TunaSweeperBossDefinition::BuildTransforms(const FTunaSweeperBossDefinition& Definition, TMap<int32, FTransform>& OutTransforms)
{
	OutTransforms.Reset();
	if (Definition.Parts.IsEmpty() || Definition.Parts.Num() > MaxParts) return false;
	TSet<int32> Ids;
	for (const auto& Part : Definition.Parts)
	{
		if (Ids.Contains(Part.InstanceId) || !FindModule(Part.ModuleId) || Part.SocketIndex < 0 || Part.SocketIndex >= 6) return false;
		Ids.Add(Part.InstanceId);
	}
	for (int32 Pass = 0; Pass < Definition.Parts.Num(); ++Pass)
	{
		for (const auto& Part : Definition.Parts)
		{
			if (OutTransforms.Contains(Part.InstanceId)) continue;
			const FQuat Rotation(FRotator(0, Part.YawSteps * 90.f, 0));
			if (Part.ParentId == INDEX_NONE) { OutTransforms.Add(Part.InstanceId, FTransform(Rotation)); continue; }
			const FTransform* ParentTransform = OutTransforms.Find(Part.ParentId);
			const auto* Parent = Definition.Parts.FindByPredicate([&Part](const auto& P) { return P.InstanceId == Part.ParentId; });
			if (!Parent || !ParentTransform) continue;
			const FVector ParentExtent = FindModule(Parent->ModuleId)->HalfExtent;
			const FVector ChildExtent = RotatedExtent(FindModule(Part.ModuleId)->HalfExtent, Rotation);
			const int32 Axis = Part.SocketIndex / 2;
			FVector Offset = FVector::ZeroVector;
			// x front/back, y left/right, z top/bottom.
			const float Sign = Part.SocketIndex == 2 ? -1.f : Part.SocketIndex == 3 ? 1.f : (Part.SocketIndex % 2 == 0 ? 1.f : -1.f);
			Offset[Axis] = Sign * (ParentExtent[Axis] + ChildExtent[Axis] + 10.f);
			OutTransforms.Add(Part.InstanceId, FTransform(Rotation, Offset) * *ParentTransform);
		}
		if (OutTransforms.Num() == Definition.Parts.Num()) return true;
	}
	return false;
}

bool TunaSweeperBossDefinition::Validate(const FTunaSweeperBossDefinition& Definition, FName& OutError)
{
	OutError = NAME_None;
	if (Definition.Version != 1 || Definition.CatalogVersion != 1) return Fail(OutError, TEXT("ui.boss_lab.error.unsupported_version"));
	if (!Definition.BossId.IsValid()) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition"));
	if (Definition.Name.Len() > 64 || Definition.Parts.Num() > MaxParts) return Fail(OutError, TEXT("ui.boss_lab.error.limit"));
	for (TCHAR Character : Definition.Name) if (Character < 32 || Character == 127) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition"));
	if (Definition.Parts.IsEmpty()) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
	if (static_cast<uint8>(Definition.Tactic) > 2 || !FMath::IsFinite(Definition.AttackInterval) || !FMath::IsFinite(Definition.PhaseThreshold)
		|| Definition.AttackInterval < 1.f || Definition.AttackInterval > 8.f || Definition.PhaseThreshold < .1f || Definition.PhaseThreshold > .9f)
		return Fail(OutError, TEXT("ui.boss_lab.error.invalid_tactics"));
	TMap<int32, const FTunaSweeperBossPart*> ById;
	TSet<uint64> Occupied;
	int32 Roots = 0;
	for (const auto& Part : Definition.Parts)
	{
		if (Part.InstanceId <= 0 || Part.InstanceId > 1000000 || ById.Contains(Part.InstanceId) || !FindModule(Part.ModuleId)
			|| Part.SocketIndex < 0 || Part.SocketIndex > 5 || Part.YawSteps < 0 || Part.YawSteps > 3)
			return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
		ById.Add(Part.InstanceId, &Part);
		if (Part.ParentId == INDEX_NONE)
		{
			if (Part.ModuleId != TEXT("core")) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
			++Roots;
		}
		else
		{
			if (Part.ParentId <= 0 || Part.ModuleId == TEXT("core")) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
			const uint64 SocketKey = (uint64(Part.ParentId) << 3) | uint64(Part.SocketIndex);
			if (Occupied.Contains(SocketKey)) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
			Occupied.Add(SocketKey);
		}
	}
	if (Roots != 1) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
	for (const auto& Part : Definition.Parts)
	{
		TSet<int32> Visited;
		const FTunaSweeperBossPart* Node = &Part;
		while (Node)
		{
			if (Visited.Contains(Node->InstanceId)) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
			Visited.Add(Node->InstanceId);
			if (Visited.Num() > MaxDepth) return Fail(OutError, TEXT("ui.boss_lab.error.limit"));
			if (Node->ParentId == INDEX_NONE) break;
			const auto* Found = ById.Find(Node->ParentId);
			if (!Found) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
			Node = *Found;
		}
	}
	TMap<int32, FTransform> Transforms;
	if (!BuildTransforms(Definition, Transforms)) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
	TArray<FBox> Boxes;
	for (const auto& Part : Definition.Parts)
	{
		const FTransform& Transform = Transforms[Part.InstanceId];
		const FVector Extent = RotatedExtent(FindModule(Part.ModuleId)->HalfExtent, Transform.GetRotation());
		const FVector Position = Transform.GetLocation();
		if ((Position.GetAbs() + Extent).GetMax() > 1600.f) return Fail(OutError, TEXT("ui.boss_lab.error.limit"));
		const FBox Box(Position - Extent + FVector(1.f), Position + Extent - FVector(1.f));
		for (const auto& Existing : Boxes) if (Existing.Intersect(Box)) return Fail(OutError, TEXT("ui.boss_lab.error.overlap"));
		Boxes.Add(Box);
	}
	return true;
}

bool TunaSweeperBossDefinition::FromJson(const FString& Json, FTunaSweeperBossDefinition& OutDefinition, FName& OutError)
{
	OutError = NAME_None;
	if (Json.Len() > MaxFileBytes || FTCHARToUTF8(*Json).Length() > MaxFileBytes) return Fail(OutError, TEXT("ui.boss_lab.error.limit"));
	// Reject extreme nesting before the general JSON reader can allocate a deep tree.
	int32 Depth = 0; bool bString = false; bool bEscape = false;
	for (TCHAR C : Json)
	{
		if (bString) { if (bEscape) bEscape = false; else if (C == TEXT('\\')) bEscape = true; else if (C == TEXT('"')) bString = false; continue; }
		if (C == TEXT('"')) bString = true;
		else if (C == TEXT('{') || C == TEXT('[')) { if (++Depth > 8) return Fail(OutError, TEXT("ui.boss_lab.error.limit")); }
		else if (C == TEXT('}') || C == TEXT(']')) { if (--Depth < 0) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition")); }
	}
	TSharedPtr<FJsonObject> Object;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object.IsValid()) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition"));
	FTunaSweeperBossDefinition Candidate;
	FString Id; int32 Tactic; double Interval, Threshold;
	const TArray<TSharedPtr<FJsonValue>>* Parts;
	if (!ReadInteger(Object, TEXT("version"), Candidate.Version) || !ReadInteger(Object, TEXT("catalog_version"), Candidate.CatalogVersion)
		|| !Object->TryGetStringField(TEXT("boss_id"), Id) || !FGuid::Parse(Id, Candidate.BossId) || !Object->TryGetStringField(TEXT("name"), Candidate.Name)
		|| !ReadInteger(Object, TEXT("tactic"), Tactic) || Tactic < 0 || Tactic > 2 || !Object->TryGetNumberField(TEXT("attack_interval"), Interval)
		|| !Object->TryGetNumberField(TEXT("phase_threshold"), Threshold) || !Object->TryGetBoolField(TEXT("alternate_weapons"), Candidate.bAlternateWeapons)
		|| !Object->TryGetArrayField(TEXT("parts"), Parts)) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition"));
	if (Parts->Num() > MaxParts) return Fail(OutError, TEXT("ui.boss_lab.error.limit"));
	Candidate.Tactic = static_cast<ETunaSweeperBossTactic>(Tactic);
	Candidate.AttackInterval = static_cast<float>(Interval); Candidate.PhaseThreshold = static_cast<float>(Threshold);
	for (const auto& Value : *Parts)
	{
		const TSharedPtr<FJsonObject>* PartObject;
		FTunaSweeperBossPart Part; FString ModuleId;
		if (!Value.IsValid() || !Value->TryGetObject(PartObject) || !PartObject->IsValid()
			|| !ReadInteger(*PartObject, TEXT("id"), Part.InstanceId) || !(*PartObject)->TryGetStringField(TEXT("module"), ModuleId) || ModuleId.Len() > 32
			|| !ReadInteger(*PartObject, TEXT("parent"), Part.ParentId) || !ReadInteger(*PartObject, TEXT("socket"), Part.SocketIndex)
			|| !ReadInteger(*PartObject, TEXT("yaw"), Part.YawSteps)) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
		// Do not intern arbitrary untrusted names before checking the finite catalog.
		const auto* Module = GetCatalog().FindByPredicate([&ModuleId](const auto& Entry) { return Entry.Id.ToString() == ModuleId; });
		if (!Module) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_parts"));
		Part.ModuleId = Module->Id; Candidate.Parts.Add(Part);
	}
	if (!Validate(Candidate, OutError)) return false;
	OutDefinition = MoveTemp(Candidate);
	return true;
}

bool TunaSweeperBossDefinition::ToJson(const FTunaSweeperBossDefinition& Definition, FString& OutJson, FName& OutError)
{
	if (!Validate(Definition, OutError)) return false;
	auto Object = MakeShared<FJsonObject>();
	Object->SetNumberField(TEXT("version"), Definition.Version);
	Object->SetNumberField(TEXT("catalog_version"), Definition.CatalogVersion);
	Object->SetStringField(TEXT("boss_id"), Definition.BossId.ToString(EGuidFormats::DigitsWithHyphensLower));
	Object->SetStringField(TEXT("name"), Definition.Name);
	Object->SetNumberField(TEXT("tactic"), static_cast<uint8>(Definition.Tactic));
	Object->SetNumberField(TEXT("attack_interval"), Definition.AttackInterval);
	Object->SetNumberField(TEXT("phase_threshold"), Definition.PhaseThreshold);
	Object->SetBoolField(TEXT("alternate_weapons"), Definition.bAlternateWeapons);
	TArray<TSharedPtr<FJsonValue>> Parts;
	auto Sorted = Definition.Parts;
	Sorted.Sort([](const auto& A, const auto& B) { return A.InstanceId < B.InstanceId; });
	for (const auto& Part : Sorted)
	{
		auto Entry = MakeShared<FJsonObject>();
		Entry->SetNumberField(TEXT("id"), Part.InstanceId); Entry->SetStringField(TEXT("module"), Part.ModuleId.ToString());
		Entry->SetNumberField(TEXT("parent"), Part.ParentId); Entry->SetNumberField(TEXT("socket"), Part.SocketIndex); Entry->SetNumberField(TEXT("yaw"), Part.YawSteps);
		Parts.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Object->SetArrayField(TEXT("parts"), Parts);
	FString Json;
	if (!FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Json))) return Fail(OutError, TEXT("ui.boss_lab.error.invalid_definition"));
	OutJson = MoveTemp(Json); return true;
}

void TunaSweeperBossDefinition::RemoveBranch(FTunaSweeperBossDefinition& Definition, int32 InstanceId)
{
	const auto* Part = Definition.Parts.FindByPredicate([InstanceId](const auto& P) { return P.InstanceId == InstanceId; });
	if (!Part || Part->ParentId == INDEX_NONE) return;
	TSet<int32> Removed { InstanceId };
	for (int32 Pass = 0; Pass < Definition.Parts.Num(); ++Pass)
		for (const auto& Entry : Definition.Parts) if (Removed.Contains(Entry.ParentId)) Removed.Add(Entry.InstanceId);
	Definition.Parts.RemoveAll([&Removed](const auto& P) { return Removed.Contains(P.InstanceId); });
}
