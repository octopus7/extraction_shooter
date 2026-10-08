#include "../../TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperStartupLogoFlow.h"
#include <cstdio>
#include <cstdlib>

int main()
{
	using namespace TunaSweeperStartupLogo;
	int Failures = 0;
	auto Check = [&](bool Value, const char* Message) { if (!Value) { std::printf("FAIL: %s\n", Message); ++Failures; } };
	Check(ShouldShow(false, false, false, false), "Main launches play logo");
	Check(ShouldShow(false, false, false, true), "Disabled daily limit ignores today's record");
	Check(!ShouldShow(true, false, false, false), "Demo bypasses logo");
	Check(!ShouldShow(false, true, false, false), "Returning to title never repeats logo");
	Check(!ShouldShow(false, false, true, true), "Enabled daily limit skips today's logo");
	Check(ShouldShow(false, false, true, false), "Enabled daily limit plays on a new day");
	FFlow Flow;
	Check(Flow.BlackOpacity == 1.0f && Flow.StillOpacity == 0.0f, "Startup is completely black");
	Flow.Opened();
	Flow.Tick(0.35f);
	Check(Flow.BlackOpacity > 0.4f && Flow.BlackOpacity < 0.6f, "Video fades in gradually");
	Flow.Tick(0.36f);
	Check(Flow.Phase == EPhase::Video && Flow.BlackOpacity == 0.0f, "Video stays fully visible until ended");
	Flow.EndVideo(true);
	Flow.Tick(0.35f);
	Check(Flow.StillOpacity > 0.4f && Flow.StillOpacity < 0.6f && Flow.BlackOpacity == 0.0f, "Crossfade has no black interruption");
	Flow.Opened(); // A stale media callback cannot restart the sequence.
	Check(Flow.Phase == EPhase::Crossfade, "Late open event is ignored");
	Flow.Tick(0.36f);
	Check(Flow.Phase == EPhase::Still && Flow.StillOpacity == 1.0f, "Still replaces video");
	Flow.Tick(1.51f);
	Check(Flow.Phase == EPhase::FadeOut, "Still holds before fade out");
	Flow.Tick(0.71f);
	Check(Flow.Phase == EPhase::Black && Flow.BlackOpacity == 1.0f, "Black precedes title");
	Flow.Tick(0.21f);
	Check(Flow.Phase == EPhase::Done && Flow.VideoCompleted, "Successful sequence completes");
	FFlow Failed;
	Failed.Tick(10.1f);
	Check(Failed.Phase == EPhase::Crossfade && !Failed.VideoCompleted, "Open timeout falls back without marking video seen");
	Failed.Tick(0.71f);
	Check(Failed.Phase == EPhase::Still && Failed.BlackOpacity == 0.0f, "Fallback reveals still from black");
	FFlow Stalled;
	Stalled.Opened(); Stalled.Tick(0.71f); Stalled.Tick(30.1f);
	Check(Stalled.Phase == EPhase::Crossfade && !Stalled.VideoCompleted, "Stalled playback cannot trap the user");
	std::printf("Startup logo tests: %d failure(s)\n", Failures);
	return Failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
