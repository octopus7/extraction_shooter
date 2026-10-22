#pragma once
#include "Widgets/SCompoundWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "PixelWorldProfile.h"
class IDetailsView;

class SPixelWorldPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPixelWorldPanel) {} SLATE_END_ARGS()
    void Construct(const FArguments&);
private:
    TStrongObjectPtr<UPixelWorldProfile> Profile;
    TSharedPtr<IDetailsView> Details;
    FText Status;
    FReply Analyze();
    FReply Generate(bool bReplace);
    FReply SaveProfile();
    void SetProfile(UPixelWorldProfile* InProfile);
};
