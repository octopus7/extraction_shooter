#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace
{
	struct FVehicleHUDWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FVehicleHUDWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperVehicleHUDLayoutTest,
	"TunaSweeper.Vehicle.HUDLayout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperVehicleHUDLayoutTest::RunTest(const FString& Parameters)
{
	UClass* HudClass = LoadClass<UTunaSweeperVehicleDismountWidget>(nullptr, TEXT("/Game/UI/Vehicle/WBP_ATV_HUD.WBP_ATV_HUD_C"));
	if (!TestNotNull(TEXT("Saved editable ATV HUD Blueprint"), HudClass)) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	auto* Instance = NewObject<UTunaSweeperGameInstance>(GEngine);
	FVehicleHUDWorldContextAccess::Attach(Instance, &Context);
	Context.OwningGameInstance = Instance;
	World->SetGameInstance(Instance);
	Instance->UGameInstance::Init();
	Instance->GetSubsystem<UTunaSweeperTextSubsystem>()->LoadTextData();
	auto* Widget = NewObject<UTunaSweeperVehicleDismountWidget>(World, HudClass);
	Widget->Initialize();
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>();
	Widget->SetVehicle(ATV);
	Widget->TakeWidget();
	auto* Panel = Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("DismountPanel")));
	auto* Label = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("DismountText")));
	auto* KeyText = Cast<UTextBlock>(Widget->WidgetTree->FindWidget(TEXT("DismountKeyText")));
	auto* Keycap = Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("DismountKeycap")));
	auto* Row = Cast<UHorizontalBox>(Widget->WidgetTree->FindWidget(TEXT("DismountRow")));
	auto* Bar = Cast<UProgressBar>(Widget->WidgetTree->FindWidget(TEXT("DurabilityBar")));
	TestTrue(TEXT("Serialized WBP tree supplies all runtime bindings"), Panel && Label && KeyText && Keycap && Row && Bar);
	if (Panel && Label && KeyText && Keycap && Row && Bar)
	{
		TestTrue(TEXT("Keycap follows text on the right"), Row->GetChildAt(0) == Label && Row->GetChildAt(1) == Keycap);
		TestEqual(TEXT("White interaction panel"), Panel->Background.TintColor.GetSpecifiedColor(), FLinearColor::White);
		TestEqual(TEXT("Matching keycap radius"), Keycap->Background.OutlineSettings.CornerRadii.X, 5.0);
		TestEqual(TEXT("Matching keycap outline"), Keycap->Background.OutlineSettings.Width, 1.0f);
		TestEqual(TEXT("Key is two font points below label"), KeyText->GetFont().Size, Label->GetFont().Size - 2);
		TestEqual(TEXT("Localized dismount label"), Label->GetText().ToString(), Instance->ResolveLocalizedText(TEXT("ui.vehicle.dismount"), FText::GetEmpty()).ToString());
		TestEqual(TEXT("Localized X key"), KeyText->GetText().ToString(), Instance->ResolveLocalizedText(TEXT("ui.key.x"), FText::GetEmpty()).ToString());
	}
	TestNull(TEXT("No interaction ring in HUD"), Widget->WidgetTree->FindWidget(TEXT("RingImage")));
	const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
	// Check the final engine slot, not just the intended SetAnchors argument.
	TestTrue(TEXT("Vehicle HUD stays anchored at bottom center after sizing"),
		Slot.Anchors.Minimum.Equals(FVector2D(0.5, 0.82), 0.001) &&
		Slot.Anchors.Maximum.Equals(FVector2D(0.5, 0.82), 0.001));
	const FVector2D Size(Slot.Offsets.Right, Slot.Offsets.Bottom);
	TestTrue(TEXT("Vehicle HUD reserves one extra text line above the hint"), Size.Equals(FVector2D(180, 86)));
	for (const FVector2D Viewport : {FVector2D(1280, 720), FVector2D(1920, 1080)})
	{
		const FVector2D TopLeft = Viewport * Slot.Anchors.Minimum - Size * Slot.Alignment + FVector2D(Slot.Offsets.Left, Slot.Offsets.Top);
		TestTrue(TEXT("Entire vehicle HUD lies inside the viewport"),
			TopLeft.X >= 0 && TopLeft.Y >= 0 && TopLeft.X + Size.X <= Viewport.X && TopLeft.Y + Size.Y <= Viewport.Y);
		TestTrue(TEXT("Bar rises 28px while hint retains previous vertical position"),
			FMath::IsNearlyEqual(TopLeft.Y, Viewport.Y * .82 - 29 - 28, .01) &&
			FMath::IsNearlyEqual(TopLeft.Y + 50, Viewport.Y * .82 - 29 + 22, .01));
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ATVHUDPreview")))
	{
		ATV->CurrentDurability = ATV->MaxDurability * 0.5f;
		// A linear render target preserves Slate's already converted vertex colors.
		FWidgetRenderer Renderer(false);
		const FVector2D Resolution(1280, 720);
		auto Canvas = SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot().Offset(Slot.Offsets).Anchors(Slot.Anchors).Alignment(Slot.Alignment)
			[Widget->TakeWidget()];
		for (const bool bHint : {false, true})
		{
			Widget->SetDismountHintVisible(bHint);
			auto* Target = Renderer.DrawWidget(Canvas, Resolution);
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags ReadFlags;
			ReadFlags.SetLinearToGamma(false); // Slate already rendered in gamma space.
			Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, ReadFlags);
			const FColor Filled = Pixels[540 * 1280 + 560];
			const FColor Empty = Pixels[540 * 1280 + 710];
			const FColor KeyBox = Pixels[590 * 1280 + 552];
			const FColor Outline = Pixels[540 * 1280 + 550];
			TestTrue(TEXT("Green durability is painted inside the screen"), Filled.G > Filled.R && Filled.G > Filled.B && Filled.A > 200);
			TestTrue(TEXT("Empty durability is opaque dark gray"), FMath::Abs(int32(Empty.G) - Empty.R) < 3 && Empty.R < 100 && Empty.A > 200);
			TestTrue(TEXT("X key box is painted only for the stationary hint"), bHint ? KeyBox.R > 200 && KeyBox.A > 200 : KeyBox.A == 0);
			TestTrue(TEXT("Durability has a darker opaque outline"), Outline.R < Empty.R && Outline.A > 200);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1280, 720, Pixels, PNG);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / (bHint ? TEXT("ATVRigWork/HUD_Stopped.png") : TEXT("ATVRigWork/HUD_Moving.png"))));
		}
	}
	UGameViewportSubsystem::Get()->RemoveWidget(Widget);
	Instance->UGameInstance::Shutdown();
	World->SetGameInstance(nullptr);
	Context.OwningGameInstance = nullptr;
	FVehicleHUDWorldContextAccess::Attach(Instance, nullptr);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperVehicleMountTest,
	"TunaSweeper.Vehicle.MountInteraction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperVehicleMountTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(FVector(0,0,45), FRotator::ZeroRotator, Spawn);
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0,150,90), FRotator::ZeroRotator, Spawn);
	auto* Other = World->SpawnActor<ATunaSweeperTopDownCharacter>(FVector(0,-150,90), FRotator::ZeroRotator, Spawn);
	auto* Mount = ATV->MountComponent.Get();
	Mount->EngineStartSound = nullptr;
	Mount->EngineIdleSound = nullptr;
	Mount->EngineStopSound = nullptr;
	const auto Collision = Player->GetCapsuleComponent()->GetCollisionEnabled();
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	auto* Interactions = World->GetSubsystem<UTunaSweeperInteractionSubsystem>();
	Player->SetActorLocation(FVector(1000,0,90));
	TestFalse(TEXT("Remote interaction cannot mount"), Mount->RequestInteraction(Player));
	Player->SetActorLocation(FVector(0,150,90));
	const auto MovementMode = Player->GetCharacterMovement()->MovementMode.GetValue();
	const FTransform OnFootTransform = Player->GetActorTransform();
	TestTrue(TEXT("No-fuel interaction consumes the input"), Mount->RequestInteraction(Player));
	TestFalse(TEXT("No-fuel interaction leaves the player on foot"), Player->IsMountedInVehicle());
	TestNull(TEXT("No-fuel interaction leaves the seat empty"), Mount->GetRider());
	TestNull(TEXT("No-fuel interaction does not attach the player"), Player->GetAttachParentActor());
	TestTrue(TEXT("No-fuel interaction preserves transform"), Player->GetActorTransform().Equals(OnFootTransform));
	TestEqual(TEXT("No-fuel interaction preserves movement"), Player->GetCharacterMovement()->MovementMode.GetValue(), MovementMode);
	TestEqual(TEXT("No-fuel interaction preserves collision"), Player->GetCapsuleComponent()->GetCollisionEnabled(), Collision);
	TestTrue(TEXT("Repeated no-fuel interaction is handled"), Mount->RequestInteraction(Player));
	TestFalse(TEXT("Repeated interaction still cannot mount"), Player->IsMountedInVehicle());
	TestTrue(TEXT("Direct mount API remains available for vehicle lifecycle tests"), Mount->TryMount(Player));
	TestTrue(TEXT("Player records the seat"), Player->GetVehicleMount() == Mount);
	TestTrue(TEXT("Possession stays on the original player"), Controller->GetPawn() == Player);
	TestTrue(TEXT("Player is attached to the vehicle"), Player->GetAttachParentActor() == ATV);
	TestEqual(TEXT("Character movement is disabled"), Player->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	TestFalse(TEXT("Occupied interaction is unavailable"), Interactions->CanOfferInteraction(Mount));
	TestFalse(TEXT("Second rider cannot take an occupied seat"), Mount->TryMount(Other));
	Mount->UpdateStationaryHint(1.4f, 0);
	TestFalse(TEXT("Hint waits for the full delay"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.11f, 0);
	TestTrue(TEXT("Hint appears when stationary"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.01f, 100);
	TestFalse(TEXT("Movement hides the hint immediately"), Mount->IsDismountHintVisible());
	Mount->UpdateStationaryHint(0.1f, 0);
	TestFalse(TEXT("Movement resets the entire delay"), Mount->IsDismountHintVisible());
	TestFalse(TEXT("No ground rejects dismount without releasing the rider"), Mount->TryDismount());
	TestTrue(TEXT("Failed dismount stays mounted"), Player->IsMountedInVehicle());
	// A real collision floor exercises ground traces and capsule clearance.
	auto* Floor = World->SpawnActor<AActor>();
	auto* FloorBox = NewObject<UBoxComponent>(Floor);
	Floor->SetRootComponent(FloorBox);
	FloorBox->SetBoxExtent(FVector(1000,1000,10));
	FloorBox->SetCollisionProfileName(TEXT("BlockAll"));
	FloorBox->RegisterComponent();
	Floor->SetActorLocation(FVector(0,0,-10));
	auto* Blocker = World->SpawnActor<AActor>();
	auto* BlockerBox = NewObject<UBoxComponent>(Blocker);
	Blocker->SetRootComponent(BlockerBox);
	BlockerBox->SetBoxExtent(FVector(400,400,100));
	BlockerBox->SetCollisionProfileName(TEXT("BlockAll"));
	BlockerBox->RegisterComponent();
	Blocker->SetActorLocation(FVector(0,0,110));
	TestFalse(TEXT("Blocked exit cannot place the rider through geometry"), Mount->TryDismount());
	Blocker->Destroy();
	TestTrue(TEXT("Safe ground permits dismount even before hint delay"), Mount->TryDismount());
	TestFalse(TEXT("Dismount clears both sides"), Player->IsMountedInVehicle() || Mount->GetRider());
	TestNull(TEXT("Dismount detaches player"), Player->GetAttachParentActor());
	TestEqual(TEXT("Collision restored"), Player->GetCapsuleComponent()->GetCollisionEnabled(), Collision);
	TestEqual(TEXT("Original movement mode restored"), int32(Player->GetCharacterMovement()->MovementMode.GetValue()), int32(MovementMode));
	Player->SetActorLocation(FVector(0,150,90));
	TestTrue(TEXT("Direct remount works"), Mount->TryMount(Player));
	Mount->ReleaseRiderForEndPlay();
	TestFalse(TEXT("Vehicle cleanup releases character state"), Player->IsMountedInVehicle());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
