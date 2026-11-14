#pragma once
// Authored by gpt-6-astra; reasoning effort: medium (verified session turn_context).
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Components/ComboBoxString.h"
#include "Encyclopedia/TunaSweeperEncyclopedia.h"
#include "EncyclopediaGPT6Astra.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UBorder;
class UListView;
class UComboBoxString;

UCLASS(Abstract, Blueprintable)
class TUNASWEEPER_API UEncyclopediaEntryGPT6Astra : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()
protected:
    virtual void NativeOnListItemObjectSet(UObject* Object) override;
    virtual void NativeOnItemSelectionChanged(bool bSelected) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> PortraitImage;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> MetaText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> RowBorder;
};

UCLASS(Abstract, Blueprintable)
class TUNASWEEPER_API UEncyclopediaPanelGPT6Astra : public UUserWidget
{
    GENERATED_BODY()
public:
    UEncyclopediaPanelGPT6Astra(const FObjectInitializer& Initializer);
    void Refresh();
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry&, const FKeyEvent&) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EnemyButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> WeaponButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AmmoButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CloseButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> EnemyText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> WeaponText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> AmmoText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CloseText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProgressText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailName;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailMeta;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailDescription;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailBehavior;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> DetailPortrait;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UListView> EntriesList;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> TypeFilter;
private:
    EEncyclopediaCategory Category = EEncyclopediaCategory::Enemy;
    UPROPERTY() TArray<TObjectPtr<UEncyclopediaEntryData>> Entries;
    TArray<FName> FilterGroups;
    bool bUpdatingFilter = false;
    FDelegateHandle LanguageHandle;
    UFUNCTION() void ShowEnemies();
    UFUNCTION() void ShowWeapons();
    UFUNCTION() void ShowAmmo();
    UFUNCTION() void Close();
    UFUNCTION() void FilterChanged(FString Selected, ESelectInfo::Type SelectionType);
    void ApplyFilter();
    void SelectionChanged(UObject* Object);
};

UCLASS(Abstract, Blueprintable)
class TUNASWEEPER_API UEncyclopediaLauncherGPT6Astra : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> OpenButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> OpenText;
private:
    FDelegateHandle LanguageHandle;
    void RefreshText();
    UFUNCTION() void Open();
};
