#include "TunaSweeperTitleCreditsRemoval.h"

#include "Blueprint/WidgetTree.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace TunaSweeperTitleCreditsRemoval
{
namespace
{
	bool SaveWidgetBlueprint(UWidgetBlueprint* Blueprint)
	{
		TSet<FName> ConnectedWidgetNames;
		Blueprint->WidgetTree->ForEachWidget([&ConnectedWidgetNames, Blueprint](UWidget* Widget)
		{
			ConnectedWidgetNames.Add(Widget->GetFName());
			Widget->bIsVariable = true;
			if (!Blueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
			{
				Blueprint->OnVariableAdded(Widget->GetFName());
			}
		});

		for (auto It = Blueprint->WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!ConnectedWidgetNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error)
		{
			return false;
		}

		Blueprint->MarkPackageDirty();
		UPackage* Package = Blueprint->GetOutermost();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *PackageFileName, SaveArgs);
	}

	bool RemoveWidgetAndSave(const TCHAR* BlueprintPath, const TCHAR* WidgetName)
	{
		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, BlueprintPath);
		if (!Blueprint || !Blueprint->WidgetTree)
		{
			return false;
		}

		UWidget* Widget = Blueprint->WidgetTree->FindWidget(WidgetName);
		if (!Widget)
		{
			return SaveWidgetBlueprint(Blueprint);
		}

		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (!Blueprint->WidgetTree->RemoveWidget(Widget))
		{
			return false;
		}
		Widget->Rename(
			nullptr,
			GetTransientPackage(),
			REN_DontCreateRedirectors | REN_NonTransactional);
		return SaveWidgetBlueprint(Blueprint);
	}
}

bool Run()
{
	return RemoveWidgetAndSave(
		TEXT("/Game/UI/Title/Screens/WBP_TitleMain.WBP_TitleMain"),
		TEXT("CreditsButtonBox")) &&
		RemoveWidgetAndSave(
			TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu"),
			TEXT("CreditsPanelView"));
}
}
