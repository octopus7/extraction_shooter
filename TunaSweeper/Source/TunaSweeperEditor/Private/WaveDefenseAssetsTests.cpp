#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "WaveDefense/WaveDefenseSpawnPoint.h"
#include "WaveDefense/WaveDefenseCore.h"
#include "WaveDefense/WaveDefenseTerminal.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseSavedAssetsTest, "TunaSweeper.WaveDefense.Assets.Saved", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseSavedAssetsTest::RunTest(const FString& Parameters)
{
    auto* Panel = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/MainRaid/WaveDefense/UI/WBP_WaveDefensePanel_GPT61Terra.WBP_WaveDefensePanel_GPT61Terra"));
    auto* HUD = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/MainRaid/WaveDefense/UI/WBP_WaveDefenseHUD_GPT61Terra.WBP_WaveDefenseHUD_GPT61Terra"));
    if (TestNotNull(TEXT("Panel Blueprint"), Panel) && TestNotNull(TEXT("Panel editable tree"), Panel->WidgetTree.Get()))
    {
        for (const TCHAR* Name : {TEXT("TitleText"),TEXT("IntroText"),TEXT("StatusText"),TEXT("DetailText"),TEXT("MessageText"),TEXT("StartButton"),TEXT("ReadyButton"),TEXT("AbortButton"),TEXT("ClaimButton"),TEXT("CloseButton"),TEXT("BarrierButton"),TEXT("TurretButton"),TEXT("RepairCoreButton"),TEXT("RepairFacilityButton"),TEXT("ReloadButton"),TEXT("NorthBarrierButton"),TEXT("WestBarrierButton"),TEXT("EastBarrierButton"),TEXT("NorthTurretButton"),TEXT("WestTurretButton"),TEXT("EastTurretButton")})
            TestNotNull(*FString::Printf(TEXT("Panel widget %s"), Name), Panel->WidgetTree->FindWidget(FName(Name)));
        TestTrue(TEXT("Panel compiled"), Panel->GeneratedClass && Panel->Status != BS_Error);
    }
    if (TestNotNull(TEXT("HUD Blueprint"), HUD) && TestNotNull(TEXT("HUD editable tree"), HUD->WidgetTree.Get()))
    { TestNotNull(TEXT("HUD status"), HUD->WidgetTree->FindWidget(TEXT("StatusText"))); TestNotNull(TEXT("HUD detail"), HUD->WidgetTree->FindWidget(TEXT("DetailText"))); TestTrue(TEXT("HUD compiled"), HUD->GeneratedClass && HUD->Status != BS_Error); }
    UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/MainRaid/RaidPlains"));
    if (!TestNotNull(TEXT("Saved Plains"), World)) return false;
    AWaveDefenseDirector* Director = nullptr; int32 DirectorCount = 0, CoreCount = 0, TerminalCount = 0;
    for (TActorIterator<AWaveDefenseDirector> It(World); It; ++It) { Director = *It; ++DirectorCount; }
    for (TActorIterator<AWaveDefenseCore> It(World); It; ++It) ++CoreCount;
    for (TActorIterator<AWaveDefenseTerminal> It(World); It; ++It) ++TerminalCount;
    TestEqual(TEXT("One director"), DirectorCount, 1); TestEqual(TEXT("One core"), CoreCount, 1); TestEqual(TEXT("One terminal"), TerminalCount, 1);
    if (!Director) return false;
    TestNotNull(TEXT("Core reference"), Director->GetCore());
    const auto* Definition = Director->GetDefinition();
    if (TestNotNull(TEXT("Definition reference"), Definition))
    {
        TArray<FName> Errors; TestTrue(TEXT("Definition validates"), Definition->ValidateDefinition(Errors));
        TestEqual(TEXT("Stable encounter id"), Definition->EncounterId, FName(TEXT("plains.wave_defense.01")));
        const int32 Counts[] = {6,8,10,12,14,16}; const int32 Caps[] = {4,5,6,7,8,8};
        TestEqual(TEXT("Six waves"), Definition->Waves.Num(), 6);
        for (int32 Index = 0; Index < FMath::Min(6,Definition->Waves.Num()); ++Index)
        { TestEqual(TEXT("Wave count"), Definition->Waves[Index].GetCount(), Counts[Index]); TestEqual(TEXT("Wave cap"), Definition->Waves[Index].AliveCap, Caps[Index]); }
    }
    TestEqual(TEXT("Six spawn candidates"), Director->GetSpawnPoints().Num(), 6);
    TestEqual(TEXT("Six facility slots"), Director->GetFacilitySlots().Num(), 6);
    TMap<FName,int32> Lanes; for (const auto& Point : Director->GetSpawnPoints()) if (Point) ++Lanes.FindOrAdd(Point->LaneId);
    for (FName Lane : {FName(TEXT("north")),FName(TEXT("west")),FName(TEXT("east"))}) TestEqual(TEXT("Two candidates per lane"), Lanes.FindRef(Lane), 2);
    return true;
}
#endif
