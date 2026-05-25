#include "Component/TunaSweeperLedExpressionComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperLedExpression, Log, All);

UTunaSweeperLedExpressionComponent::UTunaSweeperLedExpressionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bUseAsyncCooking = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);

	LedMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_LumberjackMeleeSwingArc.M_LumberjackMeleeSwingArc")));
}

void UTunaSweeperLedExpressionComponent::OnRegister()
{
	Super::OnRegister();

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);

	ApplyLedMaterial();
	RefreshExpressionPresets();
	RebuildLedMesh();

	if (!DefaultExpressionName.IsNone() && SetExpressionByName(DefaultExpressionName))
	{
		return;
	}

	if (!ExpressionPresets.IsEmpty())
	{
		for (const TPair<FName, FString>& PresetPair : ExpressionPresets)
		{
			SetExpressionByName(PresetPair.Key);
			break;
		}
	}
}

void UTunaSweeperLedExpressionComponent::BeginPlay()
{
	Super::BeginPlay();

	RefreshExpressionPresets();
	if (!bMeshBuilt)
	{
		RebuildLedMesh();
	}
	if (!CurrentExpressionName.IsNone())
	{
		SetExpressionByName(CurrentExpressionName);
	}
	else
	{
		SetExpressionByName(DefaultExpressionName);
	}
}

#if WITH_EDITOR
void UTunaSweeperLedExpressionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyLedMaterial();
	RefreshExpressionPresets();
	RebuildLedMesh();
	SetExpressionByName(CurrentExpressionName.IsNone() ? DefaultExpressionName : CurrentExpressionName);
}
#endif

bool UTunaSweeperLedExpressionComponent::LoadExpressionPresetFile(bool bForceReload)
{
	if (bPresetFileLoaded && !bForceReload)
	{
		return true;
	}

	return RefreshExpressionPresets();
}

bool UTunaSweeperLedExpressionComponent::RefreshExpressionPresets()
{
	ExpressionPresets.Reset();
	bPresetFileLoaded = LoadPresetFileIntoMap();

	for (const FTunaSweeperLedExpressionPreset& BlueprintPreset : BlueprintPresets)
	{
		if (BlueprintPreset.ExpressionName.IsNone())
		{
			continue;
		}

		const FString NormalizedPattern = NormalizePattern(BlueprintPreset.Pattern);
		if (!NormalizedPattern.IsEmpty())
		{
			ExpressionPresets.Add(BlueprintPreset.ExpressionName, NormalizedPattern);
		}
	}

	return !ExpressionPresets.IsEmpty();
}

void UTunaSweeperLedExpressionComponent::RebuildLedMesh()
{
	MatrixColumns = FMath::Max(1, MatrixColumns);
	MatrixRows = FMath::Max(1, MatrixRows);
	LedPitch = FMath::Max(0.1f, LedPitch);
	LedRadius = FMath::Max(0.01f, LedRadius);
	CircleSegments = FMath::Clamp(CircleSegments, 3, 24);
	HorizontalCurvatureDegrees = FMath::Clamp(HorizontalCurvatureDegrees, 0.0f, 160.0f);

	const int32 LedCount = GetLedCount();
	const int32 VerticesPerLed = CircleSegments + 1;
	if (LedStates.Num() != LedCount)
	{
		LedStates.SetNumZeroed(LedCount);
	}

	CachedVertices.Reset(LedCount * VerticesPerLed);
	CachedNormals.Reset(LedCount * VerticesPerLed);
	CachedUVs.Reset(LedCount * VerticesPerLed);
	CachedTangents.Reset(LedCount * VerticesPerLed);
	CachedVertexColors.Reset(LedCount * VerticesPerLed);
	CachedTriangles.Reset(LedCount * CircleSegments * 6);

	const float HalfColumns = static_cast<float>(MatrixColumns - 1) * 0.5f;
	const float HalfRows = static_cast<float>(MatrixRows - 1) * 0.5f;

	for (int32 Row = 0; Row < MatrixRows; ++Row)
	{
		for (int32 Column = 0; Column < MatrixColumns; ++Column)
		{
			const int32 LedIndex = Row * MatrixColumns + Column;
			const int32 CenterVertexIndex = CachedVertices.Num();
			const float CenterY = (static_cast<float>(Column) - HalfColumns) * LedPitch;
			const float CenterZ = (HalfRows - static_cast<float>(Row)) * LedPitch;
			const FLinearColor VertexColor = ResolveLedVertexColor(LedStates.IsValidIndex(LedIndex) && LedStates[LedIndex] != 0);
			FVector Center = FVector::ZeroVector;
			FVector CenterNormal = FVector::ForwardVector;
			FProcMeshTangent CenterTangent(0.0f, 1.0f, 0.0f);
			BuildCurvedLedVertex(CenterY, CenterZ, Center, CenterNormal, CenterTangent);

			CachedVertices.Add(Center);
			CachedNormals.Add(CenterNormal);
			CachedUVs.Add(FVector2D(0.5f, 0.5f));
			CachedTangents.Add(CenterTangent);
			CachedVertexColors.Add(VertexColor);

			for (int32 SegmentIndex = 0; SegmentIndex < CircleSegments; ++SegmentIndex)
			{
				const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / static_cast<float>(CircleSegments);
				const float OffsetY = FMath::Cos(Angle) * LedRadius;
				const float OffsetZ = FMath::Sin(Angle) * LedRadius;
				FVector RingVertex = FVector::ZeroVector;
				FVector RingNormal = FVector::ForwardVector;
				FProcMeshTangent RingTangent(0.0f, 1.0f, 0.0f);
				BuildCurvedLedVertex(CenterY + OffsetY, CenterZ + OffsetZ, RingVertex, RingNormal, RingTangent);

				CachedVertices.Add(RingVertex);
				CachedNormals.Add(RingNormal);
				CachedUVs.Add(FVector2D(0.5f + FMath::Cos(Angle) * 0.5f, 0.5f + FMath::Sin(Angle) * 0.5f));
				CachedTangents.Add(RingTangent);
				CachedVertexColors.Add(VertexColor);
			}

			for (int32 SegmentIndex = 0; SegmentIndex < CircleSegments; ++SegmentIndex)
			{
				const int32 CurrentRingVertex = CenterVertexIndex + 1 + SegmentIndex;
				const int32 NextRingVertex = CenterVertexIndex + 1 + ((SegmentIndex + 1) % CircleSegments);
				CachedTriangles.Add(CenterVertexIndex);
				CachedTriangles.Add(CurrentRingVertex);
				CachedTriangles.Add(NextRingVertex);
				CachedTriangles.Add(CenterVertexIndex);
				CachedTriangles.Add(NextRingVertex);
				CachedTriangles.Add(CurrentRingVertex);
			}
		}
	}

	ClearAllMeshSections();
	CreateMeshSection_LinearColor(
		0,
		CachedVertices,
		CachedTriangles,
		CachedNormals,
		CachedUVs,
		CachedVertexColors,
		CachedTangents,
		false);

	bMeshBuilt = true;
	ApplyLedMaterial();
	RefreshVertexColors();
}

bool UTunaSweeperLedExpressionComponent::SetExpressionByName(FName ExpressionName)
{
	if (ExpressionName.IsNone())
	{
		return false;
	}

	const FString* Pattern = ExpressionPresets.Find(ExpressionName);
	if (!Pattern)
	{
		RefreshExpressionPresets();
		Pattern = ExpressionPresets.Find(ExpressionName);
		if (!Pattern)
		{
			return false;
		}
	}

	if (!ApplyPatternToStates(*Pattern))
	{
		return false;
	}

	CurrentExpressionName = ExpressionName;
	return true;
}

void UTunaSweeperLedExpressionComponent::SetHorizontalCurvatureDegrees(float InHorizontalCurvatureDegrees)
{
	HorizontalCurvatureDegrees = FMath::Clamp(InHorizontalCurvatureDegrees, 0.0f, 160.0f);
	RebuildLedMesh();
	if (!CurrentExpressionName.IsNone())
	{
		SetExpressionByName(CurrentExpressionName);
	}
}

bool UTunaSweeperLedExpressionComponent::SetExpressionPattern(FName ExpressionName, const FString& Pattern)
{
	if (ExpressionName.IsNone())
	{
		return false;
	}

	const FString NormalizedPattern = NormalizePattern(Pattern);
	if (NormalizedPattern.IsEmpty())
	{
		return false;
	}

	ExpressionPresets.Add(ExpressionName, NormalizedPattern);
	if (CurrentExpressionName == ExpressionName)
	{
		ApplyPatternToStates(NormalizedPattern);
	}
	return true;
}

void UTunaSweeperLedExpressionComponent::AddOrUpdateBlueprintPreset(FName ExpressionName, const FString& Pattern)
{
	if (ExpressionName.IsNone())
	{
		return;
	}

	bool bUpdated = false;
	for (FTunaSweeperLedExpressionPreset& BlueprintPreset : BlueprintPresets)
	{
		if (BlueprintPreset.ExpressionName == ExpressionName)
		{
			BlueprintPreset.Pattern = Pattern;
			bUpdated = true;
			break;
		}
	}

	if (!bUpdated)
	{
		FTunaSweeperLedExpressionPreset NewPreset;
		NewPreset.ExpressionName = ExpressionName;
		NewPreset.Pattern = Pattern;
		BlueprintPresets.Add(NewPreset);
	}

	SetExpressionPattern(ExpressionName, Pattern);
}

void UTunaSweeperLedExpressionComponent::ConfigureExpressionSource(
	const FString& InPresetFilePath,
	FName InDefaultExpressionName)
{
	if (!InPresetFilePath.TrimStartAndEnd().IsEmpty())
	{
		PresetFilePath = InPresetFilePath.TrimStartAndEnd();
	}
	if (!InDefaultExpressionName.IsNone())
	{
		DefaultExpressionName = InDefaultExpressionName;
		CurrentExpressionName = InDefaultExpressionName;
	}

	RefreshExpressionPresets();
	SetExpressionByName(DefaultExpressionName);
}

void UTunaSweeperLedExpressionComponent::ConfigureLedAppearance(
	FLinearColor InLedColor,
	FLinearColor InOffColor,
	float InLedPitch,
	float InLedRadius)
{
	LedColor = InLedColor;
	OffColor = InOffColor;
	LedPitch = FMath::Max(0.1f, InLedPitch);
	LedRadius = FMath::Max(0.01f, InLedRadius);

	ApplyLedMaterial();
	RebuildLedMesh();
	if (!CurrentExpressionName.IsNone())
	{
		SetExpressionByName(CurrentExpressionName);
	}
}

bool UTunaSweeperLedExpressionComponent::DoesExpressionExist(FName ExpressionName) const
{
	return ExpressionPresets.Contains(ExpressionName);
}

FString UTunaSweeperLedExpressionComponent::ResolvePresetFilePath() const
{
	const FString TrimmedPresetFilePath = PresetFilePath.TrimStartAndEnd();
	if (TrimmedPresetFilePath.IsEmpty())
	{
		return FString();
	}

	if (FPaths::IsRelative(TrimmedPresetFilePath))
	{
		return FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectContentDir(), TrimmedPresetFilePath));
	}

	return TrimmedPresetFilePath;
}

bool UTunaSweeperLedExpressionComponent::LoadPresetFileIntoMap()
{
	const FString ResolvedPath = ResolvePresetFilePath();
	if (ResolvedPath.IsEmpty())
	{
		return false;
	}

	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *ResolvedPath))
	{
		UE_LOG(LogTunaSweeperLedExpression, Warning, TEXT("Failed to read LED expression preset file: %s"), *ResolvedPath);
		return false;
	}

	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines, false);

	bool bLoadedAnyPreset = false;
	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString TrimmedLine = Lines[LineIndex].TrimStartAndEnd();
		if (TrimmedLine.IsEmpty() || TrimmedLine.StartsWith(TEXT("//")) || TrimmedLine.StartsWith(TEXT(";")))
		{
			continue;
		}

		FString ExpressionNameText;
		FString PatternText;
		if (!TrimmedLine.Split(TEXT("="), &ExpressionNameText, &PatternText) &&
			!TrimmedLine.Split(TEXT("|"), &ExpressionNameText, &PatternText))
		{
			UE_LOG(
				LogTunaSweeperLedExpression,
				Warning,
				TEXT("Skipping LED expression preset line %d: missing '=' or '|' separator."),
				LineIndex + 1);
			continue;
		}

		ExpressionNameText = ExpressionNameText.TrimStartAndEnd();
		PatternText = PatternText.TrimStartAndEnd();
		if (ExpressionNameText.IsEmpty() || PatternText.IsEmpty())
		{
			continue;
		}

		const FString NormalizedPattern = NormalizePattern(PatternText);
		const int32 ExpectedPatternLength = GetLedCount();
		if (NormalizedPattern.Len() != ExpectedPatternLength)
		{
			UE_LOG(
				LogTunaSweeperLedExpression,
				Warning,
				TEXT("LED expression preset '%s' has %d cells; expected %d. Missing cells are treated as off and extra cells are ignored."),
				*ExpressionNameText,
				NormalizedPattern.Len(),
				ExpectedPatternLength);
		}

		ExpressionPresets.Add(FName(*ExpressionNameText), NormalizedPattern);
		bLoadedAnyPreset = true;
	}

	return bLoadedAnyPreset;
}

FString UTunaSweeperLedExpressionComponent::NormalizePattern(const FString& RawPattern) const
{
	FString NormalizedPattern;
	NormalizedPattern.Reserve(RawPattern.Len());
	for (int32 CharacterIndex = 0; CharacterIndex < RawPattern.Len(); ++CharacterIndex)
	{
		const TCHAR Character = RawPattern[CharacterIndex];
		if (Character == TEXT('\r') || Character == TEXT('\n') || Character == TEXT('\t'))
		{
			continue;
		}

		NormalizedPattern.AppendChar(Character);
	}

	return NormalizedPattern;
}

bool UTunaSweeperLedExpressionComponent::ApplyPatternToStates(const FString& Pattern)
{
	const int32 LedCount = GetLedCount();
	if (LedCount <= 0)
	{
		return false;
	}

	LedStates.SetNumZeroed(LedCount);
	const int32 PatternLength = Pattern.Len();
	for (int32 LedIndex = 0; LedIndex < LedCount; ++LedIndex)
	{
		LedStates[LedIndex] = (LedIndex < PatternLength && IsOnCharacter(Pattern[LedIndex])) ? 1 : 0;
	}

	RefreshVertexColors();
	return true;
}

void UTunaSweeperLedExpressionComponent::RefreshVertexColors()
{
	if (!bMeshBuilt || CachedVertexColors.IsEmpty())
	{
		return;
	}

	const int32 VerticesPerLed = CircleSegments + 1;
	for (int32 LedIndex = 0; LedIndex < LedStates.Num(); ++LedIndex)
	{
		const FLinearColor VertexColor = ResolveLedVertexColor(LedStates[LedIndex] != 0);
		const int32 VertexStart = LedIndex * VerticesPerLed;
		for (int32 VertexOffset = 0; VertexOffset < VerticesPerLed; ++VertexOffset)
		{
			const int32 VertexIndex = VertexStart + VertexOffset;
			if (CachedVertexColors.IsValidIndex(VertexIndex))
			{
				CachedVertexColors[VertexIndex] = VertexColor;
			}
		}
	}

	UpdateMeshSection_LinearColor(
		0,
		CachedVertices,
		CachedNormals,
		CachedUVs,
		CachedVertexColors,
		CachedTangents);
}

void UTunaSweeperLedExpressionComponent::ApplyLedMaterial()
{
	UMaterialInterface* LoadedMaterial = LedMaterial.IsNull() ? nullptr : LedMaterial.LoadSynchronous();
	if (!LoadedMaterial)
	{
		return;
	}

	DynamicLedMaterial = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
	if (!DynamicLedMaterial)
	{
		SetMaterial(0, LoadedMaterial);
		return;
	}

	DynamicLedMaterial->SetVectorParameterValue(TEXT("LedColor"), LedColor);
	DynamicLedMaterial->SetVectorParameterValue(TEXT("OffColor"), OffColor);
	DynamicLedMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), LedColor);
	DynamicLedMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), EmissiveIntensity);
	DynamicLedMaterial->SetScalarParameterValue(TEXT("Intensity"), EmissiveIntensity);
	SetMaterial(0, DynamicLedMaterial);
}

void UTunaSweeperLedExpressionComponent::BuildCurvedLedVertex(
	float SourceY,
	float SourceZ,
	FVector& OutPosition,
	FVector& OutNormal,
	FProcMeshTangent& OutTangent) const
{
	const float ClampedCurvatureDegrees = FMath::Clamp(HorizontalCurvatureDegrees, 0.0f, 160.0f);
	if (ClampedCurvatureDegrees <= KINDA_SMALL_NUMBER)
	{
		OutPosition = FVector(0.0f, SourceY, SourceZ);
		OutNormal = FVector::ForwardVector;
		OutTangent = FProcMeshTangent(0.0f, 1.0f, 0.0f);
		return;
	}

	const float MatrixWidth = FMath::Max(
		LedPitch,
		static_cast<float>(FMath::Max(1, MatrixColumns - 1)) * LedPitch);
	const float ArcRadians = FMath::DegreesToRadians(ClampedCurvatureDegrees);
	const float Radius = MatrixWidth / FMath::Max(ArcRadians, KINDA_SMALL_NUMBER);
	const float Theta = SourceY / Radius;
	const float SinTheta = FMath::Sin(Theta);
	const float CosTheta = FMath::Cos(Theta);

	OutPosition = FVector(
		Radius * (CosTheta - 1.0f),
		Radius * SinTheta,
		SourceZ);
	OutNormal = FVector(CosTheta, SinTheta, 0.0f).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	OutTangent = FProcMeshTangent(-SinTheta, CosTheta, 0.0f);
}

FLinearColor UTunaSweeperLedExpressionComponent::ResolveLedVertexColor(bool bEnabled) const
{
	return bEnabled ? LedColor : OffColor;
}

bool UTunaSweeperLedExpressionComponent::IsOnCharacter(TCHAR Character) const
{
	int32 UnusedIndex = INDEX_NONE;
	if (OffCharacters.FindChar(Character, UnusedIndex))
	{
		return false;
	}

	return OnCharacters.FindChar(Character, UnusedIndex);
}
