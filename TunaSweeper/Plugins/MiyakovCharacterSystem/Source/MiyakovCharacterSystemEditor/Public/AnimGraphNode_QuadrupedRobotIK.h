#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_QuadrupedRobotIK.h"
#include "CoreMinimal.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "AnimGraphNode_QuadrupedRobotIK.generated.h"

UCLASS()
class MIYAKOVCHARACTERSYSTEMEDITOR_API UAnimGraphNode_QuadrupedRobotIK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FAnimNode_QuadrupedRobotIK Node;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog) override;

protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }

private:
	mutable FNodeTitleTextTable CachedNodeTitles;
};
