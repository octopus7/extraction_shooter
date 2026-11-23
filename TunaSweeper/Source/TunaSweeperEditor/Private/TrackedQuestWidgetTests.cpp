#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "WidgetBlueprint.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperQuestWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrackedQuestWidgetLayoutTest, "TunaSweeper.Quest.TrackedWidgetLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrackedQuestWidgetLayoutTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("WBP_Quest"), TEXT("WBP_QuestMenu"), TEXT("WBP_QuestInteraction")})
	{
		const FString Path = FString::Printf(TEXT("/Game/UI/%s.%s"), Name, Name);
		const auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *Path);
		if (!TestNotNull(Path, Blueprint)) return false;
		const auto* Tree = Blueprint->WidgetTree.Get();
		const auto* Detail = Cast<UVerticalBox>(Tree->FindWidget(TEXT("QuestDetailStack")));
		const auto* Footer = Cast<UHorizontalBox>(Tree->FindWidget(TEXT("QuestActionRow")));
		const auto* Track = Cast<UButton>(Tree->FindWidget(TEXT("QuestTrackButton")));
		if (!TestNotNull(TEXT("Detail stack"), Detail) || !TestNotNull(TEXT("Footer"), Footer) ||
			!TestNotNull(TEXT("Track button"), Track)) return false;
		TestTrue(TEXT("Footer is last in detail panel"), Detail->GetChildAt(Detail->GetChildrenCount()-1) == Footer);
		TestTrue(TEXT("Track button starts footer"), Footer->GetChildAt(0) == Track);
		TestTrue(TEXT("Primary action ends footer"), Footer->GetChildAt(Footer->GetChildrenCount()-1) == Tree->FindWidget(TEXT("QuestPrimaryButton")));
		TestNotNull(TEXT("Track label"), Cast<UTextBlock>(Track->GetContent()));
		auto* Instance = NewObject<UTunaSweeperQuestWidget>(GetTransientPackage(), Blueprint->GeneratedClass);
		Instance->Initialize();
		UWidget* AuthoredTrack = Instance->GetWidgetFromName(TEXT("QuestTrackButton"));
		if (!TestNotNull(TEXT("Compiled quest button"), AuthoredTrack)) return false;
		Instance->RefreshQuestView();
		TestTrue(TEXT("Refreshing retains authored layout"), Instance->GetWidgetFromName(TEXT("QuestTrackButton")) == AuthoredTrack);
	}
	const auto* Hud = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/UI/WBP_GameHud.WBP_GameHud"));
	if (!TestNotNull(TEXT("HUD"), Hud)) return false;
	const auto* Tree = Hud->WidgetTree.Get();
	const auto* Panel = Cast<UBorder>(Tree->FindWidget(TEXT("TrackedQuestPanel")));
	const auto* Key = Cast<UButton>(Tree->FindWidget(TEXT("TrackedQuestMapButton")));
	if (!TestNotNull(TEXT("HUD panel"), Panel) || !TestNotNull(TEXT("Map key button"), Key)) return false;
	const auto* Slot = Cast<UCanvasPanelSlot>(Panel->Slot);
	if (!TestNotNull(TEXT("HUD anchor"), Slot)) return false;
	TestTrue(TEXT("HUD anchored top left"), Slot->GetAnchors().Minimum.IsNearlyZero() && Slot->GetAnchors().Maximum.IsNearlyZero());
	TestTrue(TEXT("Key label belongs to clickable button"), Key->GetContent() == Tree->FindWidget(TEXT("TrackedQuestMapKey")));
	TestNotNull(TEXT("HUD title"), Cast<UTextBlock>(Tree->FindWidget(TEXT("TrackedQuestTitle"))));
	TestNotNull(TEXT("HUD objective"), Cast<UTextBlock>(Tree->FindWidget(TEXT("TrackedQuestObjective"))));
	auto* Instance = NewObject<UTunaSweeperGameHudWidget>(GetTransientPackage(), Hud->GeneratedClass);
	Instance->Initialize();
	for (const TCHAR* Name : {TEXT("TrackedQuestPanel"), TEXT("TrackedQuestMapButton"), TEXT("TrackedQuestTitle"),
		TEXT("TrackedQuestObjective"), TEXT("TrackedQuestMapKey")})
	{
		const auto* Property = FindFProperty<FObjectPropertyBase>(Instance->GetClass(), Name);
		if (!TestNotNull(Name, Property)) return false;
		TestNotNull(Name, Property->GetObjectPropertyValue_InContainer(Instance));
	}
	return !HasAnyErrors();
}
#endif
