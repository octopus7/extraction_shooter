#pragma once

#include "Components/Widget.h"
#include "TunaSweeperCheckIndicatorWidget.generated.h"

class STunaSweeperCheckIndicator;

/** A font-independent check mark. The owning button supplies input and its localized label. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperCheckIndicatorWidget : public UWidget
{

	GENERATED_BODY()

public:
	void SetChecked(bool bInChecked);
	bool IsChecked() const { return bChecked; }
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	UPROPERTY(Transient)
	bool bChecked = false;
	TSharedPtr<STunaSweeperCheckIndicator> Indicator;
};
