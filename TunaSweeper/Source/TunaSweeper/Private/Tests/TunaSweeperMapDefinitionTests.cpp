#if WITH_DEV_AUTOMATION_TESTS

#include "Map/TunaSweeperMapDefinition.h"

#include "Misc/AutomationTest.h"

namespace TunaSweeperMapDefinitionTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperMapContentRectTest,
	"TunaSweeper.Map.ContentRect",
	TunaSweeperMapDefinitionTests::TestFlags)

bool FTunaSweeperMapContentRectTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntRect Square = UTunaSweeperMapDefinition::CalculateCenteredContentRect(FVector2D(3700.0, 3700.0));
	TestEqual(TEXT("Square min"), Square.Min, FIntPoint::ZeroValue);
	TestEqual(TEXT("Square size"), Square.Size(), FIntPoint(2048, 2048));

	const FIntRect Wide = UTunaSweeperMapDefinition::CalculateCenteredContentRect(FVector2D(22400.0, 24400.0));
	TestEqual(TEXT("Wide min"), Wide.Min, FIntPoint(0, 84));
	TestEqual(TEXT("Wide size"), Wide.Size(), FIntPoint(2048, 1880));

	const FIntRect Tall = UTunaSweeperMapDefinition::CalculateCenteredContentRect(FVector2D(24400.0, 22400.0));
	TestEqual(TEXT("Tall min"), Tall.Min, FIntPoint(84, 0));
	TestEqual(TEXT("Tall size"), Tall.Size(), FIntPoint(1880, 2048));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperMapProjectionRoundTripTest,
	"TunaSweeper.Map.ProjectionRoundTrip",
	TunaSweeperMapDefinitionTests::TestFlags)

bool FTunaSweeperMapProjectionRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UTunaSweeperMapDefinition* Definition = NewObject<UTunaSweeperMapDefinition>();
	Definition->CaptureCenter = FVector(1250.0, -700.0, 100.0);
	Definition->CaptureWorldSize = FVector2D(22400.0, 24400.0);
	Definition->CaptureYawDegrees = 37.0f;
	const FIntRect ContentRect = UTunaSweeperMapDefinition::CalculateCenteredContentRect(Definition->CaptureWorldSize);
	Definition->ContentPixelMin = ContentRect.Min;
	Definition->ContentPixelSize = ContentRect.Size();

	const FVector2D ExpectedContentUV(0.17, 0.82);
	const FVector WorldLocation = Definition->ContentUVToWorldLocation(ExpectedContentUV, 345.0f);
	const FVector2D ActualContentUV = Definition->WorldLocationToContentUV(WorldLocation);
	TestTrue(TEXT("World/content round trip"), ActualContentUV.Equals(ExpectedContentUV, 0.0001));

	const FVector2D TextureUV = Definition->ContentUVToTextureUV(ExpectedContentUV);
	FVector2D InverseContentUV = FVector2D::ZeroVector;
	TestTrue(TEXT("Content lies inside texture content rect"), Definition->TextureUVToContentUV(TextureUV, InverseContentUV));
	TestTrue(TEXT("Content/texture round trip"), InverseContentUV.Equals(ExpectedContentUV, 0.0001));

	FVector2D PaddingContentUV = FVector2D::ZeroVector;
	TestFalse(TEXT("Top padding is rejected"), Definition->TextureUVToContentUV(FVector2D(0.5, 0.01), PaddingContentUV));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
