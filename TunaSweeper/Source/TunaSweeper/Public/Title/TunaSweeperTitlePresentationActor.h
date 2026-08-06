#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperTitlePresentationActor.generated.h"

class UCameraComponent;
class UPointLightComponent;
class USceneComponent;
class USkyLightComponent;
class UStaticMeshComponent;

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperTitleSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Title|Look")
	void SetDirectHeadLookRotation(float YawDegrees, float PitchDegrees);

	virtual void FinalizeBoneTransform() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look")
	FName HeadBoneName = TEXT("Head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look")
	bool bApplyDirectHeadLook = true;

private:
	bool IsBoneDescendantOf(int32 BoneIndex, int32 ParentBoneIndex) const;
	void ApplyDirectHeadLookToEditablePose();

	float DirectHeadLookYaw = 0.0f;
	float DirectHeadLookPitch = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperTitlePresentationActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperTitlePresentationActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Title")
	void SetMainMenuPresentationActive(bool bActive);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TunaSweeper|Title")
	void ApplyRecommendedPresentationLayout();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Title")
	bool IsMainMenuPresentationActive() const { return bMainMenuPresentationActive; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Title|Face")
	void SetFacialWeight(FName MorphTargetName, float Weight);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Title|Face")
	void ClearFacialWeights();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Title|Look")
	float GetHeadLookYaw() const { return CurrentHeadLookYaw; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Title|Look")
	float GetHeadLookPitch() const { return CurrentHeadLookPitch; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Title|Look")
	FVector GetHeadLookTargetLocation() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "TunaSweeper|Title|Look", meta = (DisplayName = "On Title Head Look Updated"))
	void ReceiveHeadLookUpdated(float YawDegrees, float PitchDegrees, FVector WorldTargetLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UCameraComponent> TitleCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USceneComponent> CharacterAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UTunaSweeperTitleSkeletalMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USkeletalMeshComponent> FaceMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USceneComponent> HeadLookTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> BackWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> LeftWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> RightWall;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UStaticMeshComponent> Floor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<USkyLightComponent> AmbientLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UPointLightComponent> CharacterKeyLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Title|Components")
	TObjectPtr<UPointLightComponent> EmptyWallLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Character")
	FName FaceAttachmentSocketName = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Character")
	FVector CharacterRelativeLocation = FVector(120.0f, 210.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Character")
	FRotator CharacterRelativeRotation = FRotator(0.0f, 105.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera")
	FVector MainMenuCameraLocation = FVector(-500.0f, 0.0f, 155.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera")
	FRotator MainMenuCameraRotation = FRotator(-3.2f, 16.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera")
	FVector SubMenuCameraLocation = FVector(-500.0f, 0.0f, 155.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera")
	FRotator SubMenuCameraRotation = FRotator(-1.0f, -38.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float CameraBlendSpeed = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Camera", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float CameraFieldOfView = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxHeadLookYaw = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxHeadLookPitch = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float HeadLookInterpolationSpeed = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Title|Look", meta = (ClampMin = "100.0", UIMin = "100.0"))
	float HeadLookTargetDistance = 1000.0f;

private:
	void ApplyDesignTransforms();
	void UpdateCamera(float DeltaSeconds);
	void UpdateCursorLook(float DeltaSeconds);
	void UpdateCharacterPresentationState();
	void SetCharacterPresentationEnabled(bool bEnabled);
	void EnsureTitleCameraViewTarget();

	bool bMainMenuPresentationActive = true;
	bool bCharacterPresentationEnabled = true;
	float CurrentHeadLookYaw = 0.0f;
	float CurrentHeadLookPitch = 0.0f;
};
