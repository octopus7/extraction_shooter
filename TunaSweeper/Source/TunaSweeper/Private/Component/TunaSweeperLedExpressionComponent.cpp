#include "Component/TunaSweeperLedExpressionComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperLedExpression, Log, All);

namespace TunaSweeperLedExpression
{
	const TCHAR* DefaultLedMaterialPath = TEXT("/Game/Effects/M_LedExpression_VertexColorEmissive.M_LedExpression_VertexColorEmissive");
	const TCHAR* LegacyLedMaterialPath = TEXT("/Game/Effects/M_LumberjackMeleeSwingArc.M_LumberjackMeleeSwingArc");
}

UTunaSweeperLedExpressionComponent::UTunaSweeperLedExpressionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bUseAsyncCooking = true;

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCastShadow(false);

	LedMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TunaSweeperLedExpression::DefaultLedMaterialPath));
	OffLedMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor")));
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

	bool bExpressionApplied = !DefaultExpressionName.IsNone() && SetExpressionByName(DefaultExpressionName);
	if (!bExpressionApplied && !ExpressionPresets.IsEmpty())
	{
		for (const FName& PresetName : ExpressionPresetOrder)
		{
			if (SetExpressionByName(PresetName))
			{
				bExpressionApplied = true;
				break;
			}
		}
	}

	RefreshDemoTickEnabled();
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

	DemoExpressionElapsedSeconds = 0.0f;
	RefreshDemoTickEnabled();
}

void UTunaSweeperLedExpressionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDemoModeEnabled || DeltaTime <= 0.0f)
	{
		return;
	}

	DemoExpressionElapsedSeconds += DeltaTime;
	const float IntervalSeconds = FMath::Max(0.1f, DemoExpressionIntervalSeconds);
	if (DemoExpressionElapsedSeconds < IntervalSeconds)
	{
		return;
	}

	DemoExpressionElapsedSeconds = 0.0f;
	AdvanceDemoExpression();
}

#if WITH_EDITOR
void UTunaSweeperLedExpressionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	RefreshEditorPreviewAfterPropertyChange(PropertyChangedEvent);
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UTunaSweeperLedExpressionComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	RefreshEditorPreviewAfterPropertyChange(PropertyChangedEvent);
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UTunaSweeperLedExpressionComponent::RefreshEditorPreviewAfterPropertyChange(const FPropertyChangedEvent& PropertyChangedEvent)
{
	DemoExpressionIntervalSeconds = FMath::Max(0.1f, DemoExpressionIntervalSeconds);
	ApplyLedMaterial();
	RefreshExpressionPresets();
	RebuildLedMesh();
	SetExpressionByName(CurrentExpressionName.IsNone() ? DefaultExpressionName : CurrentExpressionName);
	RefreshDemoTickEnabled();

	MarkRenderDynamicDataDirty();
	MarkRenderStateDirty();
	UpdateBounds();
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
	ExpressionPresetOrder.Reset();
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
			AddExpressionPresetToCache(BlueprintPreset.ExpressionName, NormalizedPattern);
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
	HorizontalCurvatureDegrees = FMath::Clamp(HorizontalCurvatureDegrees, 0.0f, 240.0f);
	ActiveLedSurfaceOffset = FMath::Max(0.0f, ActiveLedSurfaceOffset);
	RenderLocalRotation.Normalize();

	const int32 LedCount = GetLedCount();
	const int32 VerticesPerLed = CircleSegments + 1;
	if (LedStates.Num() != LedCount)
	{
		LedStates.SetNumZeroed(LedCount);
	}

	TArray<FVector> OffVertices;
	TArray<FVector> OffNormals;
	TArray<FVector2D> OffUVs;
	TArray<FProcMeshTangent> OffTangents;
	TArray<FLinearColor> OffVertexColors;
	TArray<int32> OffTriangles;
	TArray<FVector> ActiveVertices;
	TArray<FVector> ActiveNormals;
	TArray<FVector2D> ActiveUVs;
	TArray<FProcMeshTangent> ActiveTangents;
	TArray<FLinearColor> ActiveVertexColors;
	TArray<int32> ActiveTriangles;

	OffVertices.Reserve(LedCount * VerticesPerLed);
	OffNormals.Reserve(LedCount * VerticesPerLed);
	OffUVs.Reserve(LedCount * VerticesPerLed);
	OffTangents.Reserve(LedCount * VerticesPerLed);
	OffVertexColors.Reserve(LedCount * VerticesPerLed);
	OffTriangles.Reserve(LedCount * CircleSegments * 6);
	ActiveVertices.Reserve(LedCount * VerticesPerLed);
	ActiveNormals.Reserve(LedCount * VerticesPerLed);
	ActiveUVs.Reserve(LedCount * VerticesPerLed);
	ActiveTangents.Reserve(LedCount * VerticesPerLed);
	ActiveVertexColors.Reserve(LedCount * VerticesPerLed);
	ActiveTriangles.Reserve(LedCount * CircleSegments * 6);

	const float HalfColumns = static_cast<float>(MatrixColumns - 1) * 0.5f;
	const float HalfRows = static_cast<float>(MatrixRows - 1) * 0.5f;
	const bool bDrawOffLeds = OffColor.A > KINDA_SMALL_NUMBER;

	auto AddLedDisc = [this](
		float CenterY,
		float CenterZ,
		const FLinearColor& VertexColor,
		float SurfaceOffset,
		TArray<FVector>& Vertices,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FProcMeshTangent>& Tangents,
		TArray<FLinearColor>& VertexColors,
		TArray<int32>& Triangles)
	{
		const int32 CenterVertexIndex = Vertices.Num();
		FVector Center = FVector::ZeroVector;
		FVector CenterNormal = FVector::ForwardVector;
		FProcMeshTangent CenterTangent(0.0f, 1.0f, 0.0f);
		BuildCurvedLedVertex(CenterY, CenterZ, Center, CenterNormal, CenterTangent);
		Center += CenterNormal * SurfaceOffset;
		ApplyRenderLocalTransform(Center, CenterNormal, CenterTangent);

		Vertices.Add(Center);
		Normals.Add(CenterNormal);
		UVs.Add(FVector2D(0.5f, 0.5f));
		Tangents.Add(CenterTangent);
		VertexColors.Add(VertexColor);

		for (int32 SegmentIndex = 0; SegmentIndex < CircleSegments; ++SegmentIndex)
		{
			const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / static_cast<float>(CircleSegments);
			const float OffsetY = FMath::Cos(Angle) * LedRadius;
			const float OffsetZ = FMath::Sin(Angle) * LedRadius;
			FVector RingVertex = FVector::ZeroVector;
			FVector RingNormal = FVector::ForwardVector;
			FProcMeshTangent RingTangent(0.0f, 1.0f, 0.0f);
			BuildCurvedLedVertex(CenterY + OffsetY, CenterZ + OffsetZ, RingVertex, RingNormal, RingTangent);
			RingVertex += RingNormal * SurfaceOffset;
			ApplyRenderLocalTransform(RingVertex, RingNormal, RingTangent);

			Vertices.Add(RingVertex);
			Normals.Add(RingNormal);
			UVs.Add(FVector2D(0.5f + FMath::Cos(Angle) * 0.5f, 0.5f + FMath::Sin(Angle) * 0.5f));
			Tangents.Add(RingTangent);
			VertexColors.Add(VertexColor);
		}

		for (int32 SegmentIndex = 0; SegmentIndex < CircleSegments; ++SegmentIndex)
		{
			const int32 CurrentRingVertex = CenterVertexIndex + 1 + SegmentIndex;
			const int32 NextRingVertex = CenterVertexIndex + 1 + ((SegmentIndex + 1) % CircleSegments);
			Triangles.Add(CenterVertexIndex);
			Triangles.Add(CurrentRingVertex);
			Triangles.Add(NextRingVertex);
			Triangles.Add(CenterVertexIndex);
			Triangles.Add(NextRingVertex);
			Triangles.Add(CurrentRingVertex);
		}
	};

	for (int32 Row = 0; Row < MatrixRows; ++Row)
	{
		for (int32 Column = 0; Column < MatrixColumns; ++Column)
		{
			const int32 LedIndex = Row * MatrixColumns + Column;
			const float CenterY = (static_cast<float>(Column) - HalfColumns) * LedPitch;
			const float CenterZ = (HalfRows - static_cast<float>(Row)) * LedPitch;
			if (bDrawOffLeds)
			{
				AddLedDisc(CenterY, CenterZ, OffColor, 0.0f, OffVertices, OffNormals, OffUVs, OffTangents, OffVertexColors, OffTriangles);
			}

			if (LedStates.IsValidIndex(LedIndex) && LedStates[LedIndex] != 0)
			{
				AddLedDisc(CenterY, CenterZ, LedColor, ActiveLedSurfaceOffset, ActiveVertices, ActiveNormals, ActiveUVs, ActiveTangents, ActiveVertexColors, ActiveTriangles);
			}
		}
	}

	ClearAllMeshSections();
	if (bDrawOffLeds)
	{
		CreateMeshSection_LinearColor(
			0,
			OffVertices,
			OffTriangles,
			OffNormals,
			OffUVs,
			OffVertexColors,
			OffTangents,
			false);
	}
	if (!ActiveVertices.IsEmpty())
	{
		CreateMeshSection_LinearColor(
			1,
			ActiveVertices,
			ActiveTriangles,
			ActiveNormals,
			ActiveUVs,
			ActiveVertexColors,
			ActiveTangents,
			false);
	}

	bMeshBuilt = true;
	ApplyLedMaterial();
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
	HorizontalCurvatureDegrees = FMath::Clamp(InHorizontalCurvatureDegrees, 0.0f, 240.0f);
	RebuildLedMesh();
	if (!CurrentExpressionName.IsNone())
	{
		SetExpressionByName(CurrentExpressionName);
	}
}

void UTunaSweeperLedExpressionComponent::SetRenderLocalTransform(FVector InRenderLocalLocation, FRotator InRenderLocalRotation)
{
	RenderLocalLocation = InRenderLocalLocation;
	RenderLocalRotation = InRenderLocalRotation.GetNormalized();
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

	AddExpressionPresetToCache(ExpressionName, NormalizedPattern);
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

void UTunaSweeperLedExpressionComponent::SetDemoModeEnabled(bool bEnabled)
{
	bDemoModeEnabled = bEnabled;
	DemoExpressionElapsedSeconds = 0.0f;
	RefreshDemoTickEnabled();
}

void UTunaSweeperLedExpressionComponent::SetDemoExpressionIntervalSeconds(float InIntervalSeconds)
{
	DemoExpressionIntervalSeconds = FMath::Max(0.1f, InIntervalSeconds);
	DemoExpressionElapsedSeconds = 0.0f;
	RefreshDemoTickEnabled();
}

bool UTunaSweeperLedExpressionComponent::AdvanceDemoExpression()
{
	if (ExpressionPresetOrder.IsEmpty())
	{
		RefreshExpressionPresets();
	}

	const int32 PresetCount = ExpressionPresetOrder.Num();
	if (PresetCount <= 0)
	{
		return false;
	}

	const int32 CurrentIndex = FindExpressionPresetOrderIndex(CurrentExpressionName);
	const int32 StartIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % PresetCount;
	for (int32 AttemptIndex = 0; AttemptIndex < PresetCount; ++AttemptIndex)
	{
		const int32 CandidateIndex = (StartIndex + AttemptIndex) % PresetCount;
		if (SetExpressionByName(ExpressionPresetOrder[CandidateIndex]))
		{
			return true;
		}
	}

	return false;
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

		AddExpressionPresetToCache(FName(*ExpressionNameText), NormalizedPattern);
		bLoadedAnyPreset = true;
	}

	return bLoadedAnyPreset;
}

void UTunaSweeperLedExpressionComponent::AddExpressionPresetToCache(FName ExpressionName, const FString& NormalizedPattern)
{
	if (ExpressionName.IsNone() || NormalizedPattern.IsEmpty())
	{
		return;
	}

	if (!ExpressionPresets.Contains(ExpressionName))
	{
		ExpressionPresetOrder.Add(ExpressionName);
	}
	ExpressionPresets.Add(ExpressionName, NormalizedPattern);
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
	RebuildLedMesh();
}

void UTunaSweeperLedExpressionComponent::ApplyLedMaterial()
{
	if (LedMaterial.ToSoftObjectPath().ToString().Equals(TunaSweeperLedExpression::LegacyLedMaterialPath, ESearchCase::IgnoreCase))
	{
		LedMaterial = TSoftObjectPtr<UMaterialInterface>(
			FSoftObjectPath(TunaSweeperLedExpression::DefaultLedMaterialPath));
	}

	if (!OffLedMaterial.IsNull())
	{
		if (UMaterialInterface* LoadedOffMaterial = OffLedMaterial.LoadSynchronous())
		{
			SetMaterial(0, LoadedOffMaterial);
		}
	}

	UMaterialInterface* LoadedMaterial = LedMaterial.IsNull() ? nullptr : LedMaterial.LoadSynchronous();
	if (!LoadedMaterial)
	{
		return;
	}

	DynamicLedMaterial = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
	if (!DynamicLedMaterial)
	{
		SetMaterial(1, LoadedMaterial);
		return;
	}

	DynamicLedMaterial->SetVectorParameterValue(TEXT("LedColor"), LedColor);
	DynamicLedMaterial->SetVectorParameterValue(TEXT("OffColor"), OffColor);
	DynamicLedMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), LedColor);
	DynamicLedMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), EmissiveIntensity);
	DynamicLedMaterial->SetScalarParameterValue(TEXT("Intensity"), EmissiveIntensity);
	SetMaterial(1, DynamicLedMaterial);
}

void UTunaSweeperLedExpressionComponent::BuildCurvedLedVertex(
	float SourceY,
	float SourceZ,
	FVector& OutPosition,
	FVector& OutNormal,
	FProcMeshTangent& OutTangent) const
{
	const float ClampedCurvatureDegrees = FMath::Clamp(HorizontalCurvatureDegrees, 0.0f, 240.0f);
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

void UTunaSweeperLedExpressionComponent::ApplyRenderLocalTransform(
	FVector& InOutPosition,
	FVector& InOutNormal,
	FProcMeshTangent& InOutTangent) const
{
	const FQuat RenderLocalQuat = RenderLocalRotation.Quaternion();
	InOutPosition = RenderLocalQuat.RotateVector(InOutPosition) + RenderLocalLocation;
	InOutNormal = RenderLocalQuat.RotateVector(InOutNormal).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::ForwardVector);
	const FVector TangentX = RenderLocalQuat.RotateVector(InOutTangent.TangentX).GetSafeNormal(KINDA_SMALL_NUMBER, FVector::RightVector);
	InOutTangent = FProcMeshTangent(TangentX, InOutTangent.bFlipTangentY);
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

void UTunaSweeperLedExpressionComponent::RefreshDemoTickEnabled()
{
	SetComponentTickEnabled(bDemoModeEnabled);
}

int32 UTunaSweeperLedExpressionComponent::FindExpressionPresetOrderIndex(FName ExpressionName) const
{
	for (int32 PresetIndex = 0; PresetIndex < ExpressionPresetOrder.Num(); ++PresetIndex)
	{
		if (ExpressionPresetOrder[PresetIndex] == ExpressionName)
		{
			return PresetIndex;
		}
	}

	return INDEX_NONE;
}
