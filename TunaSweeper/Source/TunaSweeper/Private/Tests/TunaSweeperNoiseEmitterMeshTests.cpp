#if WITH_DEV_AUTOMATION_TESTS
#include "Interaction/TunaSweeperPeriodicNoiseEmitterActor.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperNoiseEmitterMeshTest,
	"TunaSweeper.NoiseEmitter.AuthoredMeshAndPulse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperNoiseEmitterMeshTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("NoiseMeshTest")));
	if (!TestNotNull(TEXT("Test world"), World)) { return false; }
	ATunaSweeperPeriodicNoiseEmitterActor* Actor = World->SpawnActor<ATunaSweeperPeriodicNoiseEmitterActor>();
	if (!TestNotNull(TEXT("Direct actor placement"), Actor))
	{
		World->DestroyWorld(false); World->RemoveFromRoot(); return false;
	}
	TestNotNull(TEXT("Authored body reference"), Actor->BodySourceMesh.Get());
	TestNotNull(TEXT("Authored horn reference"), Actor->HornSourceMesh.Get());
	int32 Triangles = 0;
	FBox Bounds(ForceInit);
	for (const auto& Section : Actor->RuntimeMeshSections)
	{
		Triangles += Actor->ProceduralMesh->GetProcMeshSection(Section.SectionIndex)->ProcIndexBuffer.Num() / 3;
		for (const FVector& Vertex : Section.BaseVertices) { Bounds += Vertex; }
	}
	TestEqual(TEXT("Authored topology, not legacy fallback"), Triangles, 800);
	TestTrue(TEXT("Original size and ground pivot"), Bounds.Min.Equals(FVector(-152, -152, 0), .01f) && Bounds.Max.Equals(FVector(152, 152, 206), .01f));
	UTunaSweeperNoiseSubsystem* Noise = World->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	int32 Events = 0;
	FTunaSweeperNoiseEvent Event;
	const FDelegateHandle Handle = Noise->OnNoiseReported.AddLambda([&](const FTunaSweeperNoiseEvent& Value) { ++Events; Event = Value; });
	Actor->EmitNoise();
	TestEqual(TEXT("One emitted event"), Events, 1);
	TestEqual(TEXT("Loudness unchanged"), Event.Loudness, 1.0f);
	TestEqual(TEXT("Range unchanged"), Event.MaxRange, 2600.0f);
	TestTrue(TEXT("Source offset unchanged"), Event.SourceLocation.Equals(FVector(0, 0, 160)));
	TestTrue(TEXT("Pulse starts"), Actor->bHornPulseActive);
	float MaxDisplacement = 0.0f;
	for (const auto& Section : Actor->RuntimeMeshSections)
	{
		const FProcMeshSection* Current = Actor->ProceduralMesh->GetProcMeshSection(Section.SectionIndex);
		for (int32 I = 0; I < Section.BaseVertices.Num(); ++I)
		{
			const float Distance = FVector::Distance(Current->ProcVertexBuffer[I].Position, Section.BaseVertices[I]);
			if (Section.bIsHorn) { MaxDisplacement = FMath::Max(MaxDisplacement, Distance); }
			else { TestTrue(TEXT("Body stays fixed during pulse"), Distance < .001f); }
		}
	}
	TestTrue(TEXT("Authored horns deform"), MaxDisplacement > 17.0f);
	Actor->Tick(.25f);
	TestFalse(TEXT("Pulse stops after original duration"), Actor->bHornPulseActive);
	for (const auto& Section : Actor->RuntimeMeshSections)
	{
		const FProcMeshSection* Current = Actor->ProceduralMesh->GetProcMeshSection(Section.SectionIndex);
		for (int32 I = 0; I < Section.BaseVertices.Num(); ++I)
		{
			TestTrue(TEXT("Pulse restores authored shape"), Current->ProcVertexBuffer[I].Position.Equals(Section.BaseVertices[I], .001f));
		}
	}
	Noise->OnNoiseReported.Remove(Handle);
	// Clearing optional mesh assets preserves the original JSON compatibility path.
	Actor->BodySourceMesh = nullptr;
	Actor->RebuildProceduralMesh();
	TestEqual(TEXT("Legacy JSON fallback remains available"), Actor->RuntimeMeshSections.Num(), 6);
	World->DestroyWorld(false); World->RemoveFromRoot();
	return true;
}
#endif
