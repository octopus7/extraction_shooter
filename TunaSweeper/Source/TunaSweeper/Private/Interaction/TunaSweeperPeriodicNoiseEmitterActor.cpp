#include "Interaction/TunaSweeperPeriodicNoiseEmitterActor.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperNoiseEmitter, Log, All);

namespace
{
	const TCHAR* DefaultMeshMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	struct FNoiseEmitterMeshPart
	{
		FString Type;
		FString MaterialId;
		FVector Center = FVector::ZeroVector;
		FVector Extent = FVector(20.0f, 20.0f, 20.0f);
		FVector BaseCenter = FVector::ZeroVector;
		FVector Axis = FVector::ForwardVector;
		float Length = 100.0f;
		float InnerRadius = 18.0f;
		float OuterRadius = 52.0f;
		int32 Sides = 8;
		bool bCapBack = true;
		bool bCapFront = false;
	};

	struct FNoiseEmitterMeshDefinition
	{
		FString BaseMaterialPath = DefaultMeshMaterialPath;
		TMap<FString, FLinearColor> MaterialColors;
		TArray<FNoiseEmitterMeshPart> Parts;
	};

	bool TryReadVectorValue(const TSharedPtr<FJsonValue>& JsonValue, FVector& OutVector)
	{
		if (!JsonValue.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* VectorArray = nullptr;
		if (!JsonValue->TryGetArray(VectorArray) || !VectorArray || VectorArray->Num() < 3)
		{
			return false;
		}

		OutVector = FVector(
			static_cast<float>((*VectorArray)[0]->AsNumber()),
			static_cast<float>((*VectorArray)[1]->AsNumber()),
			static_cast<float>((*VectorArray)[2]->AsNumber()));
		return true;
	}

	bool TryReadVectorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FVector& OutVector)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		const TSharedPtr<FJsonValue>* VectorValue = JsonObject->Values.Find(FieldName);
		return VectorValue && TryReadVectorValue(*VectorValue, OutVector);
	}

	bool TryReadColorValue(const TSharedPtr<FJsonValue>& JsonValue, FLinearColor& OutColor)
	{
		if (!JsonValue.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* ColorArray = nullptr;
		if (!JsonValue->TryGetArray(ColorArray) || !ColorArray || ColorArray->Num() < 3)
		{
			return false;
		}

		OutColor = FLinearColor(
			static_cast<float>((*ColorArray)[0]->AsNumber()),
			static_cast<float>((*ColorArray)[1]->AsNumber()),
			static_cast<float>((*ColorArray)[2]->AsNumber()),
			ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f);
		return true;
	}

	void AddQuad(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FLinearColor& Color)
	{
		const int32 BaseIndex = Vertices.Num();
		FVector Normal = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = FVector::UpVector;
		}

		FVector TangentVector = (B - A).GetSafeNormal();
		if (TangentVector.IsNearlyZero())
		{
			TangentVector = FVector::ForwardVector;
		}

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Vertices.Add(D);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Normals.Add(Normal);
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(TangentVector, false));
		}
		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));

		Triangles.Append({ BaseIndex, BaseIndex + 2, BaseIndex + 1, BaseIndex, BaseIndex + 3, BaseIndex + 2 });
		Triangles.Append({ BaseIndex, BaseIndex + 1, BaseIndex + 2, BaseIndex, BaseIndex + 2, BaseIndex + 3 });
	}

	void AddTriangle(
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FLinearColor& Color)
	{
		const int32 BaseIndex = Vertices.Num();
		FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = FVector::UpVector;
		}

		FVector TangentVector = (B - A).GetSafeNormal();
		if (TangentVector.IsNearlyZero())
		{
			TangentVector = FVector::ForwardVector;
		}

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Normals.Add(Normal);
			VertexColors.Add(Color);
			Tangents.Add(FProcMeshTangent(TangentVector, false));
		}
		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		UVs.Add(FVector2D(0.5f, 1.0f));

		Triangles.Append({ BaseIndex, BaseIndex + 2, BaseIndex + 1 });
		Triangles.Append({ BaseIndex, BaseIndex + 1, BaseIndex + 2 });
	}

	void BuildBoxPart(
		const FNoiseEmitterMeshPart& Part,
		const FLinearColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents)
	{
		const FVector E(
			FMath::Max(0.1f, Part.Extent.X),
			FMath::Max(0.1f, Part.Extent.Y),
			FMath::Max(0.1f, Part.Extent.Z));
		const FVector C = Part.Center;
		const FVector P000 = C + FVector(-E.X, -E.Y, -E.Z);
		const FVector P100 = C + FVector(E.X, -E.Y, -E.Z);
		const FVector P110 = C + FVector(E.X, E.Y, -E.Z);
		const FVector P010 = C + FVector(-E.X, E.Y, -E.Z);
		const FVector P001 = C + FVector(-E.X, -E.Y, E.Z);
		const FVector P101 = C + FVector(E.X, -E.Y, E.Z);
		const FVector P111 = C + FVector(E.X, E.Y, E.Z);
		const FVector P011 = C + FVector(-E.X, E.Y, E.Z);

		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P000, P100, P110, P010, Color);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P001, P011, P111, P101, Color);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P000, P001, P101, P100, Color);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P100, P101, P111, P110, Color);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P110, P111, P011, P010, Color);
		AddQuad(Vertices, Triangles, Normals, UVs, VertexColors, Tangents, P010, P011, P001, P000, Color);
	}

	void BuildHornPart(
		const FNoiseEmitterMeshPart& Part,
		const FLinearColor& Color,
		TArray<FVector>& Vertices,
		TArray<int32>& Triangles,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& VertexColors,
		TArray<FProcMeshTangent>& Tangents)
	{
		FVector Axis = Part.Axis.GetSafeNormal();
		if (Axis.IsNearlyZero())
		{
			Axis = FVector::ForwardVector;
		}

		FVector BasisA = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal();
		if (BasisA.IsNearlyZero())
		{
			BasisA = FVector::CrossProduct(FVector::RightVector, Axis).GetSafeNormal();
		}
		FVector BasisB = FVector::CrossProduct(Axis, BasisA).GetSafeNormal();
		if (BasisB.IsNearlyZero())
		{
			BasisB = FVector::UpVector;
		}

		const int32 Sides = FMath::Clamp(Part.Sides, 4, 16);
		const float Length = FMath::Max(1.0f, Part.Length);
		const float InnerRadius = FMath::Max(0.1f, Part.InnerRadius);
		const float OuterRadius = FMath::Max(InnerRadius + 0.1f, Part.OuterRadius);
		const FVector MouthCenter = Part.BaseCenter + Axis * Length;
		TArray<FVector> BaseRing;
		TArray<FVector> MouthRing;
		BaseRing.Reserve(Sides);
		MouthRing.Reserve(Sides);

		for (int32 SideIndex = 0; SideIndex < Sides; ++SideIndex)
		{
			const float Angle = (static_cast<float>(SideIndex) / static_cast<float>(Sides)) * 2.0f * PI;
			const FVector Radial = BasisA * FMath::Cos(Angle) + BasisB * FMath::Sin(Angle);
			BaseRing.Add(Part.BaseCenter + Radial * InnerRadius);
			MouthRing.Add(MouthCenter + Radial * OuterRadius);
		}

		for (int32 SideIndex = 0; SideIndex < Sides; ++SideIndex)
		{
			const int32 NextIndex = (SideIndex + 1) % Sides;
			AddQuad(
				Vertices,
				Triangles,
				Normals,
				UVs,
				VertexColors,
				Tangents,
				BaseRing[SideIndex],
				BaseRing[NextIndex],
				MouthRing[NextIndex],
				MouthRing[SideIndex],
				Color);
			if (Part.bCapBack)
			{
				AddTriangle(
					Vertices,
					Triangles,
					Normals,
					UVs,
					VertexColors,
					Tangents,
					Part.BaseCenter,
					BaseRing[NextIndex],
					BaseRing[SideIndex],
					Color);
			}
			if (Part.bCapFront)
			{
				AddTriangle(
					Vertices,
					Triangles,
					Normals,
					UVs,
					VertexColors,
					Tangents,
					MouthCenter,
					MouthRing[SideIndex],
					MouthRing[NextIndex],
					Color);
			}
		}
	}

	bool TryReadMeshPart(const TSharedPtr<FJsonObject>& PartObject, FNoiseEmitterMeshPart& OutPart)
	{
		if (!PartObject.IsValid())
		{
			return false;
		}

		PartObject->TryGetStringField(TEXT("type"), OutPart.Type);
		PartObject->TryGetStringField(TEXT("material"), OutPart.MaterialId);
		OutPart.Type = OutPart.Type.TrimStartAndEnd().ToLower();
		OutPart.MaterialId = OutPart.MaterialId.TrimStartAndEnd();
		TryReadVectorField(PartObject, TEXT("center"), OutPart.Center);
		TryReadVectorField(PartObject, TEXT("extent"), OutPart.Extent);
		TryReadVectorField(PartObject, TEXT("base_center"), OutPart.BaseCenter);
		TryReadVectorField(PartObject, TEXT("axis"), OutPart.Axis);

		double NumericLength = OutPart.Length;
		double NumericInnerRadius = OutPart.InnerRadius;
		double NumericOuterRadius = OutPart.OuterRadius;
		double NumericSides = OutPart.Sides;
		PartObject->TryGetNumberField(TEXT("length"), NumericLength);
		PartObject->TryGetNumberField(TEXT("inner_radius"), NumericInnerRadius);
		PartObject->TryGetNumberField(TEXT("outer_radius"), NumericOuterRadius);
		PartObject->TryGetNumberField(TEXT("sides"), NumericSides);
		PartObject->TryGetBoolField(TEXT("cap_back"), OutPart.bCapBack);
		PartObject->TryGetBoolField(TEXT("cap_front"), OutPart.bCapFront);
		OutPart.Length = FMath::Max(1.0f, static_cast<float>(NumericLength));
		OutPart.InnerRadius = FMath::Max(0.1f, static_cast<float>(NumericInnerRadius));
		OutPart.OuterRadius = FMath::Max(OutPart.InnerRadius + 0.1f, static_cast<float>(NumericOuterRadius));
		OutPart.Sides = FMath::Clamp(FMath::RoundToInt(NumericSides), 4, 16);

		return OutPart.Type == TEXT("box") || OutPart.Type == TEXT("horn");
	}
}

ATunaSweeperPeriodicNoiseEmitterActor::ATunaSweeperPeriodicNoiseEmitterActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	ProceduralMesh->SetupAttachment(SceneRoot);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProceduralMesh->SetCastShadow(false);
}

void ATunaSweeperPeriodicNoiseEmitterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bHornPulseActive)
	{
		SetActorTickEnabled(false);
		return;
	}

	HornPulseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float Duration = FMath::Max(0.05f, HornPulseDurationSeconds);
	if (HornPulseElapsedSeconds >= Duration)
	{
		bHornPulseActive = false;
		HornPulseElapsedSeconds = Duration;
		ApplyHornPulse(0.0f);
		SetActorTickEnabled(false);
		return;
	}

	ApplyHornPulse(CalculateHornPulseAmount());
}

void ATunaSweeperPeriodicNoiseEmitterActor::ConfigureNoiseEmitterDefaults(
	FName InMeshDefinitionId,
	const FString& InMeshDefinitionJsonRelativePath,
	float InNoiseIntervalSeconds,
	float InNoiseLoudness,
	float InNoiseMaxRange,
	FName InNoiseTag,
	const FVector& InNoiseSourceLocalOffset,
	bool bInStartEnabled)
{
	if (!InMeshDefinitionId.IsNone())
	{
		MeshDefinitionId = InMeshDefinitionId;
	}
	if (!InMeshDefinitionJsonRelativePath.TrimStartAndEnd().IsEmpty())
	{
		MeshDefinitionJsonRelativePath = InMeshDefinitionJsonRelativePath.TrimStartAndEnd();
	}
	NoiseIntervalSeconds = FMath::Max(0.05f, InNoiseIntervalSeconds);
	NoiseLoudness = FMath::Max(0.0f, InNoiseLoudness);
	NoiseMaxRange = FMath::Max(0.0f, InNoiseMaxRange);
	NoiseTag = InNoiseTag.IsNone() ? NoiseTag : InNoiseTag;
	NoiseSourceLocalOffset = InNoiseSourceLocalOffset;
	bStartEnabled = bInStartEnabled;
}

void ATunaSweeperPeriodicNoiseEmitterActor::BeginPlay()
{
	Super::BeginPlay();

	RebuildProceduralMesh();
	if (bStartEnabled)
	{
		EmitNoise();
		StartNoiseTimer();
	}
}

void ATunaSweeperPeriodicNoiseEmitterActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopNoiseTimer();
	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperPeriodicNoiseEmitterActor::EmitNoise()
{
	UWorld* World = GetWorld();
	if (!World || NoiseLoudness <= 0.0f || NoiseMaxRange <= 0.0f)
	{
		return;
	}

	if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
	{
		NoiseSubsystem->ReportNoiseAtLocation(
			GetActorTransform().TransformPosition(NoiseSourceLocalOffset),
			NoiseLoudness,
			NoiseMaxRange,
			NoiseTag,
			this,
			this);
		TriggerHornPulse();
	}
}

void ATunaSweeperPeriodicNoiseEmitterActor::StartNoiseTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		NoiseTimerHandle,
		this,
		&ATunaSweeperPeriodicNoiseEmitterActor::EmitNoise,
		FMath::Max(0.05f, NoiseIntervalSeconds),
		true);
}

void ATunaSweeperPeriodicNoiseEmitterActor::StopNoiseTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoiseTimerHandle);
	}
}

FString ATunaSweeperPeriodicNoiseEmitterActor::ResolveMeshDefinitionJsonPath() const
{
	const FString TrimmedPath = MeshDefinitionJsonRelativePath.TrimStartAndEnd();
	if (FPaths::IsRelative(TrimmedPath))
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), TrimmedPath);
	}
	return TrimmedPath;
}

void ATunaSweeperPeriodicNoiseEmitterActor::RebuildProceduralMesh()
{
	if (!ProceduralMesh)
	{
		return;
	}

	ProceduralMesh->ClearAllMeshSections();
	DynamicMaterials.Reset();
	RuntimeMeshSections.Reset();
	bHornPulseActive = false;
	HornPulseElapsedSeconds = 0.0f;
	SetActorTickEnabled(false);

	FString JsonContent;
	const FString JsonPath = ResolveMeshDefinitionJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperNoiseEmitter, Warning, TEXT("Failed to read noise emitter mesh JSON: %s"), *JsonPath);
		return;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, RootValue) || !RootValue.IsValid())
	{
		UE_LOG(LogTunaSweeperNoiseEmitter, Warning, TEXT("Failed to parse noise emitter mesh JSON: %s"), *JsonPath);
		return;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* MeshValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		MeshValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			RootObject->TryGetArrayField(TEXT("mesh_definitions"), MeshValues);
		}
	}

	if (!MeshValues)
	{
		UE_LOG(LogTunaSweeperNoiseEmitter, Warning, TEXT("Noise emitter mesh JSON has no mesh_definitions array: %s"), *JsonPath);
		return;
	}

	FNoiseEmitterMeshDefinition MeshDefinition;
	bool bFoundMeshDefinition = false;
	for (const TSharedPtr<FJsonValue>& MeshValue : *MeshValues)
	{
		const TSharedPtr<FJsonObject> MeshObject = MeshValue.IsValid() ? MeshValue->AsObject() : nullptr;
		if (!MeshObject.IsValid())
		{
			continue;
		}

		FString MeshId;
		MeshObject->TryGetStringField(TEXT("id"), MeshId);
		if (FName(*MeshId.TrimStartAndEnd()) != MeshDefinitionId)
		{
			continue;
		}

		MeshObject->TryGetStringField(TEXT("base_material"), MeshDefinition.BaseMaterialPath);
		const TSharedPtr<FJsonObject>* MaterialsObjectPtr = nullptr;
		if (MeshObject->TryGetObjectField(TEXT("materials"), MaterialsObjectPtr) && MaterialsObjectPtr && MaterialsObjectPtr->IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& MaterialPair : (*MaterialsObjectPtr)->Values)
			{
				FLinearColor MaterialColor;
				if (TryReadColorValue(MaterialPair.Value, MaterialColor))
				{
					MeshDefinition.MaterialColors.Add(MaterialPair.Key, MaterialColor);
				}
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* PartValues = nullptr;
		if (MeshObject->TryGetArrayField(TEXT("parts"), PartValues) && PartValues)
		{
			for (const TSharedPtr<FJsonValue>& PartValue : *PartValues)
			{
				const TSharedPtr<FJsonObject> PartObject = PartValue.IsValid() ? PartValue->AsObject() : nullptr;
				FNoiseEmitterMeshPart MeshPart;
				if (TryReadMeshPart(PartObject, MeshPart))
				{
					MeshDefinition.Parts.Add(MeshPart);
				}
			}
		}

		bFoundMeshDefinition = true;
		break;
	}

	if (!bFoundMeshDefinition || MeshDefinition.Parts.IsEmpty())
	{
		UE_LOG(LogTunaSweeperNoiseEmitter, Warning, TEXT("Noise emitter mesh definition not found or empty: %s"), *MeshDefinitionId.ToString());
		return;
	}

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, *MeshDefinition.BaseMaterialPath);
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultMeshMaterialPath);
	}

	for (int32 PartIndex = 0; PartIndex < MeshDefinition.Parts.Num(); ++PartIndex)
	{
		const FNoiseEmitterMeshPart& Part = MeshDefinition.Parts[PartIndex];
		const FLinearColor* FoundColor = MeshDefinition.MaterialColors.Find(Part.MaterialId);
		const FLinearColor PartColor = FoundColor
			? *FoundColor
			: FLinearColor(0.82f, 0.08f, 0.05f, 1.0f);

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;

		if (Part.Type == TEXT("box"))
		{
			BuildBoxPart(Part, PartColor, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		}
		else if (Part.Type == TEXT("horn"))
		{
			BuildHornPart(Part, PartColor, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
		}

		if (Vertices.IsEmpty() || Triangles.IsEmpty())
		{
			continue;
		}

		const int32 SectionIndex = PartIndex;
		ProceduralMesh->CreateMeshSection_LinearColor(
			SectionIndex,
			Vertices,
			Triangles,
			Normals,
			UVs,
			VertexColors,
			Tangents,
			false);

		FRuntimeMeshSection RuntimeSection;
		RuntimeSection.SectionIndex = SectionIndex;
		RuntimeSection.bIsHorn = Part.Type == TEXT("horn");
		RuntimeSection.HornBaseCenter = Part.BaseCenter;
		RuntimeSection.HornAxis = Part.Axis.GetSafeNormal();
		RuntimeSection.HornLength = FMath::Max(1.0f, Part.Length);
		RuntimeSection.BaseColor = PartColor;
		RuntimeSection.BaseVertices = Vertices;
		RuntimeSection.Normals = Normals;
		RuntimeSection.UVs = UVs;
		RuntimeSection.VertexColors = VertexColors;
		RuntimeSection.Tangents = Tangents;

		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (DynamicMaterial)
			{
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), PartColor);
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PartColor);
				DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PartColor);
				RuntimeSection.DynamicMaterialIndex = DynamicMaterials.Num();
				DynamicMaterials.Add(DynamicMaterial);
				ProceduralMesh->SetMaterial(SectionIndex, DynamicMaterial);
			}
		}

		RuntimeMeshSections.Add(MoveTemp(RuntimeSection));
	}
}

void ATunaSweeperPeriodicNoiseEmitterActor::TriggerHornPulse()
{
	if (!ProceduralMesh || RuntimeMeshSections.IsEmpty())
	{
		return;
	}

	bHornPulseActive = true;
	HornPulseElapsedSeconds = 0.0f;
	ApplyHornPulse(CalculateHornPulseAmount());
	SetActorTickEnabled(true);
}

float ATunaSweeperPeriodicNoiseEmitterActor::CalculateHornPulseAmount() const
{
	const float Duration = FMath::Max(0.05f, HornPulseDurationSeconds);
	const float Alpha = FMath::Clamp(HornPulseElapsedSeconds / Duration, 0.0f, 1.0f);
	const float Decay = FMath::Pow(1.0f - Alpha, 2.15f);
	const float ElasticBounce = 1.0f + FMath::Sin(Alpha * PI * 5.0f) * 0.16f;
	return FMath::Clamp(Decay * ElasticBounce, 0.0f, 1.1f);
}

void ATunaSweeperPeriodicNoiseEmitterActor::ApplyHornPulse(float PulseAmount)
{
	if (!ProceduralMesh)
	{
		return;
	}

	const float ClampedPulseAmount = FMath::Clamp(PulseAmount, 0.0f, 1.1f);
	const float LengthScale = FMath::Lerp(1.0f, FMath::Max(1.0f, HornPulseLengthScale), ClampedPulseAmount);
	const float MouthRadiusScale = FMath::Lerp(1.0f, FMath::Max(1.0f, HornPulseMouthRadiusScale), ClampedPulseAmount);
	const float ColorScale = 1.0f + FMath::Max(0.0f, HornPulseColorBoost) * ClampedPulseAmount;

	for (const FRuntimeMeshSection& RuntimeSection : RuntimeMeshSections)
	{
		if (!RuntimeSection.bIsHorn || RuntimeSection.SectionIndex == INDEX_NONE)
		{
			continue;
		}

		FVector Axis = RuntimeSection.HornAxis.GetSafeNormal();
		if (Axis.IsNearlyZero())
		{
			Axis = FVector::ForwardVector;
		}

		TArray<FVector> UpdatedVertices;
		UpdatedVertices.Reserve(RuntimeSection.BaseVertices.Num());
		for (const FVector& BaseVertex : RuntimeSection.BaseVertices)
		{
			const FVector FromBase = BaseVertex - RuntimeSection.HornBaseCenter;
			const float AxialDistance = FMath::Max(0.0f, FVector::DotProduct(FromBase, Axis));
			const float AxialAlpha = FMath::Clamp(AxialDistance / FMath::Max(1.0f, RuntimeSection.HornLength), 0.0f, 1.0f);
			const FVector AxialComponent = Axis * AxialDistance;
			const FVector RadialComponent = FromBase - AxialComponent;
			const float MouthInfluence = FMath::SmoothStep(0.12f, 1.0f, AxialAlpha);

			const FVector UpdatedVertex =
				RuntimeSection.HornBaseCenter +
				AxialComponent * LengthScale +
				RadialComponent * FMath::Lerp(1.0f, MouthRadiusScale, MouthInfluence);
			UpdatedVertices.Add(UpdatedVertex);
		}

		TArray<FLinearColor> UpdatedColors = RuntimeSection.VertexColors;
		for (FLinearColor& VertexColor : UpdatedColors)
		{
			VertexColor.R = FMath::Min(VertexColor.R * ColorScale, 1.0f);
			VertexColor.G = FMath::Min(VertexColor.G * ColorScale, 1.0f);
			VertexColor.B = FMath::Min(VertexColor.B * ColorScale, 1.0f);
		}

		ProceduralMesh->UpdateMeshSection_LinearColor(
			RuntimeSection.SectionIndex,
			UpdatedVertices,
			RuntimeSection.Normals,
			RuntimeSection.UVs,
			UpdatedColors,
			RuntimeSection.Tangents);

		if (DynamicMaterials.IsValidIndex(RuntimeSection.DynamicMaterialIndex))
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = DynamicMaterials[RuntimeSection.DynamicMaterialIndex])
			{
				FLinearColor PulseColor = RuntimeSection.BaseColor;
				PulseColor.R = FMath::Min(PulseColor.R * ColorScale, 1.0f);
				PulseColor.G = FMath::Min(PulseColor.G * ColorScale, 1.0f);
				PulseColor.B = FMath::Min(PulseColor.B * ColorScale, 1.0f);
				DynamicMaterial->SetVectorParameterValue(TEXT("Color"), PulseColor);
				DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PulseColor);
				DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), PulseColor);
			}
		}
	}
}
