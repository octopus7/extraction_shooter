#pragma once

#include "CoreMinimal.h"
#include "StaticMeshQualitySwitcherService.h"
#include "Widgets/SCompoundWidget.h"

class SComboBoxBase;
class STextBlock;
class UStaticMeshQualityProfile;

class SStaticMeshQualitySwitcherPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStaticMeshQualitySwitcherPanel)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString GetProfileObjectPath() const;
	void HandleProfileChanged(const FAssetData& AssetData);
	TSharedRef<SWidget> GenerateScopeWidget(TSharedPtr<EStaticMeshQualityScope> Scope) const;
	void HandleScopeChanged(TSharedPtr<EStaticMeshQualityScope> Scope, ESelectInfo::Type SelectInfo);
	FText GetCurrentScopeText() const;
	FReply HandleValidateClicked();
	FReply HandleApplyClicked(EStaticMeshQualityTarget Target);
	UStaticMeshQualityProfile* LoadProfile() const;
	void SetStatus(const FText& Text, bool bIsError);
	void SetStatusFromErrors(const TArray<FText>& Errors);

	TArray<TSharedPtr<EStaticMeshQualityScope>> ScopeOptions;
	TSharedPtr<EStaticMeshQualityScope> CurrentScope;
	TSharedPtr<STextBlock> StatusText;
};
