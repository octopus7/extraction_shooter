#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Title/TunaSweeperTitleAnimInstance.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "UnrealClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FCheckTitleMotionPresentation,FAutomationTestBase*,Test,FString,Name,ETunaSweeperTitleMotion,Expected);
bool FCheckTitleMotionPresentation::Update()
{
	UWorld* World=GEditor->PlayWorld;
	if (!Test->TestNotNull(TEXT("Title PIE world"),World)) return true;
	ATunaSweeperTitlePresentationActor* Title=nullptr;
	for (TActorIterator<ATunaSweeperTitlePresentationActor> It(World);It;++It) {Title=*It;break;}
	if (!Test->TestNotNull(TEXT("Live title actor"),Title)) return true;
	auto* Mesh=Title->FindComponentByClass<UTunaSweeperTitleSkeletalMeshComponent>();
	auto* Anim=Mesh?Cast<UTunaSweeperTitleAnimInstance>(Mesh->GetAnimInstance()):nullptr;
	if (!Test->TestNotNull(TEXT("Live title uses dedicated animation"),Anim)) return true;
	Test->TestEqual(TEXT("Live title motion phase"),Anim->CurrentMotion,Expected);
	Test->AddInfo(FString::Printf(TEXT("%s: phase=%d clip time=%.3f head alpha=%.3f"),*Name,int32(Anim->CurrentMotion),Anim->CurrentSequenceTime,Anim->HeadLookAlpha));
	if (Expected==ETunaSweeperTitleMotion::Entrance) {Anim->MinimumIdleLoops=1;Anim->MaximumIdleLoops=1;}
	if (Expected==ETunaSweeperTitleMotion::IdleB)
	{
		const auto Time=Anim->CurrentSequenceTime;
		Title->SetMainMenuPresentationActive(false);Title->SetMainMenuPresentationActive(true);
		Test->TestEqual(TEXT("Returning to menu does not replay entrance"),Anim->CurrentMotion,ETunaSweeperTitleMotion::IdleB);
		Test->TestEqual(TEXT("Menu navigation preserves playback time"),Anim->CurrentSequenceTime,Time);
	}
	auto* Viewport=GEditor->GetPIEViewport();
	if (!Test->TestNotNull(TEXT("Live title viewport"),Viewport)) return true;
	TArray<FColor> Pixels;
	if (Test->TestTrue(TEXT("Title capture readable"),Viewport->ReadPixels(Pixels)))
	{
		const auto Size=Viewport->GetSizeXY();TArray<uint8> Png;
		FImageUtils::CompressImageArray(Size.X,Size.Y,Pixels,Png);
		Test->TestTrue(TEXT("Title capture saved"),FFileHelper::SaveArrayToFile(Png,*(FPaths::ProjectSavedDir()/TEXT("Screenshots")/Name)));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleMotionPresentationTest,"TunaSweeper.Title.Animation.Presentation",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTitleMotionPresentationTest::RunTest(const FString& Parameters)
{
	if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("Maps/IntroMap.umap"),false,true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.15f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleMotionPresentation(this,TEXT("TitleMotion_C_Start.png"),ETunaSweeperTitleMotion::Entrance));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleMotionPresentation(this,TEXT("TitleMotion_C_Turn.png"),ETunaSweeperTitleMotion::Entrance));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleMotionPresentation(this,TEXT("TitleMotion_A.png"),ETunaSweeperTitleMotion::IdleA));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(5.4f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleMotionPresentation(this,TEXT("TitleMotion_B.png"),ETunaSweeperTitleMotion::IdleB));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
