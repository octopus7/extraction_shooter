#include "SogDecoder.h"

#include "Containers/StringConv.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SogAsset.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
THIRD_PARTY_INCLUDES_START
#include "FreeImage.h"
#include "libzip/zip.h"
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "SogDecoder"

namespace
{
	constexpr double SH_C0 = 0.28209479177387814;
	constexpr double SourcePlaneWidthCm = 100.0;

	struct FDecodedImage
	{
		int32 Width = 0;
		int32 Height = 0;
		TArray<FColor> Pixels;

		const FColor& GetPixelByIndex(int32 Index) const
		{
			return Pixels[Index];
		}
	};

	struct FParsedMeta
	{
		int32 Version = 0;
		int32 Count = 0;
		TArray<float> MeansMins;
		TArray<float> MeansMaxs;
		TArray<FString> MeansFiles;
		TArray<float> ScaleCodebook;
		TArray<FString> ScaleFiles;
		TArray<FString> QuatFiles;
		TArray<float> Sh0Codebook;
		TArray<FString> Sh0Files;
	};

	class FFreeImageRuntime
	{
	public:
		static bool Initialize(FText& OutError)
		{
#if WITH_FREEIMAGE_LIB
			if (FreeImageDllHandle)
			{
				return true;
			}

			const FString FreeImageDir = FPaths::Combine(
				FPaths::EngineDir(),
				TEXT("Binaries/ThirdParty/FreeImage"),
				FPlatformProcess::GetBinariesSubdirectory());
			const FString FreeImagePath = FPaths::Combine(FreeImageDir, TEXT(FREEIMAGE_LIB_FILENAME));

			FPlatformProcess::PushDllDirectory(*FreeImageDir);
			FreeImageDllHandle = FPlatformProcess::GetDllHandle(*FreeImagePath);
			FPlatformProcess::PopDllDirectory(*FreeImageDir);

			if (!FreeImageDllHandle)
			{
				OutError = FText::Format(LOCTEXT("FreeImageLoadFailed", "Failed to load FreeImage DLL: {0}"), FText::FromString(FreeImagePath));
				return false;
			}

			::FreeImage_Initialise(false);
			return true;
#else
			OutError = LOCTEXT("FreeImageUnavailable", "FreeImage support is not available for this target.");
			return false;
#endif
		}

	private:
		static void* FreeImageDllHandle;
	};

	void* FFreeImageRuntime::FreeImageDllHandle = nullptr;

	FString NormalizeArchivePath(FString Path)
	{
		Path.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Path.StartsWith(TEXT("./")))
		{
			Path.RightChopInline(2);
		}
		while (Path.StartsWith(TEXT("/")))
		{
			Path.RightChopInline(1);
		}
		return Path;
	}

	bool ReadZipArchive(const TArray<uint8>& FileBytes, TMap<FString, TArray<uint8>>& OutFiles, FText& OutError)
	{
		zip_error_t ZipError;
		zip_error_init(&ZipError);

		zip_source_t* Source = zip_source_buffer_create(FileBytes.GetData(), FileBytes.Num(), 0, &ZipError);
		if (!Source)
		{
			OutError = FText::Format(LOCTEXT("ZipSourceFailed", "Failed to open SOG archive buffer: {0}"), FText::FromString(UTF8_TO_TCHAR(zip_error_strerror(&ZipError))));
			zip_error_fini(&ZipError);
			return false;
		}

		zip_t* Archive = zip_open_from_source(Source, ZIP_RDONLY, &ZipError);
		if (!Archive)
		{
			OutError = FText::Format(LOCTEXT("ZipOpenFailed", "Failed to read SOG ZIP archive: {0}"), FText::FromString(UTF8_TO_TCHAR(zip_error_strerror(&ZipError))));
			zip_source_free(Source);
			zip_error_fini(&ZipError);
			return false;
		}

		const zip_int64_t EntryCount = zip_get_num_entries(Archive, 0);
		for (zip_uint64_t EntryIndex = 0; EntryIndex < static_cast<zip_uint64_t>(EntryCount); ++EntryIndex)
		{
			zip_stat_t Stat;
			zip_stat_init(&Stat);
			if (zip_stat_index(Archive, EntryIndex, 0, &Stat) != 0 || !Stat.name)
			{
				continue;
			}

			const FString EntryName = NormalizeArchivePath(UTF8_TO_TCHAR(Stat.name));
			if (EntryName.IsEmpty() || EntryName.EndsWith(TEXT("/")))
			{
				continue;
			}

			if ((Stat.valid & ZIP_STAT_SIZE) == 0 || Stat.size > static_cast<zip_uint64_t>(MAX_int32))
			{
				OutError = FText::Format(LOCTEXT("ZipEntryTooLarge", "SOG archive entry has an unsupported size: {0}"), FText::FromString(EntryName));
				zip_close(Archive);
				zip_error_fini(&ZipError);
				return false;
			}

			zip_file_t* ZipFile = zip_fopen_index(Archive, EntryIndex, 0);
			if (!ZipFile)
			{
				OutError = FText::Format(LOCTEXT("ZipEntryOpenFailed", "Failed to open SOG archive entry: {0}"), FText::FromString(EntryName));
				zip_close(Archive);
				zip_error_fini(&ZipError);
				return false;
			}

			TArray<uint8> EntryBytes;
			EntryBytes.SetNumUninitialized(static_cast<int32>(Stat.size));

			zip_uint64_t TotalRead = 0;
			while (TotalRead < Stat.size)
			{
				const zip_int64_t ReadCount = zip_fread(ZipFile, EntryBytes.GetData() + TotalRead, Stat.size - TotalRead);
				if (ReadCount < 0)
				{
					zip_fclose(ZipFile);
					OutError = FText::Format(LOCTEXT("ZipEntryReadFailed", "Failed to read SOG archive entry: {0}"), FText::FromString(EntryName));
					zip_close(Archive);
					zip_error_fini(&ZipError);
					return false;
				}
				if (ReadCount == 0)
				{
					break;
				}
				TotalRead += static_cast<zip_uint64_t>(ReadCount);
			}

			zip_fclose(ZipFile);

			if (TotalRead != Stat.size)
			{
				OutError = FText::Format(LOCTEXT("ZipEntryShortRead", "SOG archive entry ended early: {0}"), FText::FromString(EntryName));
				zip_close(Archive);
				zip_error_fini(&ZipError);
				return false;
			}

			OutFiles.Add(EntryName, MoveTemp(EntryBytes));
		}

		zip_close(Archive);
		zip_error_fini(&ZipError);
		return true;
	}

	const TArray<uint8>* FindArchiveFile(const TMap<FString, TArray<uint8>>& Files, const FString& RequestedPath)
	{
		const FString NormalizedPath = NormalizeArchivePath(RequestedPath);
		if (const TArray<uint8>* Exact = Files.Find(NormalizedPath))
		{
			return Exact;
		}

		const FString BaseName = FPaths::GetCleanFilename(NormalizedPath);
		for (const TPair<FString, TArray<uint8>>& Pair : Files)
		{
			if (FPaths::GetCleanFilename(Pair.Key).Equals(BaseName, ESearchCase::IgnoreCase))
			{
				return &Pair.Value;
			}
		}

		return nullptr;
	}

	bool DecodeImage(const TArray<uint8>& EncodedBytes, FDecodedImage& OutImage, FText& OutError)
	{
		if (!FFreeImageRuntime::Initialize(OutError))
		{
			return false;
		}

		FIMEMORY* Memory = FreeImage_OpenMemory(const_cast<BYTE*>(EncodedBytes.GetData()), EncodedBytes.Num());
		if (!Memory)
		{
			OutError = LOCTEXT("ImageMemoryFailed", "Failed to open SOG image memory.");
			return false;
		}

		FREE_IMAGE_FORMAT Format = FreeImage_GetFileTypeFromMemory(Memory, 0);
		if (Format == FIF_UNKNOWN)
		{
			Format = FIF_WEBP;
		}

		FIBITMAP* Bitmap = FreeImage_LoadFromMemory(Format, Memory, 0);
		if (!Bitmap)
		{
			FreeImage_CloseMemory(Memory);
			OutError = LOCTEXT("ImageDecodeFailed", "Failed to decode a SOG image. Lossless WebP is expected.");
			return false;
		}

		FIBITMAP* Converted = FreeImage_ConvertTo32Bits(Bitmap);
		FreeImage_Unload(Bitmap);
		if (!Converted)
		{
			FreeImage_CloseMemory(Memory);
			OutError = LOCTEXT("ImageConvertFailed", "Failed to convert a SOG image to 32-bit RGBA.");
			return false;
		}

		OutImage.Width = static_cast<int32>(FreeImage_GetWidth(Converted));
		OutImage.Height = static_cast<int32>(FreeImage_GetHeight(Converted));
		OutImage.Pixels.SetNumUninitialized(OutImage.Width * OutImage.Height);

		const BYTE* Bits = FreeImage_GetBits(Converted);
		const int32 Pitch = static_cast<int32>(FreeImage_GetPitch(Converted));
		for (int32 Y = 0; Y < OutImage.Height; ++Y)
		{
			const BYTE* SourceRow = Bits + (OutImage.Height - 1 - Y) * Pitch;
			for (int32 X = 0; X < OutImage.Width; ++X)
			{
				const BYTE* SourcePixel = SourceRow + X * 4;
				OutImage.Pixels[Y * OutImage.Width + X] = FColor(
					SourcePixel[FI_RGBA_RED],
					SourcePixel[FI_RGBA_GREEN],
					SourcePixel[FI_RGBA_BLUE],
					SourcePixel[FI_RGBA_ALPHA]);
			}
		}

		FreeImage_Unload(Converted);
		FreeImage_CloseMemory(Memory);
		return true;
	}

	bool BytesToString(const TArray<uint8>& Bytes, FString& OutString)
	{
		if (Bytes.IsEmpty())
		{
			OutString.Reset();
			return true;
		}

		FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
		OutString = FString(Converter.Length(), Converter.Get());
		return true;
	}

	bool ReadFloatArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, int32 ExpectedCount, TArray<float>& OutValues, FText& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* JsonValues = nullptr;
		if (!Object->TryGetArrayField(FieldName, JsonValues))
		{
			OutError = FText::Format(LOCTEXT("MetaArrayMissing", "SOG meta.json is missing array field: {0}"), FText::FromString(FieldName));
			return false;
		}

		if (ExpectedCount > 0 && JsonValues->Num() != ExpectedCount)
		{
			OutError = FText::Format(LOCTEXT("MetaArrayWrongSize", "SOG meta.json field {0} has an unexpected length."), FText::FromString(FieldName));
			return false;
		}

		OutValues.Reset(JsonValues->Num());
		for (const TSharedPtr<FJsonValue>& Value : *JsonValues)
		{
			OutValues.Add(static_cast<float>(Value->AsNumber()));
		}
		return true;
	}

	bool ReadStringArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, int32 ExpectedCount, TArray<FString>& OutValues, FText& OutError)
	{
		const TArray<TSharedPtr<FJsonValue>>* JsonValues = nullptr;
		if (!Object->TryGetArrayField(FieldName, JsonValues))
		{
			OutError = FText::Format(LOCTEXT("MetaStringArrayMissing", "SOG meta.json is missing file list: {0}"), FText::FromString(FieldName));
			return false;
		}

		if (ExpectedCount > 0 && JsonValues->Num() != ExpectedCount)
		{
			OutError = FText::Format(LOCTEXT("MetaStringArrayWrongSize", "SOG meta.json file list {0} has an unexpected length."), FText::FromString(FieldName));
			return false;
		}

		OutValues.Reset(JsonValues->Num());
		for (const TSharedPtr<FJsonValue>& Value : *JsonValues)
		{
			OutValues.Add(Value->AsString());
		}
		return true;
	}

	bool ReadObjectField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TSharedPtr<FJsonObject>& OutObject, FText& OutError)
	{
		const TSharedPtr<FJsonObject>* FoundObject = nullptr;
		if (!Object->TryGetObjectField(FieldName, FoundObject) || !FoundObject)
		{
			OutError = FText::Format(LOCTEXT("MetaObjectMissing", "SOG meta.json is missing object field: {0}"), FText::FromString(FieldName));
			return false;
		}

		OutObject = *FoundObject;
		return OutObject.IsValid();
	}

	bool ParseMetaJson(const TArray<uint8>& MetaBytes, FParsedMeta& OutMeta, FText& OutError)
	{
		FString JsonText;
		BytesToString(MetaBytes, JsonText);

		TSharedPtr<FJsonObject> RootObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
		{
			OutError = LOCTEXT("MetaParseFailed", "Failed to parse SOG meta.json.");
			return false;
		}

		OutMeta.Version = RootObject->GetIntegerField(TEXT("version"));
		OutMeta.Count = RootObject->GetIntegerField(TEXT("count"));
		if (OutMeta.Version != 2)
		{
			OutError = FText::Format(LOCTEXT("UnsupportedVersion", "Unsupported SOG version: {0}. Version 2 is expected."), FText::AsNumber(OutMeta.Version));
			return false;
		}

		TSharedPtr<FJsonObject> MeansObject;
		TSharedPtr<FJsonObject> ScalesObject;
		TSharedPtr<FJsonObject> QuatsObject;
		TSharedPtr<FJsonObject> Sh0Object;
		if (!ReadObjectField(RootObject, TEXT("means"), MeansObject, OutError)
			|| !ReadObjectField(RootObject, TEXT("scales"), ScalesObject, OutError)
			|| !ReadObjectField(RootObject, TEXT("quats"), QuatsObject, OutError)
			|| !ReadObjectField(RootObject, TEXT("sh0"), Sh0Object, OutError))
		{
			return false;
		}

		return ReadFloatArray(MeansObject, TEXT("mins"), 3, OutMeta.MeansMins, OutError)
			&& ReadFloatArray(MeansObject, TEXT("maxs"), 3, OutMeta.MeansMaxs, OutError)
			&& ReadStringArray(MeansObject, TEXT("files"), 2, OutMeta.MeansFiles, OutError)
			&& ReadFloatArray(ScalesObject, TEXT("codebook"), 256, OutMeta.ScaleCodebook, OutError)
			&& ReadStringArray(ScalesObject, TEXT("files"), 1, OutMeta.ScaleFiles, OutError)
			&& ReadStringArray(QuatsObject, TEXT("files"), 1, OutMeta.QuatFiles, OutError)
			&& ReadFloatArray(Sh0Object, TEXT("codebook"), 256, OutMeta.Sh0Codebook, OutError)
			&& ReadStringArray(Sh0Object, TEXT("files"), 1, OutMeta.Sh0Files, OutError);
	}

	bool LoadImageFromArchive(const TMap<FString, TArray<uint8>>& Files, const FString& FileName, FDecodedImage& OutImage, FText& OutError)
	{
		const TArray<uint8>* ImageBytes = FindArchiveFile(Files, FileName);
		if (!ImageBytes)
		{
			OutError = FText::Format(LOCTEXT("ImageMissing", "SOG archive is missing image: {0}"), FText::FromString(FileName));
			return false;
		}

		return DecodeImage(*ImageBytes, OutImage, OutError);
	}

	bool ValidateImageDimensions(const FParsedMeta& Meta, const FDecodedImage& MeansL, const FDecodedImage& MeansU, const FDecodedImage& Scales, const FDecodedImage& Quats, const FDecodedImage& Sh0, FText& OutError)
	{
		const int64 PixelCount = static_cast<int64>(MeansL.Width) * static_cast<int64>(MeansL.Height);
		if (Meta.Count <= 0 || Meta.Count > PixelCount)
		{
			OutError = LOCTEXT("InvalidSplatCount", "SOG meta.json count is invalid for the property image dimensions.");
			return false;
		}

		const bool bDimensionsMatch =
			MeansL.Width == MeansU.Width && MeansL.Height == MeansU.Height
			&& MeansL.Width == Scales.Width && MeansL.Height == Scales.Height
			&& MeansL.Width == Quats.Width && MeansL.Height == Quats.Height
			&& MeansL.Width == Sh0.Width && MeansL.Height == Sh0.Height;
		if (!bDimensionsMatch)
		{
			OutError = LOCTEXT("ImageDimensionMismatch", "SOG property images do not have matching dimensions.");
			return false;
		}

		return true;
	}

	float DecodeUnlog(float Value)
	{
		return FMath::Sign(Value) * (FMath::Exp(FMath::Abs(Value)) - 1.0f);
	}

	float DecodeQuantizedAxis(uint8 LowByte, uint8 HighByte, float MinValue, float MaxValue)
	{
		const uint16 Quantized = (static_cast<uint16>(HighByte) << 8) | static_cast<uint16>(LowByte);
		const float Alpha = static_cast<float>(Quantized) / 65535.0f;
		return DecodeUnlog(FMath::Lerp(MinValue, MaxValue, Alpha));
	}

	FVector ConvertSogVectorToUnreal(const FVector& SogVector, bool bFlipVerticalAxis)
	{
		const double UnrealZ = bFlipVerticalAxis ? -SogVector.Y : SogVector.Y;
		return FVector(-SogVector.Z, SogVector.X, UnrealZ);
	}

	FVector GetSogBasisAxis(int32 AxisIndex)
	{
		switch (AxisIndex)
		{
		case 0:
			return FVector(1.0, 0.0, 0.0);
		case 1:
			return FVector(0.0, 1.0, 0.0);
		case 2:
			return FVector(0.0, 0.0, 1.0);
		default:
			return FVector::ForwardVector;
		}
	}

	float DecodeSogScale(float EncodedScale)
	{
		return FMath::Max(FMath::Exp(FMath::Clamp(EncodedScale, -20.0f, 20.0f)), KINDA_SMALL_NUMBER);
	}

	void SelectPlaneAxes(const FVector& DecodedScales, int32& OutMajorAxis, int32& OutMinorAxis)
	{
		const float AxisScales[3] = { DecodedScales.X, DecodedScales.Y, DecodedScales.Z };

		OutMajorAxis = 0;
		for (int32 AxisIndex = 1; AxisIndex < 3; ++AxisIndex)
		{
			if (AxisScales[AxisIndex] > AxisScales[OutMajorAxis])
			{
				OutMajorAxis = AxisIndex;
			}
		}

		OutMinorAxis = OutMajorAxis == 0 ? 1 : 0;
		for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
		{
			if (AxisIndex != OutMajorAxis && AxisScales[AxisIndex] > AxisScales[OutMinorAxis])
			{
				OutMinorAxis = AxisIndex;
			}
		}
	}

	FQuat DecodeSogQuaternion(const FColor& EncodedQuat)
	{
		const int32 Mode = static_cast<int32>(EncodedQuat.A) - 252;
		if (Mode < 0 || Mode > 3)
		{
			return FQuat::Identity;
		}

		constexpr float InvSqrt2 = UE_INV_SQRT_2;
		const auto ToComponent = [](uint8 Component)
		{
			return (static_cast<float>(Component) / 255.0f - 0.5f) * 2.0f * InvSqrt2;
		};

		const float A = ToComponent(EncodedQuat.R);
		const float B = ToComponent(EncodedQuat.G);
		const float C = ToComponent(EncodedQuat.B);
		const float Missing = FMath::Sqrt(FMath::Max(0.0f, 1.0f - A * A - B * B - C * C));

		FQuat Quat = FQuat::Identity;
		switch (Mode)
		{
		case 0:
			Quat = FQuat(Missing, A, B, C);
			break;
		case 1:
			Quat = FQuat(A, Missing, B, C);
			break;
		case 2:
			Quat = FQuat(A, B, Missing, C);
			break;
		case 3:
			Quat = FQuat(A, B, C, Missing);
			break;
		default:
			break;
		}

		Quat.Normalize();
		return Quat;
	}

	FQuat ConvertSogQuaternionToUnrealPlaneRotation(const FQuat& SogQuat, int32 PlaneXAxis, int32 PlaneYAxis, bool bFlipVerticalAxis)
	{
		const FVector SogXAxis = SogQuat.RotateVector(GetSogBasisAxis(PlaneXAxis));
		const FVector SogYAxis = SogQuat.RotateVector(GetSogBasisAxis(PlaneYAxis));
		const FVector UnrealXAxis = ConvertSogVectorToUnreal(SogXAxis, bFlipVerticalAxis).GetSafeNormal();
		const FVector UnrealYAxis = ConvertSogVectorToUnreal(SogYAxis, bFlipVerticalAxis).GetSafeNormal();

		if (UnrealXAxis.IsNearlyZero() || UnrealYAxis.IsNearlyZero())
		{
			return FQuat::Identity;
		}

		return FRotationMatrix::MakeFromXY(UnrealXAxis, UnrealYAxis).ToQuat();
	}

	float ToLinearColorChannel(float GammaChannel)
	{
		return FMath::Pow(FMath::Clamp(GammaChannel, 0.0f, 1.0f), 2.2f);
	}

	bool DecodeSplats(const FParsedMeta& Meta, const FSogDecodeOptions& Options, const FDecodedImage& MeansL, const FDecodedImage& MeansU, const FDecodedImage& Scales, const FDecodedImage& Quats, const FDecodedImage& Sh0, TArray<FSogSplatInstance>& OutSplats)
	{
		int32 Stride = FMath::Max(1, Options.ImportStride);
		if (Options.MaxImportedSplats > 0)
		{
			Stride = FMath::Max(Stride, FMath::DivideAndRoundUp(Meta.Count, Options.MaxImportedSplats));
		}

		OutSplats.Reset(FMath::DivideAndRoundUp(Meta.Count, Stride));

		for (int32 Index = 0; Index < Meta.Count; Index += Stride)
		{
			const FColor& MeansLow = MeansL.GetPixelByIndex(Index);
			const FColor& MeansHigh = MeansU.GetPixelByIndex(Index);
			const FColor& ScaleIndices = Scales.GetPixelByIndex(Index);
			const FColor& EncodedQuat = Quats.GetPixelByIndex(Index);
			const FColor& Sh0Value = Sh0.GetPixelByIndex(Index);

			const FVector SogPosition(
				DecodeQuantizedAxis(MeansLow.R, MeansHigh.R, Meta.MeansMins[0], Meta.MeansMaxs[0]),
				DecodeQuantizedAxis(MeansLow.G, MeansHigh.G, Meta.MeansMins[1], Meta.MeansMaxs[1]),
				DecodeQuantizedAxis(MeansLow.B, MeansHigh.B, Meta.MeansMins[2], Meta.MeansMaxs[2]));

			const FVector UnrealPosition = ConvertSogVectorToUnreal(SogPosition, Options.bFlipVerticalAxis) * Options.UnitsPerSogUnit;
			const FVector DecodedScales(
				DecodeSogScale(Meta.ScaleCodebook[ScaleIndices.R]),
				DecodeSogScale(Meta.ScaleCodebook[ScaleIndices.G]),
				DecodeSogScale(Meta.ScaleCodebook[ScaleIndices.B]));

			int32 PlaneXAxis = 0;
			int32 PlaneYAxis = 1;
			SelectPlaneAxes(DecodedScales, PlaneXAxis, PlaneYAxis);

			const FQuat UnrealRotation = ConvertSogQuaternionToUnrealPlaneRotation(
				DecodeSogQuaternion(EncodedQuat),
				PlaneXAxis,
				PlaneYAxis,
				Options.bFlipVerticalAxis);

			const float AxisScales[3] = { DecodedScales.X, DecodedScales.Y, DecodedScales.Z };
			float DiameterX = FMath::Max(Options.MinCardDiameterCm, AxisScales[PlaneXAxis] * Options.UnitsPerSogUnit * Options.GaussianRadiusMultiplier * 2.0f);
			float DiameterY = FMath::Max(Options.MinCardDiameterCm, AxisScales[PlaneYAxis] * Options.UnitsPerSogUnit * Options.GaussianRadiusMultiplier * 2.0f);
			if (Options.MaxCardDiameterCm > 0.0f)
			{
				DiameterX = FMath::Min(DiameterX, Options.MaxCardDiameterCm);
				DiameterY = FMath::Min(DiameterY, Options.MaxCardDiameterCm);
			}
			const FVector UnrealScale(DiameterX / SourcePlaneWidthCm, DiameterY / SourcePlaneWidthCm, 1.0f);

			const float GammaR = 0.5f + Meta.Sh0Codebook[Sh0Value.R] * static_cast<float>(SH_C0);
			const float GammaG = 0.5f + Meta.Sh0Codebook[Sh0Value.G] * static_cast<float>(SH_C0);
			const float GammaB = 0.5f + Meta.Sh0Codebook[Sh0Value.B] * static_cast<float>(SH_C0);
			const float Alpha = static_cast<float>(Sh0Value.A) / 255.0f;
			if (Alpha <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			FSogSplatInstance& Splat = OutSplats.AddDefaulted_GetRef();
			Splat.Transform = FTransform(UnrealRotation, UnrealPosition, UnrealScale);
			Splat.Color = Options.bConvertGammaColorToLinear
				? FLinearColor(ToLinearColorChannel(GammaR), ToLinearColorChannel(GammaG), ToLinearColorChannel(GammaB), Alpha)
				: FLinearColor(FMath::Clamp(GammaR, 0.0f, 1.0f), FMath::Clamp(GammaG, 0.0f, 1.0f), FMath::Clamp(GammaB, 0.0f, 1.0f), Alpha);
		}

		return true;
	}
}

bool FSogDecoder::DecodeFileToAsset(const FString& FilePath, const FSogDecodeOptions& Options, USogAsset* TargetAsset, FText& OutError)
{
	if (!TargetAsset)
	{
		OutError = LOCTEXT("NullTargetAsset", "Target SOG asset is null.");
		return false;
	}

	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *FilePath))
	{
		OutError = FText::Format(LOCTEXT("FileReadFailed", "Failed to read SOG file: {0}"), FText::FromString(FilePath));
		return false;
	}

	TMap<FString, TArray<uint8>> ArchiveFiles;
	if (!ReadZipArchive(FileBytes, ArchiveFiles, OutError))
	{
		return false;
	}

	const TArray<uint8>* MetaBytes = FindArchiveFile(ArchiveFiles, TEXT("meta.json"));
	if (!MetaBytes)
	{
		OutError = LOCTEXT("MetaMissing", "SOG archive does not contain meta.json.");
		return false;
	}

	FParsedMeta Meta;
	if (!ParseMetaJson(*MetaBytes, Meta, OutError))
	{
		return false;
	}

	FDecodedImage MeansL;
	FDecodedImage MeansU;
	FDecodedImage Scales;
	FDecodedImage Quats;
	FDecodedImage Sh0;
	if (!LoadImageFromArchive(ArchiveFiles, Meta.MeansFiles[0], MeansL, OutError)
		|| !LoadImageFromArchive(ArchiveFiles, Meta.MeansFiles[1], MeansU, OutError)
		|| !LoadImageFromArchive(ArchiveFiles, Meta.ScaleFiles[0], Scales, OutError)
		|| !LoadImageFromArchive(ArchiveFiles, Meta.QuatFiles[0], Quats, OutError)
		|| !LoadImageFromArchive(ArchiveFiles, Meta.Sh0Files[0], Sh0, OutError))
	{
		return false;
	}

	if (!ValidateImageDimensions(Meta, MeansL, MeansU, Scales, Quats, Sh0, OutError))
	{
		return false;
	}

	TArray<FSogSplatInstance> Splats;
	DecodeSplats(Meta, Options, MeansL, MeansU, Scales, Quats, Sh0, Splats);

	TargetAsset->Modify();
	TargetAsset->SourceFilePath = FilePath;
	TargetAsset->SourceGaussianCount = Meta.Count;
	TargetAsset->DecodeOptions = Options;
	TargetAsset->SetSplats(MoveTemp(Splats));

	return true;
}

#undef LOCTEXT_NAMESPACE
