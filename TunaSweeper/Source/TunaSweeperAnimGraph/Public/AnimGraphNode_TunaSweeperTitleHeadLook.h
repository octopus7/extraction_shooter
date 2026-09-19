#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "Title/AnimNode_TunaSweeperTitleHeadLook.h"
#include "AnimGraphNode_TunaSweeperTitleHeadLook.generated.h"

UCLASS()
class TUNASWEEPERANIMGRAPH_API UAnimGraphNode_TunaSweeperTitleHeadLook : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_TunaSweeperTitleHeadLook Node;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
};
