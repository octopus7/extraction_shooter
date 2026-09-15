#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperScenarioSubsystem.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleSingleInteractionTest,
	"TunaSweeper.Character.Mole.SingleInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleSingleInteractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("MoleInteractionBunkerMap"), CreatePackage(TEXT("/Temp/MoleInteractionBunkerMap")));
	TestTrue(TEXT("Fixture uses the bunker interaction gate"), World->GetMapName().EndsWith(TEXT("BunkerMap")));
	UClass* MoleClass = LoadClass<ATunaSweeperMoleCompanionActor>(nullptr, TEXT("/Game/Characters/Mole/BP_Mole.BP_Mole_C"));
	if (!TestNotNull(TEXT("Existing mole Blueprint loads"), MoleClass))
	{
		World->DestroyWorld(false);
		return false;
	}
	auto* Mole = World->SpawnActor<ATunaSweeperMoleCompanionActor>(MoleClass);
	// Isolate option selection from quest progression and disk saves.
	auto* Fallback = FindFProperty<FNameProperty>(Mole->GetClass(), TEXT("QuestFallbackId"));
	Fallback->SetPropertyValue_InContainer(Mole, TEXT("test.mole.quest"));
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	TInlineComponentArray<UTunaSweeperInteractableComponent*> Components(Mole);
	int32 OfferedCount = 0;
	for (auto* Component : Components)
	{
		if (Interactions->CanOfferInteraction(Component))
		{
			++OfferedCount;
			TestEqual(TEXT("Only quest is offered, including on existing Blueprints"), Component->GetInteractionType(), ETunaSweeperInteractionType::Quest);
		}
	}
	TestEqual(TEXT("Mole has exactly one usable option"), OfferedCount, 1);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleUnseenDialogueTest,
	"TunaSweeper.Character.Mole.UnseenDialogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleUnseenDialogueTest::RunTest(const FString& Parameters)
{
	// A transient instance and deferred saving leave the player's save untouched.
	auto* GameInstance = NewObject<UTunaSweeperGameInstance>();
	auto* Scenarios = NewObject<UTunaSweeperScenarioSubsystem>(GameInstance);
	TestTrue(TEXT("Active scenario pack loads"), Scenarios->LoadScenarioData());
	FTunaSweeperScenarioPresentation Presentation;
	TestTrue(TEXT("Unseen mole introduction is available"), Scenarios->TryResolveScenario(TEXT("interaction.mole"), TEXT("BunkerMap"), false, Presentation));
	TestFalse(TEXT("Introduction has dialogue"), Presentation.DialogueLines.IsEmpty());
	TestFalse(TEXT("Introduction has a completion flag"), Presentation.CompletionFlag.IsNone());
	GameInstance->MarkScenarioProgressFlag(Presentation.CompletionFlag, false);
	TestFalse(TEXT("Completed introduction does not block quests by replaying"), Scenarios->TryResolveScenario(TEXT("interaction.mole"), TEXT("BunkerMap"), false, Presentation));
	return true;
}
#endif
