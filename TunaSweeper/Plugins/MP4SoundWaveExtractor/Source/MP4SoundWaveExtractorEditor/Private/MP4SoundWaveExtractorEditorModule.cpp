#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Async/Async.h"
#include "Components/AudioComponent.h"
#include "DesktopPlatformModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Factories/SoundFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "IDesktopPlatform.h"
#include "Input/Events.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Rendering/DrawElements.h"
#include "Sound/SoundWave.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "ToolMenus.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"

THIRD_PARTY_INCLUDES_START
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
THIRD_PARTY_INCLUDES_END

#include "Microsoft/COMPointer.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "MP4SoundWaveExtractor"

DEFINE_LOG_CATEGORY_STATIC(LogMP4SoundWaveExtractor, Log, All);

namespace MP4SoundWaveExtractor
{
	const FName TabName(TEXT("MP4SoundWaveExtractor"));
	const FString DefaultDestinationPath(TEXT("/Game/Audio/Imported"));
	constexpr int64 HundredNanosecondsPerSecond = 10000000LL;

	FString GetHResultDescription(const HRESULT Result)
	{
		return FString::Printf(TEXT("HRESULT 0x%08X"), static_cast<uint32>(Result));
	}

	FString SanitizeAssetName(FString Name)
	{
		Name.TrimStartAndEndInline();
		if (Name.IsEmpty())
		{
			Name = TEXT("SW_ExtractedAudio");
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
			Name = TEXT("SW_") + Name;
		}

		return Name;
	}

	void AppendUInt16LE(TArray<uint8>& Bytes, const uint16 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
	}

	void AppendUInt32LE(TArray<uint8>& Bytes, const uint32 Value)
	{
		Bytes.Add(static_cast<uint8>(Value & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
		Bytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
	}

	bool WritePcm16Wav(
		const FString& WavPath,
		const TArray<uint8>& PcmData,
		const uint32 SampleRate,
		const uint16 NumChannels,
		FString& OutError)
	{
		OutError.Reset();
		if (SampleRate == 0 || NumChannels == 0 || PcmData.IsEmpty())
		{
			OutError = TEXT("No decoded PCM audio was available to write.");
			return false;
		}

		if (PcmData.Num() > static_cast<int64>(MAX_uint32) - 44)
		{
			OutError = TEXT("The selected audio range is too large for a WAV file.");
			return false;
		}

		const uint32 DataSize = static_cast<uint32>(PcmData.Num());
		const uint16 BitsPerSample = 16;
		const uint16 BlockAlign = NumChannels * (BitsPerSample / 8);
		const uint32 ByteRate = SampleRate * BlockAlign;

		TArray<uint8> WavBytes;
		WavBytes.Reserve(44 + PcmData.Num());
		WavBytes.Append(reinterpret_cast<const uint8*>("RIFF"), 4);
		AppendUInt32LE(WavBytes, 36 + DataSize);
		WavBytes.Append(reinterpret_cast<const uint8*>("WAVE"), 4);
		WavBytes.Append(reinterpret_cast<const uint8*>("fmt "), 4);
		AppendUInt32LE(WavBytes, 16);
		AppendUInt16LE(WavBytes, 1);
		AppendUInt16LE(WavBytes, NumChannels);
		AppendUInt32LE(WavBytes, SampleRate);
		AppendUInt32LE(WavBytes, ByteRate);
		AppendUInt16LE(WavBytes, BlockAlign);
		AppendUInt16LE(WavBytes, BitsPerSample);
		WavBytes.Append(reinterpret_cast<const uint8*>("data"), 4);
		AppendUInt32LE(WavBytes, DataSize);
		WavBytes.Append(PcmData);

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(WavPath), true);
		if (!FFileHelper::SaveArrayToFile(WavBytes, *WavPath))
		{
			OutError = FString::Printf(TEXT("Unable to write WAV: %s"), *WavPath);
			return false;
		}

		return true;
	}

#if PLATFORM_WINDOWS
	class FMediaFoundationScope final
	{
	public:
		bool Initialize(FString& OutError)
		{
			OutError.Reset();
			CoInitializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			if (FAILED(CoInitializeResult) && CoInitializeResult != RPC_E_CHANGED_MODE)
			{
				OutError = FString::Printf(TEXT("Unable to initialize COM (%s)."), *GetHResultDescription(CoInitializeResult));
				return false;
			}

			const HRESULT StartupResult = MFStartup(MF_VERSION);
			if (FAILED(StartupResult))
			{
				OutError = FString::Printf(TEXT("Unable to initialize Windows Media Foundation (%s)."), *GetHResultDescription(StartupResult));
				return false;
			}

			bMediaFoundationStarted = true;
			return true;
		}

		~FMediaFoundationScope()
		{
			if (bMediaFoundationStarted)
			{
				MFShutdown();
			}

			if (SUCCEEDED(CoInitializeResult))
			{
				CoUninitialize();
			}
		}

	private:
		HRESULT CoInitializeResult = E_FAIL;
		bool bMediaFoundationStarted = false;
	};

	bool CreatePcmReader(
		const FString& SourcePath,
		TComPtr<IMFSourceReader>& OutReader,
		uint32& OutSampleRate,
		uint16& OutNumChannels,
		FString& OutError)
	{
		OutReader.Reset();
		OutSampleRate = 0;
		OutNumChannels = 0;
		OutError.Reset();

		HRESULT Result = MFCreateSourceReaderFromURL(*SourcePath, nullptr, &OutReader);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Unable to open the MP4 file (%s)."), *GetHResultDescription(Result));
			return false;
		}

		Result = OutReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, false);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Unable to select an audio stream (%s)."), *GetHResultDescription(Result));
			return false;
		}

		Result = OutReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, true);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("The MP4 file does not expose a readable audio stream (%s)."), *GetHResultDescription(Result));
			return false;
		}

		TComPtr<IMFMediaType> NativeType;
		Result = OutReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &NativeType);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Unable to read the MP4 audio format (%s)."), *GetHResultDescription(Result));
			return false;
		}

		GUID MajorType = GUID_NULL;
		if (FAILED(NativeType->GetGUID(MF_MT_MAJOR_TYPE, &MajorType)) || MajorType != MFMediaType_Audio)
		{
			OutError = TEXT("The selected file does not contain a supported audio stream.");
			return false;
		}

		const uint32 NativeChannels = MFGetAttributeUINT32(NativeType, MF_MT_AUDIO_NUM_CHANNELS, 0);
		const uint32 NativeSampleRate = MFGetAttributeUINT32(NativeType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
		if (NativeChannels == 0 || NativeChannels > 2 || NativeSampleRate == 0)
		{
			OutError = TEXT("Only mono or stereo MP4 audio is supported.");
			return false;
		}

		TComPtr<IMFMediaType> PcmType;
		Result = MFCreateMediaType(&PcmType);
		if (FAILED(Result) ||
			FAILED(PcmType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
			FAILED(PcmType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
			FAILED(PcmType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16)) ||
			FAILED(PcmType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, NativeChannels)) ||
			FAILED(PcmType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, NativeSampleRate)))
		{
			OutError = TEXT("Unable to configure 16-bit PCM audio decoding.");
			return false;
		}

		Result = OutReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, PcmType);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Windows Media Foundation cannot decode this MP4 audio stream (%s)."), *GetHResultDescription(Result));
			return false;
		}

		TComPtr<IMFMediaType> CurrentType;
		Result = OutReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &CurrentType);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Unable to verify decoded audio format (%s)."), *GetHResultDescription(Result));
			return false;
		}

		const uint32 BitsPerSample = MFGetAttributeUINT32(CurrentType, MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
		OutSampleRate = MFGetAttributeUINT32(CurrentType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
		OutNumChannels = static_cast<uint16>(MFGetAttributeUINT32(CurrentType, MF_MT_AUDIO_NUM_CHANNELS, 0));
		if (BitsPerSample != 16 || OutSampleRate == 0 || OutNumChannels == 0 || OutNumChannels > 2)
		{
			OutError = TEXT("The MP4 decoder did not provide compatible 16-bit mono or stereo PCM audio.");
			return false;
		}

		return true;
	}

	bool TryReadDurationSeconds(const FString& SourcePath, double& OutDurationSeconds, FString& OutError)
	{
		OutDurationSeconds = 0.0;
		FMediaFoundationScope MediaFoundation;
		if (!MediaFoundation.Initialize(OutError))
		{
			return false;
		}

		TComPtr<IMFSourceReader> Reader;
		HRESULT Result = MFCreateSourceReaderFromURL(*SourcePath, nullptr, &Reader);
		if (FAILED(Result))
		{
			OutError = FString::Printf(TEXT("Unable to open the MP4 file (%s)."), *GetHResultDescription(Result));
			return false;
		}

		PROPVARIANT DurationValue;
		PropVariantInit(&DurationValue);
		Result = Reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &DurationValue);
		if (SUCCEEDED(Result) && (DurationValue.vt == VT_UI8 || DurationValue.vt == VT_I8))
		{
			const int64 Duration = DurationValue.vt == VT_UI8
				? static_cast<int64>(DurationValue.uhVal.QuadPart)
				: DurationValue.hVal.QuadPart;
			OutDurationSeconds = static_cast<double>(Duration) / static_cast<double>(HundredNanosecondsPerSecond);
		}
		PropVariantClear(&DurationValue);

		if (OutDurationSeconds <= 0.0)
		{
			OutError = TEXT("The MP4 duration could not be read.");
			return false;
		}

		return true;
	}

	bool ExtractAudioRangeToWav(
		const FString& SourcePath,
		const double StartSeconds,
		const double EndSeconds,
		const FString& WavPath,
		FString& OutError)
	{
		OutError.Reset();
		if (!FPaths::FileExists(SourcePath))
		{
			OutError = TEXT("Select an existing MP4 file first.");
			return false;
		}

		if (!FPaths::GetExtension(SourcePath).Equals(TEXT("mp4"), ESearchCase::IgnoreCase))
		{
			OutError = TEXT("Only .mp4 files are supported.");
			return false;
		}

		if (StartSeconds < 0.0 || EndSeconds <= StartSeconds)
		{
			OutError = TEXT("End Time must be greater than Start Time.");
			return false;
		}

		FMediaFoundationScope MediaFoundation;
		if (!MediaFoundation.Initialize(OutError))
		{
			return false;
		}

		TComPtr<IMFSourceReader> Reader;
		uint32 SampleRate = 0;
		uint16 NumChannels = 0;
		if (!CreatePcmReader(SourcePath, Reader, SampleRate, NumChannels, OutError))
		{
			return false;
		}

		const int64 StartTime = FMath::RoundToInt64(StartSeconds * static_cast<double>(HundredNanosecondsPerSecond));
		const int64 EndTime = FMath::RoundToInt64(EndSeconds * static_cast<double>(HundredNanosecondsPerSecond));

		PROPVARIANT StartPosition;
		PropVariantInit(&StartPosition);
		StartPosition.vt = VT_I8;
		StartPosition.hVal.QuadPart = StartTime;
		const HRESULT SeekResult = Reader->SetCurrentPosition(GUID_NULL, StartPosition);
		PropVariantClear(&StartPosition);
		if (FAILED(SeekResult))
		{
			OutError = FString::Printf(TEXT("Unable to seek to the requested start time (%s)."), *GetHResultDescription(SeekResult));
			return false;
		}

		const uint32 FrameSizeBytes = static_cast<uint32>(NumChannels) * sizeof(int16);
		TArray<uint8> PcmData;

		for (;;)
		{
			DWORD StreamFlags = 0;
			DWORD ActualStreamIndex = 0;
			LONGLONG SampleTimestamp = 0;
			TComPtr<IMFSample> Sample;
			const HRESULT ReadResult = Reader->ReadSample(
				MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				0,
				&ActualStreamIndex,
				&StreamFlags,
				&SampleTimestamp,
				&Sample);
			if (FAILED(ReadResult) || (StreamFlags & MF_SOURCE_READERF_ERROR) != 0)
			{
				OutError = FString::Printf(TEXT("Audio decoding failed (%s)."), *GetHResultDescription(ReadResult));
				return false;
			}

			if ((StreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
			{
				OutError = TEXT("The MP4 audio format changed during decoding and cannot be exported safely.");
				return false;
			}

			if (Sample)
			{
				TComPtr<IMFMediaBuffer> Buffer;
				const HRESULT BufferResult = Sample->ConvertToContiguousBuffer(&Buffer);
				if (FAILED(BufferResult))
				{
					OutError = FString::Printf(TEXT("Unable to access decoded audio data (%s)."), *GetHResultDescription(BufferResult));
					return false;
				}

				BYTE* BufferData = nullptr;
				DWORD MaxLength = 0;
				DWORD CurrentLength = 0;
				const HRESULT LockResult = Buffer->Lock(&BufferData, &MaxLength, &CurrentLength);
				if (FAILED(LockResult))
				{
					OutError = FString::Printf(TEXT("Unable to read decoded audio data (%s)."), *GetHResultDescription(LockResult));
					return false;
				}

				const bool bValidAlignment = (CurrentLength % FrameSizeBytes) == 0;
				const int64 FrameCount = bValidAlignment ? static_cast<int64>(CurrentLength / FrameSizeBytes) : 0;
				if (!bValidAlignment)
				{
					Buffer->Unlock();
					OutError = TEXT("Decoded audio data is not aligned to whole PCM frames.");
					return false;
				}

				int64 FirstFrame = 0;
				if (SampleTimestamp < StartTime)
				{
					FirstFrame = FMath::Clamp<int64>(
						FMath::CeilToInt64(
							static_cast<double>(StartTime - SampleTimestamp) * static_cast<double>(SampleRate) /
							static_cast<double>(HundredNanosecondsPerSecond)),
						0,
						FrameCount);
				}

				int64 LastFrameExclusive = FrameCount;
				if (SampleTimestamp < EndTime)
				{
					LastFrameExclusive = FMath::Clamp<int64>(
						FMath::CeilToInt64(
							static_cast<double>(EndTime - SampleTimestamp) * static_cast<double>(SampleRate) /
							static_cast<double>(HundredNanosecondsPerSecond)),
						0,
						FrameCount);
				}
				else
				{
					LastFrameExclusive = 0;
				}

				if (LastFrameExclusive > FirstFrame)
				{
					const int64 BytesToAppend = (LastFrameExclusive - FirstFrame) * static_cast<int64>(FrameSizeBytes);
					if (PcmData.Num() > static_cast<int64>(MAX_uint32) - 44 - BytesToAppend)
					{
						Buffer->Unlock();
						OutError = TEXT("The selected audio range is too large for a WAV file.");
						return false;
					}

					PcmData.Append(BufferData + FirstFrame * FrameSizeBytes, static_cast<int32>(BytesToAppend));
				}

				Buffer->Unlock();
			}

			if ((StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0 || SampleTimestamp >= EndTime)
			{
				break;
			}
		}

		return WritePcm16Wav(WavPath, PcmData, SampleRate, NumChannels, OutError);
	}

	bool BuildWaveformPeaks(
		const FString& SourcePath,
		const double DurationSeconds,
		const int32 PeakCount,
		TArray<float>& OutPeaks,
		FString& OutError)
	{
		OutPeaks.Reset();
		OutError.Reset();
		if (DurationSeconds <= 0.0 || PeakCount <= 0)
		{
			OutError = TEXT("The MP4 duration is unavailable for waveform analysis.");
			return false;
		}

		FMediaFoundationScope MediaFoundation;
		if (!MediaFoundation.Initialize(OutError))
		{
			return false;
		}

		TComPtr<IMFSourceReader> Reader;
		uint32 SampleRate = 0;
		uint16 NumChannels = 0;
		if (!CreatePcmReader(SourcePath, Reader, SampleRate, NumChannels, OutError))
		{
			return false;
		}

		const int64 DurationTicks = FMath::Max(
			1LL,
			FMath::RoundToInt64(DurationSeconds * static_cast<double>(HundredNanosecondsPerSecond)));
		const uint32 FrameSizeBytes = static_cast<uint32>(NumChannels) * sizeof(int16);
		const int64 AnalysisStrideFrames = FMath::Max<int64>(1, static_cast<int64>(SampleRate) / 240);
		OutPeaks.SetNumZeroed(PeakCount);

		for (;;)
		{
			DWORD StreamFlags = 0;
			DWORD ActualStreamIndex = 0;
			LONGLONG SampleTimestamp = 0;
			TComPtr<IMFSample> Sample;
			const HRESULT ReadResult = Reader->ReadSample(
				MF_SOURCE_READER_FIRST_AUDIO_STREAM,
				0,
				&ActualStreamIndex,
				&StreamFlags,
				&SampleTimestamp,
				&Sample);
			if (FAILED(ReadResult) || (StreamFlags & MF_SOURCE_READERF_ERROR) != 0)
			{
				OutError = FString::Printf(TEXT("Waveform decoding failed (%s)."), *GetHResultDescription(ReadResult));
				return false;
			}

			if ((StreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
			{
				OutError = TEXT("The MP4 audio format changed during waveform analysis.");
				return false;
			}

			if (Sample)
			{
				TComPtr<IMFMediaBuffer> Buffer;
				const HRESULT BufferResult = Sample->ConvertToContiguousBuffer(&Buffer);
				if (FAILED(BufferResult))
				{
					OutError = FString::Printf(TEXT("Unable to access waveform audio data (%s)."), *GetHResultDescription(BufferResult));
					return false;
				}

				BYTE* BufferData = nullptr;
				DWORD MaxLength = 0;
				DWORD CurrentLength = 0;
				const HRESULT LockResult = Buffer->Lock(&BufferData, &MaxLength, &CurrentLength);
				if (FAILED(LockResult))
				{
					OutError = FString::Printf(TEXT("Unable to read waveform audio data (%s)."), *GetHResultDescription(LockResult));
					return false;
				}

				if ((CurrentLength % FrameSizeBytes) != 0)
				{
					Buffer->Unlock();
					OutError = TEXT("Waveform audio data is not aligned to whole PCM frames.");
					return false;
				}

				const int64 FrameCount = static_cast<int64>(CurrentLength / FrameSizeBytes);
				for (int64 FrameIndex = 0; FrameIndex < FrameCount; FrameIndex += AnalysisStrideFrames)
				{
					const int16* FrameSamples = reinterpret_cast<const int16*>(
						BufferData + FrameIndex * static_cast<int64>(FrameSizeBytes));
					float FramePeak = 0.0f;
					for (uint16 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
					{
						FramePeak = FMath::Max(
							FramePeak,
							FMath::Abs(static_cast<float>(FrameSamples[ChannelIndex])) / 32768.0f);
					}

					const int64 FrameTimestamp = SampleTimestamp + FMath::RoundToInt64(
						static_cast<double>(FrameIndex) * static_cast<double>(HundredNanosecondsPerSecond) /
						static_cast<double>(SampleRate));
					const int32 PeakIndex = FMath::Clamp(
						static_cast<int32>(
							static_cast<double>(FrameTimestamp) * static_cast<double>(PeakCount) /
							static_cast<double>(DurationTicks)),
						0,
						PeakCount - 1);
					OutPeaks[PeakIndex] = FMath::Max(OutPeaks[PeakIndex], FramePeak);
				}

				Buffer->Unlock();
			}

			if ((StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
			{
				break;
			}
		}

		for (float& Peak : OutPeaks)
		{
			Peak = FMath::Sqrt(FMath::Clamp(Peak, 0.0f, 1.0f));
		}
		return true;
	}
#else
	bool TryReadDurationSeconds(const FString&, double&, FString& OutError)
	{
		OutError = TEXT("MP4 Sound Wave Extractor is only available on Windows.");
		return false;
	}

	bool ExtractAudioRangeToWav(const FString&, double, double, const FString&, FString& OutError)
	{
		OutError = TEXT("MP4 Sound Wave Extractor is only available on Windows.");
		return false;
	}

	bool BuildWaveformPeaks(const FString&, double, int32, TArray<float>&, FString& OutError)
	{
		OutError = TEXT("MP4 Sound Wave Extractor is only available on Windows.");
		return false;
	}
#endif

	bool SaveImportedAsset(UObject* Asset)
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
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFilename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
	}

	bool ImportWavAsSoundWave(
		const FString& WavPath,
		const FString& DestinationPath,
		USoundWave*& OutSoundWave,
		FString& OutError)
	{
		OutSoundWave = nullptr;
		OutError.Reset();
		if (!FPaths::FileExists(WavPath))
		{
			OutError = TEXT("The temporary WAV file was not created.");
			return false;
		}

		FModuleManager::Get().LoadModuleChecked(TEXT("AudioEditor"));
		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(WavPath);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		for (UObject* ImportedAsset : ImportedAssets)
		{
			if (USoundWave* SoundWave = Cast<USoundWave>(ImportedAsset))
			{
				SoundWave->Modify();
				SoundWave->bLooping = false;
				SoundWave->PostEditChange();
				SoundWave->MarkPackageDirty();
				SaveImportedAsset(SoundWave);
				OutSoundWave = SoundWave;
				return true;
			}
		}

		OutError = TEXT("WAV extraction succeeded, but Unreal could not import a Sound Wave asset.");
		return false;
	}

	DECLARE_DELEGATE_TwoParams(FOnMP4RangeChanged, float, float);

	FPaintGeometry MakeLocalBoxGeometry(
		const FGeometry& AllottedGeometry,
		const FVector2D& Position,
		const FVector2D& Size)
	{
		return AllottedGeometry.ToPaintGeometry(
			FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y)),
			FSlateLayoutTransform(FVector2f(static_cast<float>(Position.X), static_cast<float>(Position.Y))));
	}

	class SMP4AudioRangeSelector final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMP4AudioRangeSelector) {}
			SLATE_EVENT(FOnMP4RangeChanged, OnRangeChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnRangeChanged = InArgs._OnRangeChanged;
			SetToolTipText(LOCTEXT("RangeSelectorTooltip", "Drag the A and B handles to select the MP4 audio range."));
		}

		void SetDuration(const float InDurationSeconds)
		{
			DurationSeconds = FMath::Max(0.0f, InDurationSeconds);
			StartSeconds = FMath::Clamp(StartSeconds, 0.0f, DurationSeconds);
			EndSeconds = FMath::Clamp(EndSeconds, StartSeconds, DurationSeconds);
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetRange(const float InStartSeconds, const float InEndSeconds)
		{
			if (DurationSeconds <= 0.0f)
			{
				StartSeconds = 0.0f;
				EndSeconds = 0.0f;
			}
			else
			{
				const float MinimumRange = FMath::Min(0.01f, DurationSeconds);
				StartSeconds = FMath::Clamp(InStartSeconds, 0.0f, DurationSeconds - MinimumRange);
				EndSeconds = FMath::Clamp(InEndSeconds, StartSeconds + MinimumRange, DurationSeconds);
			}
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetWaveformPeaks(TArray<float>&& InWaveformPeaks)
		{
			WaveformPeaks = MoveTemp(InWaveformPeaks);
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(680.0f, 130.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override
		{
			const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
			if (!WhiteBrush)
			{
				return LayerId;
			}

			const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
			const float Width = FMath::Max(1.0f, static_cast<float>(LocalSize.X));
			const float Height = FMath::Max(1.0f, static_cast<float>(LocalSize.Y));
			const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled)
				? ESlateDrawEffect::None
				: ESlateDrawEffect::DisabledEffect;
			const FLinearColor WidgetTint = InWidgetStyle.GetColorAndOpacityTint();

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				WhiteBrush,
				DrawEffects,
				FLinearColor(0.015f, 0.02f, 0.03f, 1.0f) * WidgetTint);

			const float StartX = SecondsToX(StartSeconds, Width);
			const float EndX = SecondsToX(EndSeconds, Width);
			if (StartX > 0.0f)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					MakeLocalBoxGeometry(AllottedGeometry, FVector2D::ZeroVector, FVector2D(StartX, Height)),
					WhiteBrush,
					DrawEffects,
					FLinearColor(0.0f, 0.0f, 0.0f, 0.55f) * WidgetTint);
			}
			if (EndX < Width)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					MakeLocalBoxGeometry(AllottedGeometry, FVector2D(EndX, 0.0f), FVector2D(Width - EndX, Height)),
					WhiteBrush,
					DrawEffects,
					FLinearColor(0.0f, 0.0f, 0.0f, 0.55f) * WidgetTint);
			}
			if (EndX > StartX)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 2,
					MakeLocalBoxGeometry(AllottedGeometry, FVector2D(StartX, 0.0f), FVector2D(EndX - StartX, Height)),
					WhiteBrush,
					DrawEffects,
					FLinearColor(0.04f, 0.30f, 0.62f, 0.20f) * WidgetTint);
			}

			const float WaveTop = 24.0f;
			const float WaveBottom = FMath::Max(WaveTop + 2.0f, Height - 18.0f);
			const float WaveCenter = (WaveTop + WaveBottom) * 0.5f;
			const float WaveHalfHeight = (WaveBottom - WaveTop) * 0.5f;
			TArray<FVector2f> BaselinePoints;
			BaselinePoints.Add(FVector2f(0.0f, WaveCenter));
			BaselinePoints.Add(FVector2f(Width, WaveCenter));
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(BaselinePoints),
				DrawEffects,
				FLinearColor(0.18f, 0.26f, 0.36f, 0.9f) * WidgetTint,
				true,
				1.0f);

			for (int32 PeakIndex = 0; PeakIndex < WaveformPeaks.Num(); ++PeakIndex)
			{
				const float Peak = FMath::Clamp(WaveformPeaks[PeakIndex], 0.0f, 1.0f);
				const float X = WaveformPeaks.Num() > 1
					? (static_cast<float>(PeakIndex) / static_cast<float>(WaveformPeaks.Num() - 1)) * Width
					: Width * 0.5f;
				const float HalfAmplitude = FMath::Max(1.0f, Peak * WaveHalfHeight);
				TArray<FVector2f> WaveLine;
				WaveLine.Add(FVector2f(X, WaveCenter - HalfAmplitude));
				WaveLine.Add(FVector2f(X, WaveCenter + HalfAmplitude));
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + 4,
					AllottedGeometry.ToPaintGeometry(),
					MoveTemp(WaveLine),
					DrawEffects,
					FLinearColor(0.38f, 0.80f, 1.0f, 0.95f) * WidgetTint,
					true,
					1.0f);
			}

			DrawHandle(OutDrawElements, AllottedGeometry, LayerId + 5, DrawEffects, WidgetTint, StartX, true);
			DrawHandle(OutDrawElements, AllottedGeometry, LayerId + 5, DrawEffects, WidgetTint, EndX, false);

			const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
			const FString StartLabel = FString::Printf(TEXT("A  %.2fs"), StartSeconds);
			const FString EndLabel = FString::Printf(TEXT("B  %.2fs"), EndSeconds);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 6,
				MakeLocalBoxGeometry(AllottedGeometry, FVector2D(FMath::Min(StartX + 5.0f, Width - 75.0f), 3.0f), FVector2D(75.0f, 14.0f)),
				StartLabel,
				LabelFont,
				DrawEffects,
				FLinearColor(1.0f, 0.70f, 0.25f, 1.0f) * WidgetTint);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId + 6,
				MakeLocalBoxGeometry(AllottedGeometry, FVector2D(FMath::Max(0.0f, EndX - 76.0f), Height - 15.0f), FVector2D(75.0f, 14.0f)),
				EndLabel,
				LabelFont,
				DrawEffects,
				FLinearColor(0.32f, 0.87f, 1.0f, 1.0f) * WidgetTint);

			return LayerId + 6;
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || DurationSeconds <= 0.0f)
			{
				return FReply::Unhandled();
			}

			const float PointerSeconds = GetPointerSeconds(MyGeometry, MouseEvent);
			bDraggingStartHandle = FMath::Abs(PointerSeconds - StartSeconds) <= FMath::Abs(PointerSeconds - EndSeconds);
			SetHandleFromPointer(PointerSeconds);
			return FReply::Handled().CaptureMouse(AsShared());
		}

		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (!HasMouseCapture())
			{
				return FReply::Unhandled();
			}

			SetHandleFromPointer(GetPointerSeconds(MyGeometry, MouseEvent));
			return FReply::Handled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HasMouseCapture())
			{
				return FReply::Handled().ReleaseMouseCapture();
			}

			return FReply::Unhandled();
		}

		virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override
		{
			if (DurationSeconds <= 0.0f)
			{
				return FCursorReply::Unhandled();
			}

			const float PointerSeconds = GetPointerSeconds(MyGeometry, CursorEvent);
			const float HandleTolerance = DurationSeconds * 0.025f;
			return FMath::Abs(PointerSeconds - StartSeconds) <= HandleTolerance ||
				FMath::Abs(PointerSeconds - EndSeconds) <= HandleTolerance
				? FCursorReply::Cursor(EMouseCursor::ResizeLeftRight)
				: FCursorReply::Cursor(EMouseCursor::Hand);
		}

	private:
		float SecondsToX(const float Seconds, const float Width) const
		{
			return DurationSeconds > 0.0f
				? FMath::Clamp(Seconds / DurationSeconds, 0.0f, 1.0f) * Width
				: 0.0f;
		}

		float GetPointerSeconds(const FGeometry& Geometry, const FPointerEvent& MouseEvent) const
		{
			const FVector2D LocalPosition = Geometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const float Width = FMath::Max(1.0f, static_cast<float>(Geometry.GetLocalSize().X));
			return FMath::Clamp(static_cast<float>(LocalPosition.X) / Width, 0.0f, 1.0f) * DurationSeconds;
		}

		void SetHandleFromPointer(const float PointerSeconds)
		{
			const float MinimumRange = FMath::Min(0.01f, DurationSeconds);
			if (bDraggingStartHandle)
			{
				StartSeconds = FMath::Clamp(PointerSeconds, 0.0f, EndSeconds - MinimumRange);
			}
			else
			{
				EndSeconds = FMath::Clamp(PointerSeconds, StartSeconds + MinimumRange, DurationSeconds);
			}

			Invalidate(EInvalidateWidgetReason::Paint);
			OnRangeChanged.ExecuteIfBound(StartSeconds, EndSeconds);
		}

		static void DrawHandle(
			FSlateWindowElementList& OutDrawElements,
			const FGeometry& AllottedGeometry,
			const int32 LayerId,
			const ESlateDrawEffect DrawEffects,
			const FLinearColor& WidgetTint,
			const float HandleX,
			const bool bIsStartHandle)
		{
			const FSlateBrush* WhiteBrush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
			if (!WhiteBrush)
			{
				return;
			}

			const float Height = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				MakeLocalBoxGeometry(AllottedGeometry, FVector2D(HandleX - 2.0f, 0.0f), FVector2D(4.0f, Height)),
				WhiteBrush,
				DrawEffects,
				(bIsStartHandle
					? FLinearColor(1.0f, 0.65f, 0.18f, 1.0f)
					: FLinearColor(0.25f, 0.84f, 1.0f, 1.0f)) * WidgetTint);
		}

		FOnMP4RangeChanged OnRangeChanged;
		TArray<float> WaveformPeaks;
		float DurationSeconds = 0.0f;
		float StartSeconds = 0.0f;
		float EndSeconds = 0.0f;
		bool bDraggingStartHandle = true;
	};

	class SMP4SoundWaveExtractor final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMP4SoundWaveExtractor) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
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
						.Text(LOCTEXT("Description", "Extract an MP4 audio range as a Sound Wave asset. Windows Media Foundation is used; FFmpeg is not used."))
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						MakeSourceRow()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SMP4SoundWaveExtractor::GetDurationText)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RangeLabel", "Selection Range — drag the A and B handles"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SAssignNew(RangeSelector, SMP4AudioRangeSelector)
						.OnRangeChanged(this, &SMP4SoundWaveExtractor::HandleRangeChanged)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SMP4SoundWaveExtractor::GetSelectionText)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(SSeparator)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						MakeTextRow(LOCTEXT("DestinationLabel", "Destination Folder"), DestinationPathTextBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						MakeTextRow(LOCTEXT("AssetNameLabel", "Sound Wave Name"), AssetNameTextBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("PreviewSelection", "Preview Selection"))
							.ToolTipText(LOCTEXT("PreviewSelectionTooltip", "Extract the selected time range to a temporary WAV and play it in the Unreal Editor preview audio device."))
							.OnClicked(this, &SMP4SoundWaveExtractor::HandlePreviewClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 18.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("StopPreview", "Stop Preview"))
							.OnClicked(this, &SMP4SoundWaveExtractor::HandleStopPreviewClicked)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("CreateSoundWave", "Create Sound Wave"))
							.ToolTipText(LOCTEXT("CreateSoundWaveTooltip", "Extract the selected time range, import it into the destination folder, and save the Sound Wave asset."))
							.OnClicked(this, &SMP4SoundWaveExtractor::HandleCreateClicked)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(this, &SMP4SoundWaveExtractor::GetStatusText)
						.AutoWrapText(true)
					]
				]
			];
		}

		virtual ~SMP4SoundWaveExtractor() override
		{
			StopPreview();
		}

	private:
		TSharedRef<SWidget> MakeSourceRow()
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock).Text(LOCTEXT("SourceLabel", "Source MP4"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SAssignNew(SourcePathTextBox, SEditableTextBox)
					.Text(FText::GetEmpty())
					.HintText(LOCTEXT("SourceHint", "Select an .mp4 file"))
					.OnTextCommitted(this, &SMP4SoundWaveExtractor::HandleSourcePathCommitted)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("Browse", "Browse..."))
					.OnClicked(this, &SMP4SoundWaveExtractor::HandleBrowseClicked)
				];
		}

		TSharedRef<SWidget> MakeTextRow(const FText Label, TSharedPtr<SEditableTextBox>& OutTextBox)
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock).Text(Label)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(OutTextBox, SEditableTextBox)
					.Text(Label.EqualTo(LOCTEXT("DestinationLabel", "Destination Folder"))
						? FText::FromString(DefaultDestinationPath)
						: FText::FromString(TEXT("SW_ExtractedAudio")))
				];
		}

		FReply HandleBrowseClicked()
		{
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			if (!DesktopPlatform)
			{
				SetStatus(LOCTEXT("NoDesktopPlatform", "The file dialog is unavailable."));
				return FReply::Handled();
			}

			const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
			TArray<FString> SelectedFiles;
			const FString DefaultPath = SourcePath.IsEmpty() ? FPaths::ProjectDir() : FPaths::GetPath(SourcePath);
			if (!DesktopPlatform->OpenFileDialog(
				ParentWindowHandle,
				TEXT("Select MP4 File"),
				DefaultPath,
				TEXT(""),
				TEXT("MP4 Video (*.mp4)|*.mp4"),
				EFileDialogFlags::None,
				SelectedFiles) ||
				SelectedFiles.IsEmpty())
			{
				return FReply::Handled();
			}

			SetSourcePath(SelectedFiles[0]);
			return FReply::Handled();
		}

		FReply HandlePreviewClicked()
		{
			StopPreview();
			const FString PreviewWavPath = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("MP4SoundWaveExtractor"),
				TEXT("Preview.wav"));

			SetStatus(LOCTEXT("PreviewExtracting", "Extracting the selected range for preview..."));
			FString Error;
			if (!ExtractAudioRangeToWav(SourcePath, StartSeconds, EndSeconds, PreviewWavPath, Error))
			{
				SetStatus(FText::FromString(Error));
				return FReply::Handled();
			}

			FModuleManager::Get().LoadModuleChecked(TEXT("AudioEditor"));
			USoundFactory* SoundFactory = NewObject<USoundFactory>();
			SoundFactory->bAutoCreateCue = false;
			SoundFactory->SuppressImportDialogs();
			const FName PreviewName = MakeUniqueObjectName(
				GetTransientPackage(),
				USoundWave::StaticClass(),
				TEXT("MP4SoundWavePreview"));
			TArray<uint8> PreviewWavBytes;
			if (!FFileHelper::LoadFileToArray(PreviewWavBytes, *PreviewWavPath))
			{
				SetStatus(LOCTEXT("PreviewWavReadFailed", "The preview WAV was extracted, but its data could not be loaded."));
				return FReply::Handled();
			}

			const uint8* WavData = PreviewWavBytes.GetData();
			USoundWave* PreviewWave = Cast<USoundWave>(SoundFactory->FactoryCreateBinary(
				USoundWave::StaticClass(),
				GetTransientPackage(),
				PreviewName,
				RF_Transient,
				nullptr,
				TEXT("wav"),
				WavData,
				WavData + PreviewWavBytes.Num(),
				GWarn));
			if (!PreviewWave)
			{
				SetStatus(LOCTEXT("PreviewImportFailed", "The preview WAV was extracted, but Unreal could not load it for preview."));
				return FReply::Handled();
			}

			PreviewSoundWave.Reset(PreviewWave);
			if (GEditor)
			{
				GEditor->PlayPreviewSound(PreviewSoundWave.Get());
				SetStatus(LOCTEXT("PreviewPlaying", "Playing the selected MP4 audio range."));
			}
			else
			{
				SetStatus(LOCTEXT("PreviewUnavailable", "The editor preview audio device is unavailable."));
			}
			return FReply::Handled();
		}

		FReply HandleStopPreviewClicked()
		{
			StopPreview();
			SetStatus(LOCTEXT("PreviewStopped", "Preview stopped."));
			return FReply::Handled();
		}

		FReply HandleCreateClicked()
		{
			const FString DestinationPath = GetDestinationPath();
			const FString AssetName = SanitizeAssetName(GetAssetName());
			if (!IsValidDestinationPath(DestinationPath))
			{
				SetStatus(LOCTEXT("InvalidDestination", "Destination Folder must be a valid /Game path."));
				return FReply::Handled();
			}

			const FString ExportWavPath = FPaths::Combine(
				FPaths::ProjectSavedDir(),
				TEXT("MP4SoundWaveExtractor"),
				TEXT("Exports"),
				AssetName + TEXT(".wav"));

			SetStatus(LOCTEXT("Extracting", "Extracting the selected MP4 audio range..."));
			FString Error;
			if (!ExtractAudioRangeToWav(SourcePath, StartSeconds, EndSeconds, ExportWavPath, Error))
			{
				SetStatus(FText::FromString(Error));
				return FReply::Handled();
			}

			USoundWave* SoundWave = nullptr;
			if (!ImportWavAsSoundWave(ExportWavPath, DestinationPath, SoundWave, Error))
			{
				SetStatus(FText::FromString(Error));
				return FReply::Handled();
			}

			TArray<UObject*> AssetsToSync;
			AssetsToSync.Add(SoundWave);
			if (GEditor)
			{
				GEditor->SyncBrowserToObjects(AssetsToSync);
			}

			if (AssetNameTextBox.IsValid())
			{
				AssetNameTextBox->SetText(FText::FromString(AssetName));
			}
			SetStatus(FText::Format(
				LOCTEXT("Created", "Created and saved Sound Wave: {0}"),
				FText::FromString(SoundWave->GetPathName())));
			return FReply::Handled();
		}

		void HandleSourcePathCommitted(const FText& InText, ETextCommit::Type)
		{
			SetSourcePath(InText.ToString());
		}

		void HandleRangeChanged(const float InStartSeconds, const float InEndSeconds)
		{
			StartSeconds = InStartSeconds;
			EndSeconds = InEndSeconds;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		void SetSourcePath(FString InSourcePath)
		{
			StopPreview();
			++WaveformRequestId;
			InSourcePath.TrimStartAndEndInline();
			SourcePath = InSourcePath;
			if (SourcePathTextBox.IsValid())
			{
				SourcePathTextBox->SetText(FText::FromString(SourcePath));
			}

			const FString BaseName = SanitizeAssetName(TEXT("SW_") + FPaths::GetBaseFilename(SourcePath));
			if (AssetNameTextBox.IsValid() && !SourcePath.IsEmpty())
			{
				AssetNameTextBox->SetText(FText::FromString(BaseName));
			}

			if (SourcePath.IsEmpty())
			{
				DurationSeconds = 0.0;
				StartSeconds = 0.0f;
				EndSeconds = 0.0f;
				if (RangeSelector.IsValid())
				{
					RangeSelector->SetDuration(0.0f);
					TArray<float> EmptyWaveform;
					RangeSelector->SetWaveformPeaks(MoveTemp(EmptyWaveform));
				}
				return;
			}

			FString DurationError;
			if (TryReadDurationSeconds(SourcePath, DurationSeconds, DurationError))
			{
				EndSeconds = static_cast<float>(DurationSeconds);
				StartSeconds = 0.0f;
				if (RangeSelector.IsValid())
				{
					RangeSelector->SetDuration(static_cast<float>(DurationSeconds));
					RangeSelector->SetRange(StartSeconds, EndSeconds);
					TArray<float> EmptyWaveform;
					RangeSelector->SetWaveformPeaks(MoveTemp(EmptyWaveform));
				}
				SetStatus(LOCTEXT("SourceLoaded", "MP4 loaded. Building the waveform preview in the background..."));
				BeginWaveformAnalysis();
			}
			else
			{
				DurationSeconds = 0.0;
				StartSeconds = 0.0f;
				EndSeconds = 0.0f;
				if (RangeSelector.IsValid())
				{
					RangeSelector->SetDuration(0.0f);
					TArray<float> EmptyWaveform;
					RangeSelector->SetWaveformPeaks(MoveTemp(EmptyWaveform));
				}
				SetStatus(FText::FromString(DurationError));
			}
		}

		void BeginWaveformAnalysis()
		{
			if (SourcePath.IsEmpty() || DurationSeconds <= 0.0)
			{
				return;
			}

			const uint32 RequestId = WaveformRequestId;
			const FString AnalysisSourcePath = SourcePath;
			const double AnalysisDurationSeconds = DurationSeconds;
			const TWeakPtr<SMP4SoundWaveExtractor> WeakTool = StaticCastSharedRef<SMP4SoundWaveExtractor>(AsShared());
			Async(EAsyncExecution::ThreadPool, [WeakTool, RequestId, AnalysisSourcePath, AnalysisDurationSeconds]()
			{
				TArray<float> Peaks;
				FString Error;
				const bool bSucceeded = BuildWaveformPeaks(
					AnalysisSourcePath,
					AnalysisDurationSeconds,
					512,
					Peaks,
					Error);
				AsyncTask(ENamedThreads::GameThread, [WeakTool, RequestId, bSucceeded, Peaks = MoveTemp(Peaks), Error = MoveTemp(Error)]() mutable
				{
					const TSharedPtr<SMP4SoundWaveExtractor> Tool = WeakTool.Pin();
					if (!Tool.IsValid() || Tool->WaveformRequestId != RequestId)
					{
						return;
					}

					Tool->ApplyWaveformAnalysis(bSucceeded, MoveTemp(Peaks), Error);
				});
			});
		}

		void ApplyWaveformAnalysis(const bool bSucceeded, TArray<float>&& Peaks, const FString& Error)
		{
			if (RangeSelector.IsValid())
			{
				RangeSelector->SetWaveformPeaks(MoveTemp(Peaks));
			}

			SetStatus(bSucceeded
				? LOCTEXT("WaveformReady", "Waveform ready. Drag A and B to select the audio range, then preview or create the Sound Wave.")
				: FText::Format(
					LOCTEXT("WaveformUnavailable", "Waveform could not be built ({0}). You can still drag the A and B handles."),
					FText::FromString(Error)));
		}

		void StopPreview()
		{
			if (GEditor)
			{
				if (UAudioComponent* PreviewComponent = GEditor->GetPreviewAudioComponent())
				{
					if (PreviewComponent->Sound == PreviewSoundWave.Get())
					{
						PreviewComponent->Stop();
					}
				}
			}

			PreviewSoundWave.Reset();
		}

		FText GetDurationText() const
		{
			return DurationSeconds > 0.0
				? FText::Format(LOCTEXT("Duration", "MP4 Duration: {0} seconds"), FText::AsNumber(DurationSeconds, &FNumberFormattingOptions::DefaultNoGrouping()))
				: LOCTEXT("DurationUnknown", "MP4 Duration: select a file to read its duration.");
		}

		FText GetStatusText() const
		{
			return StatusText;
		}

		FText GetSelectionText() const
		{
			const float SelectedDuration = FMath::Max(0.0f, EndSeconds - StartSeconds);
			return FText::FromString(FString::Printf(
				TEXT("A  %.2fs   —   B  %.2fs   (Selection: %.2fs)"),
				StartSeconds,
				EndSeconds,
				SelectedDuration));
		}

		FString GetDestinationPath() const
		{
			return DestinationPathTextBox.IsValid() ? DestinationPathTextBox->GetText().ToString() : DefaultDestinationPath;
		}

		FString GetAssetName() const
		{
			return AssetNameTextBox.IsValid() ? AssetNameTextBox->GetText().ToString() : TEXT("SW_ExtractedAudio");
		}

		bool IsValidDestinationPath(FString InPath) const
		{
			InPath.TrimStartAndEndInline();
			InPath.ReplaceInline(TEXT("\\"), TEXT("/"));
			while (InPath.EndsWith(TEXT("/")))
			{
				InPath.LeftChopInline(1, EAllowShrinking::No);
			}

			return InPath.StartsWith(TEXT("/Game")) &&
				FPackageName::IsValidLongPackageName(InPath + TEXT("/MP4SoundWave"));
		}

		void SetStatus(const FText& InStatus)
		{
			StatusText = InStatus;
		}

		TSharedPtr<SEditableTextBox> SourcePathTextBox;
		TSharedPtr<SEditableTextBox> DestinationPathTextBox;
		TSharedPtr<SEditableTextBox> AssetNameTextBox;
		TSharedPtr<SMP4AudioRangeSelector> RangeSelector;
		TStrongObjectPtr<USoundWave> PreviewSoundWave;
		FString SourcePath;
		float StartSeconds = 0.0f;
		float EndSeconds = 5.0f;
		double DurationSeconds = 0.0;
		uint32 WaveformRequestId = 0;
		FText StatusText = LOCTEXT("InitialStatus", "Select an MP4 file to begin.");
	};
}

class FMP4SoundWaveExtractorEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
			MP4SoundWaveExtractor::TabName,
			FOnSpawnTab::CreateRaw(this, &FMP4SoundWaveExtractorEditorModule::SpawnToolTab))
			.SetDisplayName(LOCTEXT("TabTitle", "MP4 Sound Wave Extractor"))
			.SetTooltipText(LOCTEXT("TabTooltip", "Extract selected MP4 audio ranges into Sound Wave assets."))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMP4SoundWaveExtractorEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}

		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MP4SoundWaveExtractor::TabName);
	}

private:
	void RegisterMenus()
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
			LOCTEXT("AudioSection", "Audio"));
		Section.AddMenuEntry(
			TEXT("OpenMP4SoundWaveExtractor"),
			LOCTEXT("MenuEntry", "MP4 Sound Wave Extractor"),
			LOCTEXT("MenuEntryTooltip", "Open the MP4 Sound Wave Extractor window."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FMP4SoundWaveExtractorEditorModule::OpenToolWindow)));
	}

	void OpenToolWindow()
	{
		FGlobalTabmanager::Get()->TryInvokeTab(MP4SoundWaveExtractor::TabName);
	}

	TSharedRef<SDockTab> SpawnToolTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(MP4SoundWaveExtractor::SMP4SoundWaveExtractor)
			];
	}
};

IMPLEMENT_MODULE(FMP4SoundWaveExtractorEditorModule, MP4SoundWaveExtractorEditor)

#undef LOCTEXT_NAMESPACE
