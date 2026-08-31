#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystemInternal.h"
#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"

#include <limits>

namespace TunaSweeperSpeechBubbleTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::EngineFilter;

	struct FDirectionCase
	{
		ETunaSweeperSpeechBubbleTailDirection Direction;
		FVector2D UnitVector;
	};

	const FDirectionCase DirectionCases[] =
	{
		{ETunaSweeperSpeechBubbleTailDirection::Up, FVector2D(0.0f, -1.0f)},
		{ETunaSweeperSpeechBubbleTailDirection::UpRight, FVector2D(1.0f, -1.0f).GetSafeNormal()},
		{ETunaSweeperSpeechBubbleTailDirection::Right, FVector2D(1.0f, 0.0f)},
		{ETunaSweeperSpeechBubbleTailDirection::DownRight, FVector2D(1.0f, 1.0f).GetSafeNormal()},
		{ETunaSweeperSpeechBubbleTailDirection::Down, FVector2D(0.0f, 1.0f)},
		{ETunaSweeperSpeechBubbleTailDirection::DownLeft, FVector2D(-1.0f, 1.0f).GetSafeNormal()},
		{ETunaSweeperSpeechBubbleTailDirection::Left, FVector2D(-1.0f, 0.0f)},
		{ETunaSweeperSpeechBubbleTailDirection::UpLeft, FVector2D(-1.0f, -1.0f).GetSafeNormal()},
	};

	TunaSweeperSpeechBubbleInternal::FTargetIdentity MakeScreenTarget(const FVector2D Position)
	{
		TunaSweeperSpeechBubbleInternal::FTargetIdentity Target;
		Target.Type = TunaSweeperSpeechBubbleInternal::ETargetType::Screen;
		Target.ScreenPosition = Position;
		return Target;
	}

	TunaSweeperSpeechBubbleInternal::FTargetIdentity MakeWorldTarget(const FVector Position)
	{
		TunaSweeperSpeechBubbleInternal::FTargetIdentity Target;
		Target.Type = TunaSweeperSpeechBubbleInternal::ETargetType::World;
		Target.WorldLocation = Position;
		return Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSpeechBubbleAnchorDirectionsTest,
	"TunaSweeper.UI.SpeechBubble.AnchorDirections",
	TunaSweeperSpeechBubbleTests::Flags)

bool FTunaSweeperSpeechBubbleAnchorDirectionsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FVector2D BodySize(200.0f, 80.0f);
	const FVector2D NoneAnchor = UTunaSweeperScreenSpaceSpeechBubbleWidget::CalculateLocalAnchorPoint(
		ETunaSweeperSpeechBubbleTailDirection::None,
		BodySize);
	TestTrue(TEXT("No-tail anchor is the body center"), NoneAnchor.Equals(BodySize * 0.5f, KINDA_SMALL_NUMBER));

	constexpr float TailSize = 34.0f;
	constexpr float TailReserve = 24.0f;
	constexpr float TailTipRadius = 16.0f;
	for (const TunaSweeperSpeechBubbleTests::FDirectionCase& TestCase : TunaSweeperSpeechBubbleTests::DirectionCases)
	{
		const FVector2D Padding(
			TestCase.UnitVector.X == 0.0f ? 0.0f : TailReserve,
			TestCase.UnitVector.Y == 0.0f ? 0.0f : TailReserve);
		const FVector2D RootSize = BodySize + Padding;
		FVector2D TailCenter = RootSize * 0.5f;
		if (TestCase.UnitVector.X < 0.0f)
		{
			TailCenter.X = TailSize * 0.5f;
		}
		else if (TestCase.UnitVector.X > 0.0f)
		{
			TailCenter.X = RootSize.X - TailSize * 0.5f;
		}
		if (TestCase.UnitVector.Y < 0.0f)
		{
			TailCenter.Y = TailSize * 0.5f;
		}
		else if (TestCase.UnitVector.Y > 0.0f)
		{
			TailCenter.Y = RootSize.Y - TailSize * 0.5f;
		}

		const FVector2D ExpectedTip = TailCenter + TestCase.UnitVector * TailTipRadius;
		const FVector2D ActualTip = UTunaSweeperScreenSpaceSpeechBubbleWidget::CalculateLocalAnchorPoint(
			TestCase.Direction,
			BodySize);
		TestTrue(
			*FString::Printf(TEXT("Tail tip aligns for direction %d"), static_cast<int32>(TestCase.Direction)),
			ActualTip.Equals(ExpectedTip, KINDA_SMALL_NUMBER));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSpeechBubbleTargetReplacementTest,
	"TunaSweeper.UI.SpeechBubble.TargetReplacement",
	TunaSweeperSpeechBubbleTests::Flags)

bool FTunaSweeperSpeechBubbleTargetReplacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperSpeechBubbleInternal;

	const FTargetIdentity ScreenA = TunaSweeperSpeechBubbleTests::MakeScreenTarget(FVector2D(100.0f, 200.0f));
	const FTargetIdentity ScreenWithinOnePixel = TunaSweeperSpeechBubbleTests::MakeScreenTarget(FVector2D(100.6f, 200.6f));
	const FTargetIdentity ScreenDifferent = TunaSweeperSpeechBubbleTests::MakeScreenTarget(FVector2D(102.0f, 200.0f));
	TestTrue(TEXT("Screen targets within one logical pixel replace"), AreSameTarget(ScreenA, ScreenWithinOnePixel));
	TestFalse(TEXT("Different screen targets coexist"), AreSameTarget(ScreenA, ScreenDifferent));

	const FTargetIdentity WorldA = TunaSweeperSpeechBubbleTests::MakeWorldTarget(FVector(10.0, 20.0, 30.0));
	const FTargetIdentity WorldWithinOneCentimeter = TunaSweeperSpeechBubbleTests::MakeWorldTarget(FVector(10.5, 20.5, 30.5));
	const FTargetIdentity WorldDifferent = TunaSweeperSpeechBubbleTests::MakeWorldTarget(FVector(12.0, 20.0, 30.0));
	TestTrue(TEXT("World targets within one centimeter replace"), AreSameTarget(WorldA, WorldWithinOneCentimeter));
	TestFalse(TEXT("Different world targets coexist"), AreSameTarget(WorldA, WorldDifferent));
	TestFalse(TEXT("Different source types never replace"), AreSameTarget(ScreenA, WorldA));

	AActor* ActorA = NewObject<AActor>();
	AActor* ActorB = NewObject<AActor>();
	FTargetIdentity ActorTargetA;
	ActorTargetA.Type = ETargetType::Actor;
	ActorTargetA.Actor = ActorA;
	FTargetIdentity SameActorTarget = ActorTargetA;
	FTargetIdentity ActorTargetB = ActorTargetA;
	ActorTargetB.Actor = ActorB;
	TestTrue(TEXT("The same actor target replaces"), AreSameTarget(ActorTargetA, SameActorTarget));
	TestFalse(TEXT("Different actor targets coexist"), AreSameTarget(ActorTargetA, ActorTargetB));

	TArray<FTargetIdentity> SimultaneousTargets;
	SimultaneousTargets.Add(ScreenA);
	SimultaneousTargets.Add(WorldA);
	SimultaneousTargets.Add(ActorTargetA);
	for (int32 A = 0; A < SimultaneousTargets.Num(); ++A)
	{
		for (int32 B = A + 1; B < SimultaneousTargets.Num(); ++B)
		{
			TestFalse(TEXT("Distinct targets can remain simultaneous"), AreSameTarget(SimultaneousTargets[A], SimultaneousTargets[B]));
		}
	}
	TestEqual(TEXT("Three distinct targets remain"), SimultaneousTargets.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSpeechBubbleLifetimeAndHandleTest,
	"TunaSweeper.UI.SpeechBubble.LifetimeAndHandle",
	TunaSweeperSpeechBubbleTests::Flags)

bool FTunaSweeperSpeechBubbleLifetimeAndHandleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperSpeechBubbleInternal;
	TestFalse(TEXT("Positive duration remains before expiry"), HasExpired(2.0f, 1.99f));
	TestTrue(TEXT("Positive duration expires at its limit"), HasExpired(2.0f, 2.0f));
	TestFalse(TEXT("Zero duration remains indefinitely"), HasExpired(0.0f, 100000.0f));
	TestTrue(TEXT("Finite duration is accepted"), IsValidDuration(2.0f));
	TestFalse(TEXT("NaN duration is rejected"), IsValidDuration(std::numeric_limits<float>::quiet_NaN()));

	struct FHandleItem
	{
		FGuid Handle;
	};
	TArray<FHandleItem> Items;
	Items.Add({FGuid::NewGuid()});
	Items.Add({FGuid::NewGuid()});
	TestEqual(TEXT("Existing handle is found"), FindHandleIndex(Items, Items[1].Handle), 1);
	TestEqual(TEXT("Unknown handle is rejected"), FindHandleIndex(Items, FGuid::NewGuid()), INDEX_NONE);
	TestEqual(TEXT("Invalid handle is rejected"), FindHandleIndex(Items, FGuid()), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSpeechBubbleInvalidInputTest,
	"TunaSweeper.UI.SpeechBubble.InvalidInput",
	TunaSweeperSpeechBubbleTests::Flags)

bool FTunaSweeperSpeechBubbleInvalidInputTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UTunaSweeperSpeechBubbleSubsystem* Subsystem = NewObject<UTunaSweeperSpeechBubbleSubsystem>(GameInstance);
	TestNotNull(TEXT("Test subsystem exists"), Subsystem);
	if (!Subsystem)
	{
		return false;
	}

	const double NaN = std::numeric_limits<double>::quiet_NaN();
	const float FloatNaN = std::numeric_limits<float>::quiet_NaN();
	TestFalse(TEXT("Empty text is rejected"), Subsystem->ShowAtScreen(FText::GetEmpty(), FVector2D::ZeroVector, ETunaSweeperSpeechBubbleTailDirection::None).IsValid());
	TestFalse(TEXT("NaN screen position is rejected"), Subsystem->ShowAtScreen(FText::FromString(TEXT("Test")), FVector2D(NaN, 0.0), ETunaSweeperSpeechBubbleTailDirection::None).IsValid());
	TestFalse(TEXT("NaN world position is rejected"), Subsystem->ShowAtWorld(FText::FromString(TEXT("Test")), FVector(NaN, 0.0, 0.0), ETunaSweeperSpeechBubbleTailDirection::None).IsValid());
	TestFalse(TEXT("Null actor is rejected"), Subsystem->ShowForActor(FText::FromString(TEXT("Test")), nullptr, FVector::ZeroVector, ETunaSweeperSpeechBubbleTailDirection::None).IsValid());
	TestFalse(TEXT("NaN duration is rejected"), Subsystem->ShowAtScreen(FText::FromString(TEXT("Test")), FVector2D::ZeroVector, ETunaSweeperSpeechBubbleTailDirection::None, FloatNaN).IsValid());
	TestFalse(TEXT("Invalid tail enum is rejected"), Subsystem->ShowAtScreen(FText::FromString(TEXT("Test")), FVector2D::ZeroVector, static_cast<ETunaSweeperSpeechBubbleTailDirection>(255)).IsValid());
	TestFalse(TEXT("Invalid hide handle is rejected"), Subsystem->HideSpeechBubble(FGuid()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSpeechBubbleTextureAssetsTest,
	"TunaSweeper.UI.SpeechBubble.TextureAssets",
	TunaSweeperSpeechBubbleTests::Flags)

bool FTunaSweeperSpeechBubbleTextureAssetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	struct FTextureCase
	{
		const TCHAR* ObjectPath;
		int32 ExpectedWidth;
		int32 ExpectedHeight;
	};
	const FTextureCase Cases[] =
	{
		{TEXT("/Game/UI/SpeechBubble/T_SpeechBubble_Body.T_SpeechBubble_Body"), 512, 256},
		{TEXT("/Game/UI/SpeechBubble/T_SpeechBubble_Tail.T_SpeechBubble_Tail"), 128, 128},
	};

	for (const FTextureCase& TestCase : Cases)
	{
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TestCase.ObjectPath);
		TestNotNull(TestCase.ObjectPath, Texture);
		if (!Texture)
		{
			continue;
		}
		TestEqual(TEXT("Texture source width"), Texture->Source.GetSizeX(), static_cast<int64>(TestCase.ExpectedWidth));
		TestEqual(TEXT("Texture source height"), Texture->Source.GetSizeY(), static_cast<int64>(TestCase.ExpectedHeight));
		TestEqual(TEXT("Texture uses UI LOD group"), Texture->LODGroup, TEXTUREGROUP_UI);
		TestEqual(TEXT("Texture has no mipmaps"), Texture->MipGenSettings, TMGS_NoMipmaps);
		TestTrue(TEXT("Texture uses sRGB"), Texture->SRGB);

		const ETextureSourceFormat SourceFormat = Texture->Source.GetFormat();
		const bool bHasByteAlpha = SourceFormat == TSF_BGRA8;
		TestTrue(TEXT("Texture source has an alpha-capable format"), bHasByteAlpha);
		bool bHasTransparentPixel = false;
		bool bHasVisiblePixel = false;
		if (bHasByteAlpha)
		{
			const uint8* SourcePixels = Texture->Source.LockMipReadOnly(0);
			if (SourcePixels)
			{
				const int64 PixelCount = Texture->Source.GetSizeX() * Texture->Source.GetSizeY();
				const int64 BytesPerPixel = Texture->Source.GetBytesPerPixel();
				for (int64 PixelIndex = 0; PixelIndex < PixelCount; ++PixelIndex)
				{
					const uint8 Alpha = SourcePixels[PixelIndex * BytesPerPixel + 3];
					bHasTransparentPixel |= Alpha < 255;
					bHasVisiblePixel |= Alpha > 0;
					if (bHasTransparentPixel && bHasVisiblePixel)
					{
						break;
					}
				}
				Texture->Source.UnlockMip(0);
			}
		}
		TestTrue(TEXT("Texture source contains transparent pixels"), bHasTransparentPixel);
		TestTrue(TEXT("Texture source contains visible pixels"), bHasVisiblePixel);
	}
	return true;
}

#endif
