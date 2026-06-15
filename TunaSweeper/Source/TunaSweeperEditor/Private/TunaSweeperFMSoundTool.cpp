#include "TunaSweeperFMSoundTool.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "IAssetTools.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"
#include "Styling/CoreStyle.h"
#include "ToolMenus.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SavePackage.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TunaSweeperFMSoundTool"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperFMSoundTool, Log, All);

namespace TunaSweeperFMSound
{
	const FName TabName(TEXT("TunaSweeperFMSoundComposer"));
	const FString SettingsRelativePath = TEXT("Data/FMSoundPresets.json");
	const FString ExportRelativeDirectory = TEXT("Audio/SFX/FMTemp");
	const FString ExportAssetPath = TEXT("/Game/Audio/SFX/FMTemp");
	constexpr int32 DefaultSampleRate = 44100;
	constexpr int32 ChannelCount = 1;

	enum class EPresetKind : uint8
	{
		Explosion,
		Gunshot,
		Hit,
		Ricochet,
		UiBeep,
		CatMeow
	};

	struct FPreset
	{
		FString Id;
		EPresetKind Kind = EPresetKind::Explosion;
		float DurationSeconds = 0.5f;
		float BasePitchHz = 180.0f;
		float PitchSweepSemitones = -12.0f;
		float Decay = 0.4f;
		float FmRatio = 2.0f;
		float FmAmount = 4.0f;
		float NoiseAmount = 0.3f;
		float Brightness = 0.6f;
		float Distortion = 0.15f;
		float Randomness = 0.2f;
		float OutputGainDb = -3.0f;
		int32 Seed = 1;
	};

	struct FRenderResult
	{
		TArray<int16> Samples;
		int32 SampleRate = DefaultSampleRate;
		float DurationSeconds = 0.0f;
	};

	FString GetSettingsPath()
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), SettingsRelativePath);
	}

	FString GetExportDirectory()
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), ExportRelativeDirectory);
	}

	FString KindToString(EPresetKind Kind)
	{
		switch (Kind)
		{
		case EPresetKind::Explosion:
			return TEXT("Explosion");
		case EPresetKind::Gunshot:
			return TEXT("Gunshot");
		case EPresetKind::Hit:
			return TEXT("Hit");
		case EPresetKind::Ricochet:
			return TEXT("Ricochet");
		case EPresetKind::UiBeep:
			return TEXT("UiBeep");
		case EPresetKind::CatMeow:
			return TEXT("CatMeow");
		default:
			return TEXT("Explosion");
		}
	}

	EPresetKind KindFromString(const FString& Kind)
	{
		if (Kind.Equals(TEXT("Gunshot"), ESearchCase::IgnoreCase))
		{
			return EPresetKind::Gunshot;
		}
		if (Kind.Equals(TEXT("Hit"), ESearchCase::IgnoreCase))
		{
			return EPresetKind::Hit;
		}
		if (Kind.Equals(TEXT("Ricochet"), ESearchCase::IgnoreCase))
		{
			return EPresetKind::Ricochet;
		}
		if (Kind.Equals(TEXT("UiBeep"), ESearchCase::IgnoreCase) || Kind.Equals(TEXT("UIBeep"), ESearchCase::IgnoreCase))
		{
			return EPresetKind::UiBeep;
		}
		if (Kind.Equals(TEXT("CatMeow"), ESearchCase::IgnoreCase))
		{
			return EPresetKind::CatMeow;
		}
		return EPresetKind::Explosion;
	}

	float ReadFloatField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, float DefaultValue)
	{
		if (!Object.IsValid())
		{
			return DefaultValue;
		}

		double Value = 0.0;
		return Object->TryGetNumberField(FieldName, Value) ? static_cast<float>(Value) : DefaultValue;
	}

	int32 ReadIntField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, int32 DefaultValue)
	{
		if (!Object.IsValid())
		{
			return DefaultValue;
		}

		int32 Value = 0;
		return Object->TryGetNumberField(FieldName, Value) ? Value : DefaultValue;
	}

	float ReadRatioField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, float DefaultValue)
	{
		if (!Object.IsValid())
		{
			return DefaultValue;
		}

		double Value = 0.0;
		if (!Object->TryGetNumberField(FieldName, Value))
		{
			return DefaultValue;
		}

		return TunaSweeperDataValues::ToRatioFloat(
			TunaSweeperDataValues::ClampRatioValue(FMath::RoundToInt(Value)));
	}

	FString ReadStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, const FString& DefaultValue)
	{
		if (!Object.IsValid())
		{
			return DefaultValue;
		}

		FString Value;
		return Object->TryGetStringField(FieldName, Value) ? Value : DefaultValue;
	}

	FPreset ReadPreset(const TSharedPtr<FJsonObject>& Object)
	{
		FPreset Preset;
		Preset.Id = ReadStringField(Object, TEXT("id"), TEXT("SFX_FM_Explosion"));
		Preset.Kind = KindFromString(ReadStringField(Object, TEXT("kind"), TEXT("Explosion")));
		Preset.DurationSeconds = FMath::Clamp(ReadFloatField(Object, TEXT("duration_seconds"), Preset.DurationSeconds), 0.03f, 5.0f);
		Preset.BasePitchHz = FMath::Clamp(ReadFloatField(Object, TEXT("base_pitch_hz"), Preset.BasePitchHz), 20.0f, 8000.0f);
		Preset.PitchSweepSemitones = FMath::Clamp(ReadFloatField(Object, TEXT("pitch_sweep_semitones"), Preset.PitchSweepSemitones), -48.0f, 48.0f);
		Preset.Decay = FMath::Clamp(ReadFloatField(Object, TEXT("decay"), Preset.Decay), 0.01f, 3.0f);
		Preset.FmRatio = FMath::Clamp(ReadRatioField(Object, TEXT("fm_ratio"), Preset.FmRatio), 0.125f, 16.0f);
		Preset.FmAmount = FMath::Clamp(ReadFloatField(Object, TEXT("fm_amount"), Preset.FmAmount), 0.0f, 18.0f);
		Preset.NoiseAmount = FMath::Clamp(ReadFloatField(Object, TEXT("noise_amount"), Preset.NoiseAmount), 0.0f, 1.0f);
		Preset.Brightness = FMath::Clamp(ReadFloatField(Object, TEXT("brightness"), Preset.Brightness), 0.0f, 1.0f);
		Preset.Distortion = FMath::Clamp(ReadFloatField(Object, TEXT("distortion"), Preset.Distortion), 0.0f, 1.0f);
		Preset.Randomness = FMath::Clamp(ReadFloatField(Object, TEXT("randomness"), Preset.Randomness), 0.0f, 1.0f);
		Preset.OutputGainDb = FMath::Clamp(ReadFloatField(Object, TEXT("output_gain_db"), Preset.OutputGainDb), -36.0f, 12.0f);
		Preset.Seed = ReadIntField(Object, TEXT("seed"), Preset.Seed);
		return Preset;
	}

	TSharedRef<FJsonObject> WritePreset(const FPreset& Preset)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("id"), Preset.Id);
		Object->SetStringField(TEXT("kind"), KindToString(Preset.Kind));
		Object->SetNumberField(TEXT("duration_seconds"), Preset.DurationSeconds);
		Object->SetNumberField(TEXT("base_pitch_hz"), Preset.BasePitchHz);
		Object->SetNumberField(TEXT("pitch_sweep_semitones"), Preset.PitchSweepSemitones);
		Object->SetNumberField(TEXT("decay"), Preset.Decay);
		Object->SetNumberField(
			TEXT("fm_ratio"),
			TunaSweeperDataValues::ClampRatioValue(FMath::RoundToInt(
				Preset.FmRatio * TunaSweeperDataValues::FixedPointBasis)));
		Object->SetNumberField(TEXT("fm_amount"), Preset.FmAmount);
		Object->SetNumberField(TEXT("noise_amount"), Preset.NoiseAmount);
		Object->SetNumberField(TEXT("brightness"), Preset.Brightness);
		Object->SetNumberField(TEXT("distortion"), Preset.Distortion);
		Object->SetNumberField(TEXT("randomness"), Preset.Randomness);
		Object->SetNumberField(TEXT("output_gain_db"), Preset.OutputGainDb);
		Object->SetNumberField(TEXT("seed"), Preset.Seed);
		return Object;
	}

	TArray<FPreset> MakeDefaultPresets()
	{
		TArray<FPreset> Presets;

		FPreset Explosion;
		Explosion.Id = TEXT("SFX_FM_Explosion");
		Explosion.Kind = EPresetKind::Explosion;
		Explosion.DurationSeconds = 0.85f;
		Explosion.BasePitchHz = 92.0f;
		Explosion.PitchSweepSemitones = -24.0f;
		Explosion.Decay = 0.52f;
		Explosion.FmRatio = 1.65f;
		Explosion.FmAmount = 5.8f;
		Explosion.NoiseAmount = 0.72f;
		Explosion.Brightness = 0.42f;
		Explosion.Distortion = 0.38f;
		Explosion.Randomness = 0.28f;
		Explosion.OutputGainDb = -2.0f;
		Explosion.Seed = 101;
		Presets.Add(Explosion);

		FPreset Gunshot;
		Gunshot.Id = TEXT("SFX_FM_Gunshot");
		Gunshot.Kind = EPresetKind::Gunshot;
		Gunshot.DurationSeconds = 0.28f;
		Gunshot.BasePitchHz = 155.0f;
		Gunshot.PitchSweepSemitones = -9.0f;
		Gunshot.Decay = 0.11f;
		Gunshot.FmRatio = 3.15f;
		Gunshot.FmAmount = 4.2f;
		Gunshot.NoiseAmount = 0.82f;
		Gunshot.Brightness = 0.78f;
		Gunshot.Distortion = 0.44f;
		Gunshot.Randomness = 0.18f;
		Gunshot.OutputGainDb = -3.0f;
		Gunshot.Seed = 202;
		Presets.Add(Gunshot);

		FPreset Hit;
		Hit.Id = TEXT("SFX_FM_Hit");
		Hit.Kind = EPresetKind::Hit;
		Hit.DurationSeconds = 0.25f;
		Hit.BasePitchHz = 115.0f;
		Hit.PitchSweepSemitones = -15.0f;
		Hit.Decay = 0.18f;
		Hit.FmRatio = 1.2f;
		Hit.FmAmount = 2.7f;
		Hit.NoiseAmount = 0.46f;
		Hit.Brightness = 0.5f;
		Hit.Distortion = 0.25f;
		Hit.Randomness = 0.2f;
		Hit.OutputGainDb = -4.0f;
		Hit.Seed = 303;
		Presets.Add(Hit);

		FPreset Ricochet;
		Ricochet.Id = TEXT("SFX_FM_Ricochet");
		Ricochet.Kind = EPresetKind::Ricochet;
		Ricochet.DurationSeconds = 0.38f;
		Ricochet.BasePitchHz = 1280.0f;
		Ricochet.PitchSweepSemitones = 18.0f;
		Ricochet.Decay = 0.22f;
		Ricochet.FmRatio = 2.72f;
		Ricochet.FmAmount = 6.4f;
		Ricochet.NoiseAmount = 0.22f;
		Ricochet.Brightness = 0.95f;
		Ricochet.Distortion = 0.08f;
		Ricochet.Randomness = 0.12f;
		Ricochet.OutputGainDb = -8.0f;
		Ricochet.Seed = 404;
		Presets.Add(Ricochet);

		FPreset UiBeep;
		UiBeep.Id = TEXT("SFX_FM_UIBeep");
		UiBeep.Kind = EPresetKind::UiBeep;
		UiBeep.DurationSeconds = 0.16f;
		UiBeep.BasePitchHz = 760.0f;
		UiBeep.PitchSweepSemitones = 4.0f;
		UiBeep.Decay = 0.1f;
		UiBeep.FmRatio = 2.0f;
		UiBeep.FmAmount = 1.6f;
		UiBeep.NoiseAmount = 0.0f;
		UiBeep.Brightness = 0.88f;
		UiBeep.Distortion = 0.0f;
		UiBeep.Randomness = 0.05f;
		UiBeep.OutputGainDb = -9.0f;
		UiBeep.Seed = 505;
		Presets.Add(UiBeep);

		FPreset CatMeow;
		CatMeow.Id = TEXT("SFX_FM_CatMeow");
		CatMeow.Kind = EPresetKind::CatMeow;
		CatMeow.DurationSeconds = 0.72f;
		CatMeow.BasePitchHz = 420.0f;
		CatMeow.PitchSweepSemitones = 9.0f;
		CatMeow.Decay = 0.64f;
		CatMeow.FmRatio = 2.35f;
		CatMeow.FmAmount = 2.1f;
		CatMeow.NoiseAmount = 0.08f;
		CatMeow.Brightness = 0.72f;
		CatMeow.Distortion = 0.02f;
		CatMeow.Randomness = 0.08f;
		CatMeow.OutputGainDb = -8.0f;
		CatMeow.Seed = 606;
		Presets.Add(CatMeow);

		return Presets;
	}

	FString SanitizeAssetName(FString Name)
	{
		Name.TrimStartAndEndInline();
		if (Name.IsEmpty())
		{
			Name = TEXT("SFX_FM_Sound");
		}

		for (TCHAR& Character : Name)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
			{
				Character = TEXT('_');
			}
		}

		if (!FChar::IsAlpha(Name[0]) && Name[0] != TEXT('_'))
		{
			Name = TEXT("SFX_") + Name;
		}
		return Name;
	}

	float NoteSweepMultiplier(float SweepSemitones, float NormalizedTime, float Curve = 1.0f)
	{
		const float Amount = SweepSemitones * FMath::Pow(FMath::Clamp(NormalizedTime, 0.0f, 1.0f), Curve);
		return FMath::Pow(2.0f, Amount / 12.0f);
	}

	float EnvelopeExp(float TimeSeconds, float DecaySeconds, float AttackSeconds)
	{
		const float Attack = AttackSeconds > 0.0f ? (1.0f - FMath::Exp(-TimeSeconds / AttackSeconds)) : 1.0f;
		const float Decay = FMath::Exp(-TimeSeconds / FMath::Max(DecaySeconds, 0.001f));
		return FMath::Clamp(Attack * Decay, 0.0f, 1.0f);
	}

	float FmTone(float TimeSeconds, float CarrierHz, float Ratio, float Index, float ModulationScale, float PhaseOffset = 0.0f)
	{
		const float Modulator = FMath::Sin(2.0f * UE_PI * CarrierHz * Ratio * TimeSeconds + PhaseOffset * 0.37f);
		return FMath::Sin(2.0f * UE_PI * CarrierHz * TimeSeconds + Modulator * Index * ModulationScale + PhaseOffset);
	}

	float SoftClip(float Sample, float Amount)
	{
		if (Amount <= 0.001f)
		{
			return Sample;
		}

		const float Drive = 1.0f + Amount * 14.0f;
		return FMath::Tanh(Sample * Drive) / FMath::Tanh(Drive);
	}

	struct FNoiseGenerator
	{
		explicit FNoiseGenerator(int32 InSeed)
			: State(static_cast<uint32>(InSeed == 0 ? 1 : InSeed))
		{
		}

		float NextSigned()
		{
			State = State * 1664525u + 1013904223u;
			const float Unit = static_cast<float>((State >> 8) & 0x00ffffffu) / static_cast<float>(0x00ffffffu);
			return Unit * 2.0f - 1.0f;
		}

		uint32 State = 1u;
	};

	float RenderExplosion(const FPreset& Preset, float TimeSeconds, float NormalizedTime, float WhiteNoise, float SmoothNoise)
	{
		const float PitchJitter = 1.0f;
		const float SweepPitch = Preset.BasePitchHz * PitchJitter * NoteSweepMultiplier(Preset.PitchSweepSemitones, NormalizedTime, 0.42f);
		const float BodyEnv = EnvelopeExp(TimeSeconds, Preset.Decay, 0.001f);
		const float RumbleEnv = EnvelopeExp(TimeSeconds, FMath::Max(Preset.Decay * 1.8f, 0.25f), 0.02f);
		const float Body = FmTone(TimeSeconds, SweepPitch, Preset.FmRatio, Preset.FmAmount, BodyEnv, 0.2f) * BodyEnv;
		const float Rumble = FMath::Sin(2.0f * UE_PI * FMath::Max(22.0f, Preset.BasePitchHz * 0.45f) * TimeSeconds) * RumbleEnv;
		const float Tail = SmoothNoise * Preset.NoiseAmount * EnvelopeExp(TimeSeconds, Preset.Decay * 1.25f, 0.0f);
		const float Click = NormalizedTime < 0.018f ? WhiteNoise * (1.0f - NormalizedTime / 0.018f) : 0.0f;
		return Body * 0.55f + Rumble * 0.32f + Tail * 0.7f + Click * 0.35f;
	}

	float RenderGunshot(const FPreset& Preset, float TimeSeconds, float NormalizedTime, float WhiteNoise, float SmoothNoise)
	{
		const float BaseHz = Preset.BasePitchHz;
		const float BodyHz = BaseHz * NoteSweepMultiplier(Preset.PitchSweepSemitones, NormalizedTime, 0.25f);
		const float BodyEnv = EnvelopeExp(TimeSeconds, Preset.Decay, 0.001f);
		const float Body = FmTone(TimeSeconds, BodyHz, Preset.FmRatio, Preset.FmAmount, BodyEnv, 0.6f) * BodyEnv;
		const float CrackEnv = EnvelopeExp(TimeSeconds, 0.028f + Preset.Decay * 0.12f, 0.0f);
		const float Crack = (WhiteNoise - SmoothNoise * 0.35f) * CrackEnv * Preset.NoiseAmount;
		const float RingEnv = EnvelopeExp(TimeSeconds, 0.055f, 0.001f);
		const float Ring = FmTone(TimeSeconds, BaseHz * 9.0f, 2.47f, Preset.FmAmount * 0.45f, RingEnv, 1.8f) * RingEnv;
		const float Click = NormalizedTime < 0.012f ? WhiteNoise * (1.0f - NormalizedTime / 0.012f) : 0.0f;
		return Click * 0.75f + Crack * 0.8f + Body * 0.5f + Ring * 0.22f;
	}

	float RenderHit(const FPreset& Preset, float TimeSeconds, float NormalizedTime, float WhiteNoise, float SmoothNoise)
	{
		const float ThumpHz = Preset.BasePitchHz * NoteSweepMultiplier(Preset.PitchSweepSemitones, NormalizedTime, 0.35f);
		const float ThumpEnv = EnvelopeExp(TimeSeconds, Preset.Decay, 0.001f);
		const float Thump = FmTone(TimeSeconds, ThumpHz, Preset.FmRatio, Preset.FmAmount, ThumpEnv, 0.9f) * ThumpEnv;
		const float SlapEnv = EnvelopeExp(TimeSeconds, FMath::Min(Preset.Decay * 0.55f, 0.09f), 0.001f);
		const float Slap = (WhiteNoise * 0.35f + SmoothNoise * 0.65f) * SlapEnv * Preset.NoiseAmount;
		return Thump * 0.75f + Slap * 0.65f;
	}

	float RenderRicochet(const FPreset& Preset, float TimeSeconds, float NormalizedTime, float WhiteNoise)
	{
		const float SweepA = Preset.BasePitchHz * NoteSweepMultiplier(Preset.PitchSweepSemitones, NormalizedTime, 0.55f);
		const float SweepB = Preset.BasePitchHz * 1.46f * NoteSweepMultiplier(-Preset.PitchSweepSemitones * 0.38f, NormalizedTime, 0.8f);
		const float RingEnv = EnvelopeExp(TimeSeconds, Preset.Decay, 0.001f);
		const float RingA = FmTone(TimeSeconds, SweepA, Preset.FmRatio, Preset.FmAmount, RingEnv, 0.2f) * RingEnv;
		const float RingB = FmTone(TimeSeconds, SweepB, Preset.FmRatio * 1.6f, Preset.FmAmount * 0.7f, RingEnv, 1.7f) * RingEnv;
		const float Tick = NormalizedTime < 0.02f ? WhiteNoise * (1.0f - NormalizedTime / 0.02f) * Preset.NoiseAmount : 0.0f;
		return RingA * 0.48f + RingB * 0.33f + Tick * 0.28f;
	}

	float RenderUiBeep(const FPreset& Preset, float TimeSeconds, float NormalizedTime)
	{
		const float Step = TimeSeconds > Preset.DurationSeconds * 0.46f ? 1.25f : 1.0f;
		const float Sweep = NoteSweepMultiplier(Preset.PitchSweepSemitones, NormalizedTime, 1.0f);
		const float Env = EnvelopeExp(TimeSeconds, Preset.Decay, 0.003f);
		const float Tone = FmTone(TimeSeconds, Preset.BasePitchHz * Step * Sweep, Preset.FmRatio, Preset.FmAmount, Env, 0.0f);
		return Tone * Env;
	}

	float RenderCatMeow(const FPreset& Preset, float TimeSeconds, float NormalizedTime, float SmoothNoise)
	{
		const float UpDown = FMath::Sin(FMath::Clamp(NormalizedTime, 0.0f, 1.0f) * UE_PI);
		const float Sweep = Preset.PitchSweepSemitones * UpDown - Preset.PitchSweepSemitones * 0.22f * NormalizedTime;
		const float Vibrato = FMath::Sin(2.0f * UE_PI * 6.5f * TimeSeconds) * 0.025f;
		const float VoiceHz = Preset.BasePitchHz * (1.0f + Vibrato) * FMath::Pow(2.0f, Sweep / 12.0f);
		const float Env = EnvelopeExp(TimeSeconds, Preset.Decay, 0.035f) * FMath::Clamp((1.0f - NormalizedTime) * 1.4f, 0.0f, 1.0f);
		const float Voice = FmTone(TimeSeconds, VoiceHz, Preset.FmRatio, Preset.FmAmount, Env, 0.4f) * Env;
		const float Formant = FmTone(TimeSeconds, VoiceHz * 2.65f, 0.52f, Preset.FmAmount * 0.75f, Env, 1.2f) * Env;
		const float Breath = SmoothNoise * Preset.NoiseAmount * EnvelopeExp(TimeSeconds, Preset.Decay * 0.8f, 0.02f);
		return Voice * 0.74f + Formant * 0.32f + Breath * 0.18f;
	}

	bool RenderPreset(const FPreset& Preset, int32 SampleRate, FRenderResult& OutResult)
	{
		const float DurationSeconds = FMath::Clamp(Preset.DurationSeconds, 0.03f, 5.0f);
		const int32 SampleCount = FMath::Max(1, FMath::CeilToInt(DurationSeconds * SampleRate));
		TArray<float> FloatSamples;
		FloatSamples.SetNumZeroed(SampleCount);

		FNoiseGenerator Random(Preset.Seed);
		float SmoothNoise = 0.0f;
		float OutputFilter = 0.0f;
		const float CutoffHz = FMath::Lerp(650.0f, 18000.0f, FMath::Pow(Preset.Brightness, 1.7f));
		const float FilterAlpha = FMath::Clamp((2.0f * UE_PI * CutoffHz) / (2.0f * UE_PI * CutoffHz + static_cast<float>(SampleRate)), 0.01f, 1.0f);
		const float NoiseSmoothAlpha = FMath::Lerp(0.03f, 0.42f, Preset.Brightness);

		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float TimeSeconds = static_cast<float>(Index) / static_cast<float>(SampleRate);
			const float NormalizedTime = FMath::Clamp(TimeSeconds / DurationSeconds, 0.0f, 1.0f);
			const float WhiteNoise = Random.NextSigned();
			SmoothNoise = FMath::Lerp(SmoothNoise, WhiteNoise, NoiseSmoothAlpha);

			float Sample = 0.0f;
			switch (Preset.Kind)
			{
			case EPresetKind::Explosion:
				Sample = RenderExplosion(Preset, TimeSeconds, NormalizedTime, WhiteNoise, SmoothNoise);
				break;
			case EPresetKind::Gunshot:
				Sample = RenderGunshot(Preset, TimeSeconds, NormalizedTime, WhiteNoise, SmoothNoise);
				break;
			case EPresetKind::Hit:
				Sample = RenderHit(Preset, TimeSeconds, NormalizedTime, WhiteNoise, SmoothNoise);
				break;
			case EPresetKind::Ricochet:
				Sample = RenderRicochet(Preset, TimeSeconds, NormalizedTime, WhiteNoise);
				break;
			case EPresetKind::UiBeep:
				Sample = RenderUiBeep(Preset, TimeSeconds, NormalizedTime);
				break;
			case EPresetKind::CatMeow:
				Sample = RenderCatMeow(Preset, TimeSeconds, NormalizedTime, SmoothNoise);
				break;
			default:
				Sample = 0.0f;
				break;
			}

			Sample = SoftClip(Sample, Preset.Distortion);
			OutputFilter += (Sample - OutputFilter) * FilterAlpha;
			FloatSamples[Index] = OutputFilter;
		}

		float Peak = 0.001f;
		for (float Sample : FloatSamples)
		{
			Peak = FMath::Max(Peak, FMath::Abs(Sample));
		}

		const float Gain = FMath::Pow(10.0f, Preset.OutputGainDb / 20.0f) * FMath::Min(1.0f, 0.96f / Peak);
		OutResult.SampleRate = SampleRate;
		OutResult.DurationSeconds = DurationSeconds;
		OutResult.Samples.SetNumUninitialized(SampleCount);
		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float Sample = FMath::Clamp(FloatSamples[Index] * Gain, -1.0f, 1.0f);
			OutResult.Samples[Index] = static_cast<int16>(FMath::RoundToInt(Sample * 32767.0f));
		}
		return true;
	}

	void AddBytes(TArray<uint8>& Bytes, const char* Text, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Bytes.Add(static_cast<uint8>(Text[Index]));
		}
	}

	void AddUInt16LE(TArray<uint8>& Bytes, uint16 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
	}

	void AddUInt32LE(TArray<uint8>& Bytes, uint32 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
	}

	bool WriteWavFile(const FString& FilePath, const FRenderResult& RenderResult)
	{
		if (RenderResult.Samples.Num() == 0)
		{
			return false;
		}

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true);

		const uint16 BitsPerSample = 16;
		const uint16 NumChannels = ChannelCount;
		const uint32 SampleRate = static_cast<uint32>(RenderResult.SampleRate);
		const uint32 DataSize = static_cast<uint32>(RenderResult.Samples.Num() * sizeof(int16));
		const uint32 ByteRate = SampleRate * NumChannels * BitsPerSample / 8;
		const uint16 BlockAlign = NumChannels * BitsPerSample / 8;

		TArray<uint8> Bytes;
		Bytes.Reserve(44 + DataSize);
		AddBytes(Bytes, "RIFF", 4);
		AddUInt32LE(Bytes, 36 + DataSize);
		AddBytes(Bytes, "WAVE", 4);
		AddBytes(Bytes, "fmt ", 4);
		AddUInt32LE(Bytes, 16);
		AddUInt16LE(Bytes, 1);
		AddUInt16LE(Bytes, NumChannels);
		AddUInt32LE(Bytes, SampleRate);
		AddUInt32LE(Bytes, ByteRate);
		AddUInt16LE(Bytes, BlockAlign);
		AddUInt16LE(Bytes, BitsPerSample);
		AddBytes(Bytes, "data", 4);
		AddUInt32LE(Bytes, DataSize);

		for (int16 Sample : RenderResult.Samples)
		{
			const uint16 Value = static_cast<uint16>(Sample);
			AddUInt16LE(Bytes, Value);
		}

		return FFileHelper::SaveArrayToFile(Bytes, *FilePath);
	}

	bool SavePackageForAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString PackageFilename = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
	}

	bool ImportWavAsSoundWave(const FString& SourceFile, const FString& AssetName, FString& OutObjectPath)
	{
		OutObjectPath.Reset();
		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperFMSoundTool, Error, TEXT("Missing WAV export file: %s"), *SourceFile);
			return false;
		}

		FModuleManager::Get().LoadModule(TEXT("AudioEditor"));

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = ExportAssetPath;
		ImportData->Filenames.Add(SourceFile);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperFMSoundTool, Error, TEXT("Failed to import WAV asset: %s"), *SourceFile);
			return false;
		}

		OutObjectPath = FString::Printf(TEXT("%s/%s.%s"), *ExportAssetPath, *AssetName, *AssetName);
		USoundWave* SoundWave = LoadObject<USoundWave>(nullptr, *OutObjectPath);
		if (!SoundWave)
		{
			UE_LOG(LogTunaSweeperFMSoundTool, Error, TEXT("Failed to load imported SoundWave: %s"), *OutObjectPath);
			return false;
		}

		SoundWave->Modify();
		SoundWave->bLooping = false;
		SoundWave->SoundGroup = SOUNDGROUP_Effects;
		SoundWave->PostEditChange();
		SoundWave->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(SoundWave);
		SavePackageForAsset(SoundWave);
		return true;
	}

	class STunaSweeperFMSoundComposer final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STunaSweeperFMSoundComposer) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			KindOptions.Add(MakeShared<FString>(TEXT("Explosion")));
			KindOptions.Add(MakeShared<FString>(TEXT("Gunshot")));
			KindOptions.Add(MakeShared<FString>(TEXT("Hit")));
			KindOptions.Add(MakeShared<FString>(TEXT("Ricochet")));
			KindOptions.Add(MakeShared<FString>(TEXT("UiBeep")));
			KindOptions.Add(MakeShared<FString>(TEXT("CatMeow")));

			LoadPresets();

			ChildSlot
			[
				SNew(SBorder)
				.Padding(12.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "FM Sound Composer"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(this, &STunaSweeperFMSoundComposer::GetSourceSummaryText)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakePresetRow()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f)
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[MakeIdRow()]
							+ SVerticalBox::Slot().AutoHeight()[MakeKindRow()]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("Duration", "Duration"), 0.03f, 5.0f, 0.01f, &FPreset::DurationSeconds)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("BasePitch", "Base Pitch Hz"), 20.0f, 8000.0f, 1.0f, &FPreset::BasePitchHz)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("PitchSweep", "Pitch Sweep Semitones"), -48.0f, 48.0f, 0.25f, &FPreset::PitchSweepSemitones)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("Decay", "Decay"), 0.01f, 3.0f, 0.01f, &FPreset::Decay)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("FmRatio", "FM Ratio"), 0.125f, 16.0f, 0.01f, &FPreset::FmRatio)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("FmAmount", "FM Amount"), 0.0f, 18.0f, 0.05f, &FPreset::FmAmount)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("NoiseAmount", "Noise Amount"), 0.0f, 1.0f, 0.01f, &FPreset::NoiseAmount)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("Brightness", "Brightness"), 0.0f, 1.0f, 0.01f, &FPreset::Brightness)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("Distortion", "Distortion"), 0.0f, 1.0f, 0.01f, &FPreset::Distortion)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("Randomness", "Randomness"), 0.0f, 1.0f, 0.01f, &FPreset::Randomness)]
							+ SVerticalBox::Slot().AutoHeight()[MakeFloatControl(LOCTEXT("OutputGain", "Output Gain dB"), -36.0f, 12.0f, 0.25f, &FPreset::OutputGainDb)]
							+ SVerticalBox::Slot().AutoHeight()[MakeSeedRow()]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						MakeActionRow()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SAssignNew(StatusText, STextBlock)
						.Text(Status)
					]
				]
			];
		}

	private:
		TArray<TSharedPtr<FPreset>> Presets;
		TArray<TSharedPtr<FString>> KindOptions;
		TSharedPtr<SComboBox<TSharedPtr<FPreset>>> PresetComboBox;
		TSharedPtr<SComboBox<TSharedPtr<FString>>> KindComboBox;
		TSharedPtr<STextBlock> StatusText;
		TStrongObjectPtr<USoundWaveProcedural> PreviewSound;
		FText Status = LOCTEXT("Ready", "Ready.");
		int32 SampleRate = DefaultSampleRate;
		int32 SelectedPresetIndex = 0;
		bool bDirty = false;

		FPreset* GetSelectedPreset() const
		{
			return Presets.IsValidIndex(SelectedPresetIndex) ? Presets[SelectedPresetIndex].Get() : nullptr;
		}

		FText GetSourceSummaryText() const
		{
			return FText::Format(
				LOCTEXT("SourceSummary", "Source JSON: {0}    Export: Content/Audio/SFX/FMTemp"),
				FText::FromString(GetSettingsPath()));
		}

		FText GetPresetComboText() const
		{
			const FPreset* Preset = GetSelectedPreset();
			return Preset ? FText::FromString(Preset->Id) : LOCTEXT("NoPreset", "No preset");
		}

		TSharedRef<SWidget> MakePresetRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PresetLabel", "Preset"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(PresetComboBox, SComboBox<TSharedPtr<FPreset>>)
					.OptionsSource(&Presets)
					.OnGenerateWidget(this, &STunaSweeperFMSoundComposer::GeneratePresetWidget)
					.OnSelectionChanged(this, &STunaSweeperFMSoundComposer::HandlePresetSelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &STunaSweeperFMSoundComposer::GetPresetComboText)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("DuplicatePreset", "Duplicate"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleDuplicatePreset)
				];
		}

		TSharedRef<SWidget> GeneratePresetWidget(TSharedPtr<FPreset> Preset) const
		{
			return SNew(STextBlock).Text(Preset.IsValid() ? FText::FromString(Preset->Id) : LOCTEXT("InvalidPreset", "Invalid"));
		}

		void HandlePresetSelectionChanged(TSharedPtr<FPreset> Preset, ESelectInfo::Type)
		{
			const int32 NewIndex = Presets.IndexOfByKey(Preset);
			if (NewIndex != INDEX_NONE)
			{
				SelectedPresetIndex = NewIndex;
			}
		}

		TSharedRef<SWidget> MakeIdRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("IdLabel", "Asset Id"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]()
					{
						const FPreset* Preset = GetSelectedPreset();
						return Preset ? FText::FromString(Preset->Id) : FText::GetEmpty();
					})
					.OnTextCommitted(this, &STunaSweeperFMSoundComposer::HandleIdCommitted)
				];
		}

		void HandleIdCommitted(const FText& NewText, ETextCommit::Type)
		{
			if (FPreset* Preset = GetSelectedPreset())
			{
				Preset->Id = SanitizeAssetName(NewText.ToString());
				MarkDirty();
				if (PresetComboBox.IsValid())
				{
					PresetComboBox->RefreshOptions();
				}
			}
		}

		TSharedRef<SWidget> MakeKindRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("KindLabel", "Purpose"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SAssignNew(KindComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&KindOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
					})
					.OnSelectionChanged(this, &STunaSweeperFMSoundComposer::HandleKindSelectionChanged)
					[
						SNew(STextBlock)
						.Text(this, &STunaSweeperFMSoundComposer::GetSelectedKindText)
					]
				];
		}

		FText GetSelectedKindText() const
		{
			const FPreset* Preset = GetSelectedPreset();
			return Preset ? FText::FromString(KindToString(Preset->Kind)) : FText::GetEmpty();
		}

		void HandleKindSelectionChanged(TSharedPtr<FString> Item, ESelectInfo::Type)
		{
			if (FPreset* Preset = GetSelectedPreset(); Preset && Item.IsValid())
			{
				Preset->Kind = KindFromString(*Item);
				MarkDirty();
			}
		}

		TSharedRef<SWidget> MakeFloatControl(FText Label, float Min, float Max, float Delta, float FPreset::* Member)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock)
					.Text(Label)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.66f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SSpinBox<float>)
					.MinValue(Min)
					.MaxValue(Max)
					.MinSliderValue(Min)
					.MaxSliderValue(Max)
					.Delta(Delta)
					.Value_Lambda([this, Member]()
					{
						const FPreset* Preset = GetSelectedPreset();
						return Preset ? Preset->*Member : 0.0f;
					})
					.OnValueChanged_Lambda([this, Member, Min, Max](float NewValue)
					{
						if (FPreset* Preset = GetSelectedPreset())
						{
							Preset->*Member = FMath::Clamp(NewValue, Min, Max);
							MarkDirty();
						}
					})
				];
		}

		TSharedRef<SWidget> MakeSeedRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.34f)
				.VAlign(VAlign_Center)
				.Padding(0.0f, 3.0f, 12.0f, 3.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SeedLabel", "Variant Seed"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.42f)
				.Padding(0.0f, 3.0f)
				[
					SNew(SSpinBox<int32>)
					.MinValue(1)
					.MaxValue(INT32_MAX)
					.Value_Lambda([this]()
					{
						const FPreset* Preset = GetSelectedPreset();
						return Preset ? Preset->Seed : 1;
					})
					.OnValueChanged_Lambda([this](int32 NewValue)
					{
						if (FPreset* Preset = GetSelectedPreset())
						{
							Preset->Seed = FMath::Max(1, NewValue);
							MarkDirty();
						}
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.24f)
				.Padding(8.0f, 3.0f, 0.0f, 3.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("MutateSeed", "Mutate"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleMutateSeed)
				];
		}

		TSharedRef<SWidget> MakeActionRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Preview", "Preview"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandlePreview)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveJson", "Save JSON"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleSaveJson)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ReloadJson", "Reload JSON"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleReloadJson)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportWav", "Export WAV"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleExportWav)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportSoundWave", "Export + Import SoundWave"))
					.OnClicked(this, &STunaSweeperFMSoundComposer::HandleExportAndImport)
				];
		}

		FReply HandleDuplicatePreset()
		{
			if (const FPreset* Preset = GetSelectedPreset())
			{
				TSharedPtr<FPreset> Copy = MakeShared<FPreset>(*Preset);
				Copy->Id = SanitizeAssetName(Preset->Id + TEXT("_Variant"));
				Copy->Seed = FMath::Max(1, Preset->Seed + 1);
				Presets.Add(Copy);
				SelectedPresetIndex = Presets.Num() - 1;
				MarkDirty();
				if (PresetComboBox.IsValid())
				{
					PresetComboBox->RefreshOptions();
					PresetComboBox->SetSelectedItem(Copy);
				}
			}
			return FReply::Handled();
		}

		FReply HandleMutateSeed()
		{
			if (FPreset* Preset = GetSelectedPreset())
			{
				Preset->Seed = FMath::Max(1, Preset->Seed + 97);
				MarkDirty();
				SetStatus(LOCTEXT("SeedMutated", "Variant seed changed."));
			}
			return FReply::Handled();
		}

		FReply HandlePreview()
		{
			const FPreset* Preset = GetSelectedPreset();
			if (!Preset)
			{
				return FReply::Handled();
			}

			FRenderResult RenderResult;
			if (!RenderPreset(*Preset, SampleRate, RenderResult))
			{
				SetStatus(LOCTEXT("PreviewFailed", "Preview render failed."));
				return FReply::Handled();
			}

			TArray<uint8> PcmBytes;
			PcmBytes.SetNumUninitialized(RenderResult.Samples.Num() * sizeof(int16));
			FMemory::Memcpy(PcmBytes.GetData(), RenderResult.Samples.GetData(), PcmBytes.Num());

			USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>();
			SoundWave->SetSampleRate(RenderResult.SampleRate);
			SoundWave->NumChannels = ChannelCount;
			SoundWave->Duration = RenderResult.DurationSeconds;
			SoundWave->bLooping = false;
			SoundWave->SoundGroup = SOUNDGROUP_Effects;
			SoundWave->QueueAudio(PcmBytes.GetData(), PcmBytes.Num());
			PreviewSound.Reset(SoundWave);

			if (GEditor)
			{
				GEditor->PlayPreviewSound(SoundWave);
				SetStatus(FText::Format(LOCTEXT("PreviewRendered", "Previewing {0}."), FText::FromString(Preset->Id)));
			}
			return FReply::Handled();
		}

		FReply HandleSaveJson()
		{
			SavePresets();
			return FReply::Handled();
		}

		FReply HandleReloadJson()
		{
			LoadPresets();
			if (PresetComboBox.IsValid())
			{
				PresetComboBox->RefreshOptions();
			}
			SetStatus(LOCTEXT("Reloaded", "JSON reloaded."));
			return FReply::Handled();
		}

		FReply HandleExportWav()
		{
			ExportSelectedPreset(false);
			return FReply::Handled();
		}

		FReply HandleExportAndImport()
		{
			ExportSelectedPreset(true);
			return FReply::Handled();
		}

		void MarkDirty()
		{
			bDirty = true;
		}

		void SetStatus(const FText& NewStatus)
		{
			Status = NewStatus;
			if (StatusText.IsValid())
			{
				StatusText->SetText(Status);
			}
		}

		void LoadPresets()
		{
			Presets.Reset();
			SampleRate = DefaultSampleRate;

			FString JsonText;
			if (FFileHelper::LoadFileToString(JsonText, *GetSettingsPath()))
			{
				TSharedPtr<FJsonObject> Root;
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
				if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
				{
					SampleRate = FMath::Clamp(ReadIntField(Root, TEXT("sample_rate"), DefaultSampleRate), 8000, 192000);
					const TArray<TSharedPtr<FJsonValue>>* PresetValues = nullptr;
					if (Root->TryGetArrayField(TEXT("presets"), PresetValues))
					{
						for (const TSharedPtr<FJsonValue>& Value : *PresetValues)
						{
							const TSharedPtr<FJsonObject> PresetObject = Value.IsValid() ? Value->AsObject() : nullptr;
							if (PresetObject.IsValid())
							{
								Presets.Add(MakeShared<FPreset>(ReadPreset(PresetObject)));
							}
						}
					}
				}
			}

			if (Presets.Num() == 0)
			{
				for (const FPreset& Preset : MakeDefaultPresets())
				{
					Presets.Add(MakeShared<FPreset>(Preset));
				}
				SavePresets();
				SetStatus(LOCTEXT("DefaultCreated", "Default FM SFX presets created."));
			}

			SelectedPresetIndex = FMath::Clamp(SelectedPresetIndex, 0, FMath::Max(0, Presets.Num() - 1));
			bDirty = false;
		}

		void SavePresets()
		{
			TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
			Root->SetNumberField(TEXT("sample_rate"), SampleRate);
			Root->SetStringField(TEXT("export_directory"), FString::Printf(TEXT("Content/%s"), *ExportRelativeDirectory));
			Root->SetStringField(TEXT("export_asset_path"), ExportAssetPath);

			TArray<TSharedPtr<FJsonValue>> PresetValues;
			for (const TSharedPtr<FPreset>& Preset : Presets)
			{
				if (Preset.IsValid())
				{
					Preset->Id = SanitizeAssetName(Preset->Id);
					PresetValues.Add(MakeShared<FJsonValueObject>(WritePreset(*Preset)));
				}
			}
			Root->SetArrayField(TEXT("presets"), PresetValues);

			FString JsonText;
			const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
			FJsonSerializer::Serialize(Root, Writer);

			IFileManager::Get().MakeDirectory(*FPaths::GetPath(GetSettingsPath()), true);
			if (FFileHelper::SaveStringToFile(JsonText, *GetSettingsPath()))
			{
				bDirty = false;
				SetStatus(FText::Format(LOCTEXT("JsonSaved", "Saved JSON: {0}"), FText::FromString(GetSettingsPath())));
			}
			else
			{
				SetStatus(FText::Format(LOCTEXT("JsonSaveFailed", "Failed to save JSON: {0}"), FText::FromString(GetSettingsPath())));
			}
		}

		bool ExportSelectedPreset(bool bImportSoundWave)
		{
			FPreset* Preset = GetSelectedPreset();
			if (!Preset)
			{
				SetStatus(LOCTEXT("NoSelectedPreset", "No preset selected."));
				return false;
			}

			Preset->Id = SanitizeAssetName(Preset->Id);
			if (bDirty)
			{
				SavePresets();
			}

			FRenderResult RenderResult;
			if (!RenderPreset(*Preset, SampleRate, RenderResult))
			{
				SetStatus(LOCTEXT("ExportRenderFailed", "Render failed."));
				return false;
			}

			const FString WavPath = FPaths::Combine(GetExportDirectory(), Preset->Id + TEXT(".wav"));
			if (!WriteWavFile(WavPath, RenderResult))
			{
				SetStatus(FText::Format(LOCTEXT("WavWriteFailed", "Failed to write WAV: {0}"), FText::FromString(WavPath)));
				return false;
			}

			if (!bImportSoundWave)
			{
				SetStatus(FText::Format(LOCTEXT("WavExported", "Exported WAV: {0}"), FText::FromString(WavPath)));
				return true;
			}

			FString ObjectPath;
			if (!ImportWavAsSoundWave(WavPath, Preset->Id, ObjectPath))
			{
				SetStatus(FText::Format(LOCTEXT("ImportFailed", "WAV exported, but SoundWave import failed: {0}"), FText::FromString(WavPath)));
				return false;
			}

			SetStatus(FText::Format(LOCTEXT("Imported", "Exported and imported SoundWave: {0}"), FText::FromString(ObjectPath)));
			return true;
		}
	};
}

void FTunaSweeperFMSoundTool::Startup()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TunaSweeperFMSound::TabName,
		FOnSpawnTab::CreateRaw(this, &FTunaSweeperFMSoundTool::SpawnToolTab))
		.SetDisplayName(LOCTEXT("TabTitle", "FM Sound Composer"))
		.SetTooltipText(LOCTEXT("TabTooltip", "Create temporary FM-style sound effects and export them as WAV/SoundWave assets."))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTunaSweeperFMSoundTool::RegisterMenus));
}

void FTunaSweeperFMSoundTool::Shutdown()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TunaSweeperFMSound::TabName);
}

void FTunaSweeperFMSoundTool::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
	FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
	if (!MainSection.FindEntry(TEXT("TunaSweeper")))
	{
		FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
			TEXT("TunaSweeper"),
			LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
			LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
			FNewToolMenuChoice());
		TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
	}

	UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
		TEXT("LevelEditor.MainMenu.TunaSweeper"),
		NAME_None,
		EMultiBoxType::Menu,
		false);
	FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
		TEXT("Audio"),
		LOCTEXT("TunaSweeperAudioMenuSection", "Audio"));
	Section.AddMenuEntry(
		TEXT("OpenTunaSweeperFMSoundComposer"),
		LOCTEXT("MenuEntry", "FM Sound Composer"),
		LOCTEXT("MenuEntryTooltip", "Open the TunaSweeper temporary FM sound effect composer."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FTunaSweeperFMSoundTool::OpenToolWindow)));
}

void FTunaSweeperFMSoundTool::OpenToolWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TunaSweeperFMSound::TabName);
}

TSharedRef<SDockTab> FTunaSweeperFMSoundTool::SpawnToolTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(TunaSweeperFMSound::STunaSweeperFMSoundComposer)
		];
}

#undef LOCTEXT_NAMESPACE
