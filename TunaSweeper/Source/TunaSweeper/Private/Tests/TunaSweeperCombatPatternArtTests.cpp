#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperMissileTurret.h"
#include "AI/TunaSweeperPatternEnemyCharacter.h"
#include "AI/TunaSweeperRollingRobotMinion.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "UObject/UnrealType.h"

namespace TunaSweeperCombatPatternArtTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const FString MeshDirectory(TEXT("/Game/Characters/CombatPatterns/Meshes/"));
	const FString MaterialDirectory(TEXT("/Game/Characters/CombatPatterns/Materials/"));

	struct FTestWorld
	{
		UWorld* World = nullptr;
		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).EnableTraceCollision(true).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("CombatPatternArtTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine) { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
		}
		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) { GEngine->DestroyWorldContext(World); }
				World->RemoveFromRoot();
			}
		}
	};

	void CheckMesh(FAutomationTestBase& Test, UStaticMesh* Mesh, const TCHAR* AssetName)
	{
		const FString Name(AssetName);
		if (!Test.TestNotNull(Name + TEXT(" is an imported mesh"), Mesh)) { return; }
		Test.TestEqual(Name + TEXT(" uses its authored asset"), Mesh->GetPathName(), MeshDirectory + Name + TEXT(".") + Name);
		const FVector Size = Mesh->GetBounds().BoxExtent * 2.0f;
		Test.TestTrue(Name + TEXT(" preserves centimetre scale"), !Size.ContainsNaN() && Size.GetMax() > 10.0 && Size.GetMax() < 250.0);
		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		Test.TestTrue(Name + TEXT(" carries the shared material palette"), Slots.Num() > 0 && Slots.Num() <= 5);
		const TSet<FName> Palette = {FName(TEXT("CP_Armor")), FName(TEXT("CP_Dark")), FName(TEXT("CP_Teal")),
			FName(TEXT("CP_Amber")), FName(TEXT("CP_Rubber"))};
		for (const FStaticMaterial& Slot : Slots)
		{
			Test.TestTrue(Name + TEXT(" has an authored material slot"), Palette.Contains(Slot.MaterialSlotName));
			if (Test.TestNotNull(Name + TEXT(" material is assigned"), Slot.MaterialInterface.Get()))
			{
				const FString MaterialName = TEXT("M_") + Slot.MaterialSlotName.ToString();
				Test.TestEqual(Name + TEXT(" slot maps to the correct palette material"), Slot.MaterialInterface->GetPathName(),
					MaterialDirectory + MaterialName + TEXT(".") + MaterialName);
			}
		}
		if (const UBodySetup* Body = Mesh->GetBodySetup())
		{
			Test.TestEqual(Name + TEXT(" adds no second gameplay collision hull"), Body->AggGeom.GetElementCount(), 0);
		}
	}

	UStaticMeshComponent* CheckPart(FAutomationTestBase& Test, AActor* Defaults, const TCHAR* ComponentName, const TCHAR* AssetName)
	{
		UStaticMeshComponent* Part = Cast<UStaticMeshComponent>(Defaults->GetDefaultSubobjectByName(FName(ComponentName)));
		if (!Test.TestNotNull(FString(ComponentName) + TEXT(" is wired on the native actor"), Part)) { return nullptr; }
		CheckMesh(Test, Part->GetStaticMesh(), AssetName);
		Test.TestEqual(FString(ComponentName) + TEXT(" is visual only"), static_cast<uint8>(Part->GetCollisionEnabled()),
			static_cast<uint8>(ECollisionEnabled::NoCollision));
		Test.TestFalse(FString(ComponentName) + TEXT(" does not obstruct navigation"), Part->CanEverAffectNavigation());
		if (Part->GetStaticMesh())
		{
			for (int32 Slot = 0; Slot < Part->GetStaticMesh()->GetStaticMaterials().Num(); ++Slot)
			{
				Test.TestTrue(FString(ComponentName) + TEXT(" retains its multi-material presentation"),
					Part->GetMaterial(Slot) == Part->GetStaticMesh()->GetMaterial(Slot));
			}
		}
		return Part;
	}

	void CheckGeometry(FAutomationTestBase& Test, UProceduralMeshComponent* Mesh, const FString& Label, int32 VertexBudget)
	{
		Test.TestEqual(Label + TEXT(" mesh collision is disabled"), static_cast<uint8>(Mesh->GetCollisionEnabled()),
			static_cast<uint8>(ECollisionEnabled::NoCollision));
		Test.TestFalse(Label + TEXT(" cannot emit overlaps"), Mesh->GetGenerateOverlapEvents());
		Test.TestFalse(Label + TEXT(" does not affect navigation"), Mesh->CanEverAffectNavigation());
		bool bFinite = true;
		bool bIndicesValid = true;
		bool bCollisionless = true;
		int32 Vertices = 0;
		for (int32 Index = 0; Index < Mesh->GetNumSections(); ++Index)
		{
			const FProcMeshSection* Section = Mesh->GetProcMeshSection(Index);
			if (!Section) { continue; }
			bCollisionless &= !Section->bEnableCollision;
			Vertices += Section->ProcVertexBuffer.Num();
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
			{
				bFinite &= !Vertex.Position.ContainsNaN() && !Vertex.Normal.ContainsNaN() &&
					FMath::IsFinite(Vertex.UV0.X) && FMath::IsFinite(Vertex.UV0.Y) && Vertex.Position.GetAbsMax() < 10000.0;
			}
			bIndicesValid &= Section->ProcIndexBuffer.Num() % 3 == 0;
			for (const uint32 VertexIndex : Section->ProcIndexBuffer)
			{
				bIndicesValid &= VertexIndex < static_cast<uint32>(Section->ProcVertexBuffer.Num());
			}
		}
		Test.TestTrue(Label + TEXT(" produces bounded finite geometry"), bFinite);
		Test.TestTrue(Label + TEXT(" references valid triangle vertices"), bIndicesValid);
		Test.TestTrue(Label + TEXT(" has no procedural collision sections"), bCollisionless);
		Test.TestTrue(Label + TEXT(" stays within a small visual geometry budget"), Vertices > 0 && Vertices < VertexBudget);
	}

	void CheckShader(FAutomationTestBase& Test, UProceduralMeshComponent* Mesh, int32 Slot, const TCHAR* MaterialName)
	{
		UMaterialInterface* Material = Mesh->GetMaterial(Slot);
		if (Test.TestNotNull(TEXT("The authored vertex-colour effect material is bound"), Material) && Material->GetMaterial())
		{
			const FString Name(MaterialName);
			Test.TestEqual(TEXT("Runtime effects retain the intended glow or smoke shader"), Material->GetMaterial()->GetPathName(),
				MaterialDirectory + Name + TEXT(".") + Name);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternNativeArtBindingsTest,
	"TunaSweeper.Combat.Patterns.Art.NativeBindings", TunaSweeperCombatPatternArtTests::TestFlags)

bool FTunaSweeperCombatPatternNativeArtBindingsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternArtTests;
	ATunaSweeperRollingRobotMinion* Robot = GetMutableDefault<ATunaSweeperRollingRobotMinion>();
	UStaticMeshComponent* Shell = CheckPart(*this, Robot, TEXT("RobotShell"), TEXT("SM_CP_RobotShell"));
	UStaticMeshComponent* Eye = CheckPart(*this, Robot, TEXT("RobotEye"), TEXT("SM_CP_RobotEye"));
	CheckPart(*this, Robot, TEXT("LeftLeg"), TEXT("SM_CP_RobotLeg"));
	CheckPart(*this, Robot, TEXT("RightLeg"), TEXT("SM_CP_RobotLeg"));
	CheckPart(*this, Robot, TEXT("LeftFoot"), TEXT("SM_CP_RobotFoot"));
	CheckPart(*this, Robot, TEXT("RightFoot"), TEXT("SM_CP_RobotFoot"));
	if (Shell && Eye)
	{
		TestTrue(TEXT("The eyes roll with the armored shell"), Eye->GetAttachParent() == Shell || Eye->GetAttachParent() == Shell->GetAttachParent());
		TestTrue(TEXT("Eyes keep the authored shell-centre pivot"), Eye->GetRelativeLocation().IsNearlyZero());
	}
	ATunaSweeperMissileTurret* Turret = GetMutableDefault<ATunaSweeperMissileTurret>();
	CheckPart(*this, Turret, TEXT("BaseMesh"), TEXT("SM_CP_TurretBase"));
	CheckPart(*this, Turret, TEXT("LauncherMesh"), TEXT("SM_CP_TurretHead"));
	CheckPart(*this, Turret, TEXT("LeftLaunchTube"), TEXT("SM_CP_TurretTube"));
	CheckPart(*this, Turret, TEXT("RightLaunchTube"), TEXT("SM_CP_TurretTube"));
	const FObjectPropertyBase* MissileProperty = FindFProperty<FObjectPropertyBase>(Turret->GetClass(), TEXT("MissileVisualAsset"));
	if (TestNotNull(TEXT("The dynamically spawned missile retains a cooked asset reference"), MissileProperty))
	{
		CheckMesh(*this, Cast<UStaticMesh>(MissileProperty->GetObjectPropertyValue_InContainer(Turret)), TEXT("SM_CP_Missile"));
	}
	ATunaSweeperPatternEnemyCharacter* Charger = GetMutableDefault<ATunaSweeperPatternEnemyCharacter>();
	CheckPart(*this, Charger, TEXT("ChargeChassis"), TEXT("SM_CP_ChargeChassis"));
	for (AActor* Enemy : {static_cast<AActor*>(Robot), static_cast<AActor*>(Charger)})
	{
		for (const TCHAR* Name : {TEXT("VisualMesh"), TEXT("ForwardMarkerMesh")})
		{
			const UStaticMeshComponent* Placeholder = Cast<UStaticMeshComponent>(Enemy->GetDefaultSubobjectByName(FName(Name)));
			if (TestNotNull(TEXT("The inherited prototype visual can be inspected"), Placeholder))
			{
				TestFalse(TEXT("Prototype primitives do not overlap the imported robot"), Placeholder->IsVisible());
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternEffectsPresentationTest,
	"TunaSweeper.Combat.Patterns.Art.EffectPresentation", TunaSweeperCombatPatternArtTests::TestFlags)

bool FTunaSweeperCombatPatternEffectsPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternArtTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Effect test world exists"), TestWorld.World)) { return false; }
	for (const ETunaSweeperCombatPatternEffect Kind : {ETunaSweeperCombatPatternEffect::Summon,
		ETunaSweeperCombatPatternEffect::MissileLaunch, ETunaSweeperCombatPatternEffect::MissileTrail,
		ETunaSweeperCombatPatternEffect::Impact, ETunaSweeperCombatPatternEffect::ChargeTrail,
		ETunaSweeperCombatPatternEffect::RobotUnfold, ETunaSweeperCombatPatternEffect::RobotDeath})
	{
		const FString Label = FString::Printf(TEXT("Effect %d"), static_cast<int32>(Kind));
		ATunaSweeperCombatPatternEffectActor* Effect = ATunaSweeperCombatPatternEffectActor::Spawn(TestWorld.World, Kind,
			FVector(120.0f, 150.0f, 0.0f), 120.0f, FVector(0.6f, 0.0f, 0.8f));
		if (!TestNotNull(Label + TEXT(" spawns"), Effect)) { continue; }
		UProceduralMeshComponent* Mesh = Effect->FindComponentByClass<UProceduralMeshComponent>();
		if (!TestNotNull(Label + TEXT(" has visual geometry"), Mesh)) { continue; }
		const float Duration = Effect->GetEffectDuration();
		TestTrue(Label + TEXT(" completes within a short bounded duration"), Duration > 0.0f && Duration <= 2.0f);
		TestFalse(Label + TEXT(" actor cannot block gameplay"), Effect->GetActorEnableCollision());
		TestFalse(Label + TEXT(" remains local visual presentation"), Effect->GetIsReplicated());
		const float Lifetime = Effect->GetLifeSpan();
		TestTrue(Label + TEXT(" also has a bounded lifetime safety timer"), Lifetime >= Duration && Lifetime <= 2.1f);
		for (const float Age : {0.2f, 0.8f})
		{
			Effect->PreviewAtNormalizedAge(Age);
			CheckGeometry(*this, Mesh, Label, 2000);
			TestEqual(Label + TEXT(" preview does not reset the lifetime"), Effect->GetLifeSpan(), Lifetime);
		}
		CheckShader(*this, Mesh, 0, TEXT("M_CP_Glow"));
		CheckShader(*this, Mesh, 1, TEXT("M_CP_Smoke"));
		Effect->Tick(Duration * 0.3f);
		TestFalse(Label + TEXT(" artist preview does not consume runtime age"), Effect->IsActorBeingDestroyed());
		Effect->Tick(Duration * 0.8f);
		TestTrue(Label + TEXT(" removes itself after its visual finishes"), Effect->IsActorBeingDestroyed());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCombatPatternTelegraphPresentationTest,
	"TunaSweeper.Combat.Patterns.Art.TelegraphPresentation", TunaSweeperCombatPatternArtTests::TestFlags)

bool FTunaSweeperCombatPatternTelegraphPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperCombatPatternArtTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Warning test world exists"), TestWorld.World)) { return false; }
	ATunaSweeperAttackTelegraph* Warning = TestWorld.World->SpawnActor<ATunaSweeperAttackTelegraph>();
	if (!TestNotNull(TEXT("Warning actor spawns"), Warning)) { return false; }
	UProceduralMeshComponent* Mesh = Warning->FindComponentByClass<UProceduralMeshComponent>();
	if (!TestNotNull(TEXT("Warning has renderable geometry"), Mesh)) { return false; }
	for (const bool bCircle : {true, false})
	{
		constexpr float Radius = 180.0f;
		constexpr float Length = 900.0f;
		constexpr float HalfWidth = 70.0f;
		if (bCircle) { Warning->InitCircle(FVector(200.0f, 300.0f, 0.0f), Radius, 2.0f); }
		else { Warning->InitLane(FVector::ZeroVector, FVector(0.0f, Length, 0.0f), HalfWidth, 2.0f); }
		for (const float Progress : {0.25f, 0.81f, 1.0f})
		{
			Warning->SetProgress(Progress);
			CheckGeometry(*this, Mesh, bCircle ? TEXT("Circle warning") : TEXT("Charge lane"), 6000);
			bool bInsideBoundary = true;
			double FillExtent = 0.0;
			for (int32 SectionIndex = 0; SectionIndex < Mesh->GetNumSections(); ++SectionIndex)
			{
				const FProcMeshSection* Section = Mesh->GetProcMeshSection(SectionIndex);
				if (!Section) { continue; }
				for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
				{
					const FVector& Point = Vertex.Position;
					bInsideBoundary &= bCircle ? Point.Size2D() <= Radius + 0.05f :
						Point.X >= -0.05f && Point.X <= Length + 0.05f && FMath::Abs(Point.Y) <= HalfWidth + 0.05f;
					if (SectionIndex == 1) { FillExtent = FMath::Max(FillExtent, bCircle ? Point.Size2D() : Point.X); }
				}
			}
			TestTrue(TEXT("Warning decoration stays inside the advertised damage boundary"), bInsideBoundary);
			const double ExpectedExtent = bCircle ? Radius * FMath::Sqrt(Progress) : Length * Progress;
			TestTrue(TEXT("The visible fill communicates warning progress and reaches the boundary"),
				FMath::IsNearlyEqual(FillExtent, ExpectedExtent, 0.05));
		}
		CheckShader(*this, Mesh, 0, TEXT("M_CP_Glow"));
		CheckShader(*this, Mesh, 1, TEXT("M_CP_Effect"));
		CheckShader(*this, Mesh, 2, TEXT("M_CP_Glow"));
	}
	return true;
}

#endif
