#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/BodySetup.h"
#if WITH_EDITOR
#include "Rendering/SkeletalMeshModel.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#endif
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "AIController.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "UObject/UnrealType.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVDamageTest,
	"TunaSweeper.Vehicle.DamageAndDestruction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVDamageTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	auto* Ground = World->SpawnActor<AActor>();
	auto* Floor = NewObject<UBoxComponent>(Ground);
	Ground->SetRootComponent(Floor);
	Floor->SetBoxExtent(FVector(2000,2000,10));
	Floor->SetCollisionProfileName(TEXT("BlockAll"));
	Floor->RegisterComponent();
	Ground->SetActorLocation(FVector(0,0,-10));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* ATVClass = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("ATV Blueprint"), ATVClass)) return false;
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector(0,0,30), FRotator::ZeroRotator, Spawn);
	// These are reflected because durability is also an editable/readable Blueprint contract.
	const auto* Maximum = FindFProperty<FFloatProperty>(ATVClass, TEXT("MaxDurability"));
	const auto* Current = FindFProperty<FFloatProperty>(ATVClass, TEXT("CurrentDurability"));
	const auto* Destroyed = FindFProperty<FBoolProperty>(ATVClass, TEXT("bVehicleDestroyed"));
	if (!TestNotNull(TEXT("Editable maximum durability"), Maximum) || !TestNotNull(TEXT("Readable current durability"), Current) || !TestNotNull(TEXT("Readable destroyed state"), Destroyed)) return false;
	Maximum->SetPropertyValue_InContainer(ATV, 300);
#if WITH_EDITOR
	// Imported static fragments must occupy exactly their original rigid skinned part.
	const FName PartBones[] = {TEXT("wheel_FL"), TEXT("wheel_RR"), TEXT("handlebar")};
	const auto* SkinnedAsset = ATV->VehicleMesh->GetSkeletalMeshAsset();
	const auto* Imported = SkinnedAsset->GetImportedModel();
	for (int32 Index=0; Index<3; ++Index)
	{
		if (!ATV->DetachedPartMeshes.IsValidIndex(Index) || !TestNotNull(TEXT("Authored debris asset"), ATV->DetachedPartMeshes[Index].Get())) continue;
		const auto* Part = ATV->DetachedPartMeshes[Index].Get();
		TestTrue(TEXT("Fragment has simple physics collision"), Part->GetBodySetup() && Part->GetBodySetup()->AggGeom.GetElementCount() > 0);
		const int32 BoneIndex = SkinnedAsset->GetRefSkeleton().FindBoneIndex(PartBones[Index]);
		FBox OriginalBounds(ForceInit);
		for (const auto& Section : Imported->LODModels[0].Sections)
			for (const auto& Vertex : Section.SoftVertices)
				if (Section.BoneMap[Vertex.InfluenceBones[0]] == BoneIndex) OriginalBounds += FVector(Vertex.Position);
		const FVector Pivot = SkinnedAsset->GetRefSkeleton().GetRefBonePose()[BoneIndex].GetLocation();
		const FBox PartBounds = Part->GetBoundingBox().ShiftBy(Pivot);
		AddInfo(FString::Printf(TEXT("Part %s bounds center error %.3f cm, extent error %.3f cm"), *PartBones[Index].ToString(), FVector::Distance(PartBounds.GetCenter(), OriginalBounds.GetCenter()), FVector::Distance(PartBounds.GetExtent(), OriginalBounds.GetExtent())));
		TestTrue(TEXT("Fragment replaces original at matching pivot and axes"), PartBounds.GetCenter().Equals(OriginalBounds.GetCenter(), 0.1) && PartBounds.GetExtent().Equals(OriginalBounds.GetExtent(), 0.1));
	}
#endif
	UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
	if (!TestNotNull(TEXT("Player Blueprint"), PlayerClass)) return false;
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(0,140,90), FRotator::ZeroRotator, Spawn);
	World->SpawnActor<AAIController>()->Possess(Player);
	auto* Mount = ATV->MountComponent.Get();
	Mount->EngineStartSound = Mount->EngineIdleSound = Mount->EngineStopSound = nullptr;
	Mount->EngineDriveSound = Mount->EngineBoostSound = nullptr;
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
	World->GetWorldSettings()->NotifyMatchStarted();
	World->BeginPlay();
	TFunction<void(const TCHAR*)> Capture = [](const TCHAR*) {};
#if WITH_EDITOR
	if (FParse::Param(FCommandLine::Get(), TEXT("ATVDamagePreview")))
	{
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		auto* LightActor = World->SpawnActor<AActor>();
		auto* Light = NewObject<UDirectionalLightComponent>(LightActor);
		LightActor->SetRootComponent(Light);
		Light->SetIntensity(5);
		Light->RegisterComponent();
		Light->SetWorldRotation(FRotator(-45,-35,0));
		auto* CaptureActor = World->SpawnActor<AActor>();
		auto* Camera = NewObject<USceneCaptureComponent2D>(CaptureActor);
		CaptureActor->SetRootComponent(Camera);
		Camera->RegisterComponent();
		Camera->bCaptureEveryFrame = false;
		Camera->bCaptureOnMovement = false;
		Camera->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		auto* Target = NewObject<UTextureRenderTarget2D>();
		Target->ClearColor = FLinearColor(0.18f,0.21f,0.25f);
		Target->InitAutoFormat(1000,1000);
		Camera->TextureTarget = Target;
		Camera->FOVAngle = 42;
		Capture = [World, ATV, Camera, Target](const TCHAR* Name)
		{
			const FVector Focus = ATV->GetActorLocation()+FVector(0,0,100);
			Camera->SetWorldLocation(Focus+FVector(330,-410,220));
			Camera->SetWorldRotation((Focus-Camera->GetComponentLocation()).Rotation());
			World->SendAllEndOfFrameUpdates();
			FlushRenderingCommands();
			Camera->CaptureScene();
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1000,1000,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir()/FString::Printf(TEXT("ATVRigWork/Damage_%s.png"), Name)));
		};
	}
#endif
	auto Step = [&](int32 Frames)
	{
		for (int32 Index=0; Index<Frames; ++Index) { World->Tick(LEVELTICK_All, 1.0f/60); FPlatformProcess::Sleep(0.001f); ++GFrameCounter; }
	};
	Step(120);
	Capture(TEXT("Healthy"));
	TestEqual(TEXT("Starts at configured maximum"), Current->GetPropertyValue_InContainer(ATV), 300.0f);
	TArray<UNiagaraComponent*> Smoke;
	ATV->GetComponents(Smoke);
	if (!TestEqual(TEXT("Bounded pair of smoke components"), Smoke.Num(), 2)) return false;
	for (const auto* Component : Smoke)
	{
		TestFalse(TEXT("Undamaged vehicle does not smoke"), Component->IsActive());
		if (TestNotNull(TEXT("Smoke asset is assigned"), Component->GetAsset()))
		{
			for (const auto& Emitter : Component->GetAsset()->GetEmitterHandles())
			{
				if (const auto* Data = Emitter.GetInstance().GetEmitterData()) TestFalse(TEXT("Smoke remains in world space while driving"), Data->bLocalSpace);
			}
		}
	}
	auto ActiveSmoke = [&]() { return int32(ATV->IsDamageSmokeEmitting(false)) + int32(ATV->IsDamageSmokeEmitting(true)); };
	FPointDamageEvent Hit;
	Hit.HitInfo = FHitResult(ATV, ATV->VehicleMesh, ATV->GetActorLocation() + FVector(0,0,60), FVector::UpVector);
	Hit.HitInfo.BoneName = TEXT("root");
	Hit.ShotDirection = FVector::RightVector;
	TestEqual(TEXT("Point hit applies once"), ATV->TakeDamage(10, Hit, nullptr, nullptr), 10.0f);
	TestEqual(TEXT("Durability decreases by hit amount"), Current->GetPropertyValue_InContainer(ATV), 290.0f);
	TestEqual(TEXT("Even a light hit emits smoke"), ActiveSmoke(), 1);
	Step(121);
	TestEqual(TEXT("Light-hit puff emission stops"), ActiveSmoke(), 0);
	ATV->TakeDamage(1, Hit, nullptr, nullptr);
	TestEqual(TEXT("New hit resumes emission while previous particles drain"), ActiveSmoke(), 1);
	if (FApp::CanEverRender()) TestEqual(TEXT("Niagara is actually asked to resume spawning"), ATV->LightDamageSmoke->GetRequestedExecutionState(), ENiagaraExecutionState::Active);
	ATV->TakeDamage(109, Hit, nullptr, nullptr);
	Step(180);
	TestEqual(TEXT("Damaged vehicle keeps emitting smoke"), ActiveSmoke(), 1);
	Capture(TEXT("LightSmoke"));
	ATV->SetCanBeDamaged(false);
	TestEqual(TEXT("Damage-disabled vehicle is respected"), ATV->TakeDamage(100, Hit, nullptr, nullptr), 0.0f);
	ATV->SetCanBeDamaged(true);
	TestEqual(TEXT("Invalid damage is rejected"), ATV->TakeDamage(std::numeric_limits<float>::quiet_NaN(), Hit, nullptr, nullptr), 0.0f);
	TestEqual(TEXT("Negative damage cannot repair"), ATV->TakeDamage(-1, Hit, nullptr, nullptr), 0.0f);
	ATV->TakeDamage(100, Hit, nullptr, nullptr);
	TestEqual(TEXT("Severe damage retains a single active smoke stage"), ActiveSmoke(), 1);
	Step(90);
	Capture(TEXT("HeavySmoke"));
	Player->SetActorLocation(ATV->GetActorLocation()+FVector(0,140,90));
	const auto* OriginalAnimClass = Player->GetMesh()->GetAnimClass();
	TestTrue(TEXT("Damaged vehicle can still be mounted"), Mount->TryMount(Player));
	Mount->SetDriveInput(FVector2D(0,1));
	Step(45);
	TestEqual(TEXT("Lethal damage clamps to remaining durability"), ATV->TakeDamage(1000, Hit, nullptr, nullptr), 80.0f);
	TestEqual(TEXT("Destroyed durability is zero"), Current->GetPropertyValue_InContainer(ATV), 0.0f);
	TestTrue(TEXT("Destruction is latched"), Destroyed->GetPropertyValue_InContainer(ATV));
	TestNull(TEXT("Destroyed vehicle releases its rider"), Mount->GetRider());
	TestFalse(TEXT("Player detached from seat"), Player->IsMountedInVehicle());
	TestTrue(TEXT("Original rider animation restored"), Player->GetMesh()->GetAnimClass() == OriginalAnimClass);
	TestEqual(TEXT("Rider collision restored"), Player->GetCapsuleComponent()->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
	TestFalse(TEXT("Wreck cannot be mounted"), Mount->CanMount(Player));
	TestFalse(TEXT("Wreck cannot drive"), ATV->CanAcceptDriveInput());
	TestFalse(TEXT("Wheel simulation stops supporting missing wheels"), ATV->VehicleMovement->IsPhysicsStateCreated());
	for (const FName Bone : {FName(TEXT("wheel_FL")), FName(TEXT("wheel_RR")), FName(TEXT("handlebar"))})
		TestTrue(TEXT("Detached part disappears from original mesh"), ATV->VehicleMesh->IsBoneHiddenByName(Bone));
	TArray<TWeakObjectPtr<AActor>> Debris;
	for (TActorIterator<AActor> It(World); It; ++It) if (It->GetOwner() == ATV && It->ActorHasTag(TEXT("ATVDebris"))) Debris.Add(*It);
	TestEqual(TEXT("Exactly three original parts detach"), Debris.Num(), 3);
	for (const auto& Part : Debris)
	{
		auto* Mesh = Cast<UStaticMeshComponent>(Part->GetRootComponent());
		if (TestNotNull(TEXT("Debris uses authored mesh"), Mesh))
		{
			TestNotNull(TEXT("Debris mesh asset exists"), Mesh->GetStaticMesh().Get());
			TestTrue(TEXT("Debris simulates independently"), Mesh->IsSimulatingPhysics());
			TestEqual(TEXT("Debris cannot push player"), Mesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Ignore);
			TestTrue(TEXT("Debris has bounded launch speed"), Mesh->GetPhysicsLinearVelocity().Size() <= 400);
		}
	}
	TestEqual(TEXT("Further hits cannot destroy again"), ATV->TakeDamage(1000, Hit, nullptr, nullptr), 0.0f);
	Step(120);
	TestTrue(TEXT("Wreck settles instead of launching"), ATV->GetVelocity().Size() < 400 && ATV->GetActorLocation().Z < 100);
	Capture(TEXT("Destroyed"));
	ATV->Destroy();
	for (const auto& Part : Debris) TestTrue(TEXT("Vehicle removal cleans up debris"), !Part.IsValid() || Part->IsActorBeingDestroyed());
	// A blocked exit must never teleport a rider back to an old boarding position.
	auto* TrappedATV = World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector(0,700,30), FRotator::ZeroRotator, Spawn);
	TrappedATV->WreckSmokeDuration = 0.2f;
	TrappedATV->DebrisLifetime = 1.0f;
	auto* TrappedMount = TrappedATV->MountComponent.Get();
	TrappedMount->EngineStartSound = TrappedMount->EngineIdleSound = TrappedMount->EngineStopSound = nullptr;
	TrappedMount->EngineDriveSound = TrappedMount->EngineBoostSound = nullptr;
	Player->SetActorLocation(FVector(0,840,90));
	TestTrue(TEXT("Board before blocked-exit scenario"), TrappedMount->TryMount(Player));
	TrappedATV->SetActorLocation(FVector(700,700,30), false, nullptr, ETeleportType::TeleportPhysics);
	for (int32 Side=0; Side<4; ++Side)
	{
		auto* Wall = World->SpawnActor<AActor>();
		auto* Box = NewObject<UBoxComponent>(Wall);
		Wall->SetRootComponent(Box);
		const bool bX = Side<2;
		Box->SetBoxExtent(bX ? FVector(10,250,200) : FVector(250,10,200));
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->RegisterComponent();
		const float Sign = Side%2 ? 1 : -1;
		Wall->SetActorLocation(FVector(700,700,100) + (bX ? FVector(Sign*110,0,0) : FVector(0,Sign*90,0)));
	}
	const FVector SeatAtDestruction = Player->GetActorLocation();
	FVector NoExit;
	TestFalse(TEXT("Surrounding walls prevent normal exits"), TrappedMount->FindDismountLocation(NoExit));
	TrappedATV->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	TestTrue(TEXT("Blocked exit releases at current seat"), Player->GetActorLocation().Equals(SeatAtDestruction, 0.1));
	TestFalse(TEXT("Blocked exit still clears mount state"), Player->IsMountedInVehicle());
	TestTrue(TEXT("Emergency exit temporarily ignores only its wreck"), Player->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(TrappedATV));
	Player->SetActorLocation(FVector(1100,1100,90));
	Step(90);
	TestFalse(TEXT("Wreck ignore is removed after player clears it"), Player->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(TrappedATV));
	TestFalse(TEXT("Wreck smoke emission expires"), TrappedATV->IsDamageSmokeEmitting(true));
	int32 RemainingParts = 0;
	for (TActorIterator<AActor> It(World); It; ++It) if (It->GetOwner()==TrappedATV && It->ActorHasTag(TEXT("ATVDebris"))) ++RemainingParts;
	TestEqual(TEXT("Detached parts expire within their configured lifetime"), RemainingParts, 0);
	return true;
}
#endif
