#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperRaidLiftWidget.generated.h"
class ATunaSweeperRaidLiftActor;
UCLASS()
class TUNASWEEPER_API UTunaSweeperRaidLiftWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void InitializeLift(ATunaSweeperRaidLiftActor* InLift) { Lift = InLift; }
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    TWeakObjectPtr<ATunaSweeperRaidLiftActor> Lift;
    UFUNCTION() void Forest();
    UFUNCTION() void VillageEast();
    UFUNCTION() void VillageWest();
    UFUNCTION() void Plains();
    UFUNCTION() void Cancel();
};
