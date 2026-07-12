#pragma once

#include "CoreMinimal.h"

class FTunaSweeperGlbTextureExtractorTool
{
public:
	void Startup();
	void Shutdown();

private:
	void RegisterMenus();
	void OpenGlbTextureExtractor() const;
};
