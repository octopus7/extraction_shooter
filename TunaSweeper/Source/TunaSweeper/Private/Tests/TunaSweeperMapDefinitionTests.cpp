#if WITH_DEV_AUTOMATION_TESTS

#include "Map/TunaSweeperMapDefinition.h"
#include "UI/TunaSweeperMapWidget.h"

#include "Components/CanvasPanel.h"
#include "Input/HittestGrid.h"
#include "Rendering/DrawElements.h"
#include "Types/PaintArgs.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWindow.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperMapBookmarkCoordinatesTest,
	"TunaSweeper.Map.BookmarkCoordinates",
	TunaSweeperMapDefinitionTests::TestFlags)

bool FTunaSweeperMapBookmarkCoordinatesTest::RunTest(const FString& Parameters)
{
	UTunaSweeperMapDefinition* Definition = NewObject<UTunaSweeperMapDefinition>();
	Definition->CaptureCenter = FVector(1000.0, -2000.0, 9000.0);
	Definition->CaptureWorldSize = FVector2D(4000.0, 8000.0);
	Definition->CaptureYawDegrees = 90.0f;
	Definition->ContentPixelMin = FIntPoint(0, 512);
	Definition->ContentPixelSize = FIntPoint(2048, 1024);

	// The displayed map is square-padded. North/right on screen must resolve
	// through content UV and capture yaw, while teleport grounding receives pawn Z.
	FVector2D ContentUV;
	TestTrue(TEXT("Top right content edge is a valid bookmark"),
		Definition->TextureUVToContentUV(FVector2D(1.0, 0.25), ContentUV));
	const FVector Destination = Definition->ContentUVToWorldLocation(ContentUV, 123.0f);
	TestTrue(TEXT("Rotated top right uses the expected world XY and pawn height"),
		Destination.Equals(FVector(-3000.0, 0.0, 123.0), 0.01));
	TestTrue(TEXT("Bookmark center resolves the capture center XY"),
		Definition->ContentUVToWorldLocation(FVector2D(0.5, 0.5), -45.0f)
			.Equals(FVector(1000.0, -2000.0, -45.0), 0.01));
	TestFalse(TEXT("Transparent lower padding cannot become a bookmark"),
		Definition->TextureUVToContentUV(FVector2D(0.5, 0.9), ContentUV));
	TestFalse(TEXT("Texture exterior cannot become a bookmark"),
		Definition->TextureUVToContentUV(FVector2D(-0.01, 0.5), ContentUV));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperMapMarkerTeleportTest,
	"TunaSweeper.Map.MarkerTeleportCursorCoordinates",
	TunaSweeperMapDefinitionTests::TestFlags)

bool FTunaSweeperMapMarkerTeleportTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UTunaSweeperMapWidget> Map(NewObject<UTunaSweeperMapWidget>());
	Map->MapCanvas = NewObject<UCanvasPanel>(Map.Get());
	Map->MapDefinition = NewObject<UTunaSweeperMapDefinition>(Map.Get());
	Map->MapDefinition->CaptureCenter = FVector(5000.0, -3000.0, 0.0);
	Map->MapDefinition->CaptureYawDegrees = 37.0f;
	Map->MapDefinition->CaptureWorldSize = FVector2D(4000.0, 8000.0);
	Map->MapDefinition->ContentPixelMin = FIntPoint(0, 512);
	Map->MapDefinition->ContentPixelSize = FIntPoint(2048, 1024);
	Map->MapZoom = 1.7f;
	Map->MapPan = FVector2D(-80.0, 50.0);
	const TSharedRef<SWidget> Canvas = Map->MapCanvas->TakeWidget();
	const TSharedRef<SWindow> Window = SNew(SWindow).ClientSize(FVector2D(1600.0, 1200.0));
	const FGeometry PaintGeometry = FGeometry::MakeRoot(FVector2D(800.0, 600.0),
		FSlateLayoutTransform(1.5f, FVector2D(120.0, 90.0)));
	const FVector2D ContentUV(0.6, 0.4);
	FTunaSweeperMapMarkerSaveData Marker;
	Marker.MarkerId = 7;
	Marker.MapPosition = ContentUV;
	Map->CachedMapMarkers.Add(Marker);
	// Real Slate Paint populates desktop tick geometry and window paint geometry.
	// Cover a window on a negative-offset monitor and a positive-offset window.
	for (const FVector2D WindowOffset : {FVector2D(-1600.0, -400.0), FVector2D(350.0, 200.0)})
	{
		FHittestGrid HitGrid;
		HitGrid.SetHittestArea(WindowOffset, FVector2D(1600.0, 1200.0));
		FSlateWindowElementList Elements(Window);
		const FPaintArgs Args(&Window.Get(), HitGrid, WindowOffset, 1.0, 0.0f);
		Canvas->SlatePrepass(1.5f);
		Canvas->Paint(Args, PaintGeometry, FSlateRect(0.0f, 0.0f, 1600.0f, 1200.0f), Elements, 0, FWidgetStyle(), true);
		const FVector2D LocalPosition = Map->MapPositionToLocal(ContentUV);
		const FVector2D DesktopCursor = PaintGeometry.LocalToAbsolute(LocalPosition) + WindowOffset;
		const FVector2D CursorLocal = Map->MapCanvas->GetCachedGeometry().AbsoluteToLocal(DesktopCursor);
		TestTrue(TEXT("F9 desktop cursor resolves the same map-local point"), CursorLocal.Equals(LocalPosition, 0.001));
		FVector2D CursorContentUV;
		TestTrue(TEXT("Zoomed and panned cursor is inside map content"), Map->TryGetMapPositionFromLocal(CursorLocal, CursorContentUV));
		TestTrue(TEXT("Cursor position survives pan zoom and padded UV conversion"), CursorContentUV.Equals(ContentUV, 0.001));
		TestEqual(TEXT("F9 finds the existing ordinary marker under the desktop cursor"), Map->FindNearestMapMarkerId(CursorLocal), 7);
		TestEqual(TEXT("Point just inside 26 local unit marker hit tolerance is accepted"), Map->FindNearestMapMarkerId(CursorLocal + FVector2D(25.999, 0.0)), 7);
		TestEqual(TEXT("Empty map space does not create or select a marker"), Map->FindNearestMapMarkerId(CursorLocal + FVector2D(27.0, 0.0)), INDEX_NONE);
		TestEqual(TEXT("F9 marker lookup leaves ordinary marker storage unchanged"), Map->CachedMapMarkers.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
