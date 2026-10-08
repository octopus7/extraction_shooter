#pragma once

// Engine-independent policy/state so startup and failure paths can be tested without a renderer.
namespace TunaSweeperStartupLogo
{
	inline bool ShouldShow(bool IsDemo, bool AlreadyAttempted, bool DailyLimit, bool SeenToday)
	{
		return !IsDemo && !AlreadyAttempted && (!DailyLimit || !SeenToday);
	}

	enum class EPhase { Opening, VideoFadeIn, Video, Crossfade, Still, FadeOut, Black, Done };

	struct FFlow
	{
		EPhase Phase = EPhase::Opening;
		float Elapsed = 0.0f;
		float FadeSeconds = 0.25f;
		float StillSeconds = 1.0f;
		float BlackSeconds = 0.2f;
		float OpeningTimeoutSeconds = 10.0f;
		float VideoTimeoutSeconds = 30.0f;
		float BlackOpacity = 1.0f;
		float StillOpacity = 0.0f;
		float CrossfadeStartBlack = 0.0f;
		bool VideoCompleted = false;

		void Enter(EPhase Next) { Phase = Next; Elapsed = 0.0f; }
		void Opened() { if (Phase == EPhase::Opening) Enter(EPhase::VideoFadeIn); }
		bool SkipStill()
		{
			if (Phase != EPhase::Still) return false;
			Enter(EPhase::FadeOut);
			return true;
		}
		void EndVideo(bool Completed)
		{
			if (Phase != EPhase::Opening && Phase != EPhase::VideoFadeIn && Phase != EPhase::Video) return;
			VideoCompleted = Completed;
			CrossfadeStartBlack = BlackOpacity;
			Enter(EPhase::Crossfade);
		}
		static float Alpha(float Time, float Duration)
		{
			if (Duration <= 0.0f || Time >= Duration) return 1.0f;
			return Time <= 0.0f ? 0.0f : Time / Duration;
		}
		void Tick(float Delta)
		{
			Elapsed += Delta > 0.0f ? Delta : 0.0f;
			switch (Phase)
			{
			case EPhase::Opening:
				if (Elapsed >= OpeningTimeoutSeconds) EndVideo(false);
				break;
			case EPhase::VideoFadeIn:
				BlackOpacity = 1.0f - Alpha(Elapsed, FadeSeconds);
				if (BlackOpacity <= 0.0f) Enter(EPhase::Video);
				break;
			case EPhase::Video:
				if (Elapsed >= VideoTimeoutSeconds) EndVideo(false);
				break;
			case EPhase::Crossfade:
				StillOpacity = Alpha(Elapsed, FadeSeconds);
				BlackOpacity = CrossfadeStartBlack * (1.0f - StillOpacity);
				if (StillOpacity >= 1.0f) Enter(EPhase::Still);
				break;
			case EPhase::Still:
				if (Elapsed >= StillSeconds) Enter(EPhase::FadeOut);
				break;
			case EPhase::FadeOut:
				BlackOpacity = Alpha(Elapsed, FadeSeconds);
				if (BlackOpacity >= 1.0f) Enter(EPhase::Black);
				break;
			case EPhase::Black:
				if (Elapsed >= BlackSeconds) Enter(EPhase::Done);
				break;
			case EPhase::Done: break;
			}
		}
	};
}
