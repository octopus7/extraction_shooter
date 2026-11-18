#include "ForestScatterEditorText.h"

#include "Interfaces/IPluginManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/StringTableRegistry.h"
#include "Modules/ModuleManager.h"

namespace
{
	constexpr TCHAR EnglishTable[] = TEXT("ForestScatter.Editor");
	constexpr TCHAR KoreanTable[] = TEXT("ForestScatter.Editor.ko");
}

FText ForestScatterEditorText::Get(const TCHAR* Key)
{
	const FString& Language = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();
	return FText::FromStringTable(Language == TEXT("ko") ? KoreanTable : EnglishTable, Key);
}

class FForestScatterModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("ForestScatter"));
		check(Plugin.IsValid());
		FStringTableRegistry& Tables = FStringTableRegistry::Get();
		Tables.Internal_LocTableFromFile(EnglishTable, EnglishTable, TEXT("Localization/ForestScatterEditor.csv"), Plugin->GetContentDir());
		Tables.Internal_LocTableFromFile(KoreanTable, KoreanTable, TEXT("Localization/ForestScatterEditor.ko.csv"), Plugin->GetContentDir());
	}

	virtual void ShutdownModule() override
	{
		FStringTableRegistry& Tables = FStringTableRegistry::Get();
		Tables.UnregisterStringTable(EnglishTable);
		Tables.UnregisterStringTable(KoreanTable);
	}
};

IMPLEMENT_MODULE(FForestScatterModule, ForestScatter)
