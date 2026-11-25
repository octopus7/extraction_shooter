#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperDemoStaminaTuningTest,
	"TunaSweeper.Player.Stamina.DemoTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperDemoStaminaTuningTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATunaSweeperTopDownCharacter* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>(
		ATunaSweeperTopDownCharacter::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Player character"), Character))
	{
		return false;
	}

	Character->MaxStamina = 100.0f;
	Character->CurrentStamina = 100.0f;
	Character->bSprintInputHeld = true;
	Character->CurrentMoveInput = FVector2D(1.0f, 0.0f);
	Character->UpdateSprintAndStamina(1.0f);
	TestEqual(TEXT("One second of sprint applies six whole stamina points"), Character->CurrentStamina, 94.0f);
	Character->UpdateSprintAndStamina(3.0f);
	TestEqual(TEXT("Four seconds preserve the exact 6.25 per-second depletion rate"), Character->CurrentStamina, 75.0f);

	Character->CurrentStamina = 0.0f;
	Character->StaminaRemainder = 0.0;
	Character->bSprintInputHeld = false;
	Character->UpdateSprintAndStamina(1.0f);
	TestEqual(TEXT("One second of demo recovery restores three times the previous stamina rate"), Character->CurrentStamina, 54.0f);
	for (const int32 FPS : {30, 60, 144})
	{
		Character->CurrentStamina = 100;
		Character->StaminaRemainder = 0;
		Character->bSprintInputHeld = true;
		for (int32 Frame = 0; Frame < FPS * 4; ++Frame)
		{
			Character->UpdateSprintAndStamina(1.f / FPS);
			TestEqual(TEXT("Stamina stays integer every frame"), Character->CurrentStamina, FMath::RoundToFloat(Character->CurrentStamina));
		}
		TestEqual(TEXT("Sprint rate is independent of frame rate"), Character->CurrentStamina, 75.f);
	}

	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Game->*Member = Context;
		}
	};
	FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
	Game->bSuppressSaveWritesForTests = true;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
	};
	Game->ResetPlayerSlotArrays();

	FTunaSweeperItemInstance Item;
	Item.Uid = FGuid::NewGuid();
	Item.ItemId = 3003;
	Item.Quantity = 2;
	Game->ItemInstancesByUid.Add(Item.Uid, Item);
	Game->PlayerInventorySlots[0].ItemUid = Item.Uid;
	FTunaSweeperItemSlotReference Slot;
	Slot.Source = ETunaSweeperItemSlotSource::Inventory;
	Slot.SlotIndex = 0;

	Character->CurrentStamina = 100.0f;
	Character->StaminaRemainder = 0.0;
	TestTrue(TEXT("Energy bar consumed through inventory"), Game->TryUseItemInSlot(Slot, Character));
	Character->UpdateSprintAndStamina(4.0f);
	TestEqual(TEXT("Energy bar reduces sprint stamina drain by 20 percent"), Character->CurrentStamina, 80.0f);

	Character->bSprintInputHeld = false;
	Character->UpdateSprintAndStamina(55.0f);
	TestTrue(TEXT("Second energy bar consumed"), Game->TryUseItemInSlot(Slot, Character));
	Character->bSprintInputHeld = true;
	Character->UpdateSprintAndStamina(4.0f);
	TestEqual(TEXT("Second bar refreshes duration without stacking"), Character->CurrentStamina, 80.0f);

	Character->bSprintInputHeld = false;
	Character->UpdateSprintAndStamina(56.0f);
	Character->bSprintInputHeld = true;
	Character->UpdateSprintAndStamina(4.0f);
	TestEqual(TEXT("Normal sprint drain returns when the buff expires"), Character->CurrentStamina, 75.0f);

	Item.ItemId = 3008;
	Game->ItemInstancesByUid.Add(Item.Uid, Item);
	Game->PlayerInventorySlots[0].ItemUid = Item.Uid;
	Character->bIsSprinting = false;
	Character->UpdateMovementSpeed();
	const float WalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	TestTrue(TEXT("Rabbit jelly consumed through inventory"), Game->TryUseItemInSlot(Slot, Character));
	TestEqual(TEXT("Rabbit jelly increases walking speed"), Character->GetCharacterMovement()->MaxWalkSpeed, WalkSpeed * 1.1f);
	Character->bIsSprinting = true;
	Character->UpdateMovementSpeed();
	TestEqual(TEXT("Rabbit jelly increases sprinting speed"), Character->GetCharacterMovement()->MaxWalkSpeed,
		WalkSpeed * Character->SprintSpeedMultiplier * 1.1f);
	Character->RabbitJellySpeedBuffEndTime = World->GetTimeSeconds() + 1.0;
	TestTrue(TEXT("Second rabbit jelly consumed"), Game->TryUseItemInSlot(Slot, Character));
	TestEqual(TEXT("Second jelly refreshes for 30 seconds"), Character->RabbitJellySpeedBuffEndTime,
		static_cast<double>(World->GetTimeSeconds()) + 30.0);
	TestEqual(TEXT("Jelly speed bonuses do not stack"), Character->GetCharacterMovement()->MaxWalkSpeed,
		WalkSpeed * Character->SprintSpeedMultiplier * 1.1f);
	Character->RabbitJellySpeedBuffEndTime = World->GetTimeSeconds();
	Character->UpdateMovementSpeed();
	TestEqual(TEXT("Jelly expiry restores normal sprint speed"), Character->GetCharacterMovement()->MaxWalkSpeed,
		WalkSpeed * Character->SprintSpeedMultiplier);

	return true;
}

#endif
