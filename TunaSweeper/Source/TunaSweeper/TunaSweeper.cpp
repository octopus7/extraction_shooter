// Copyright Epic Games, Inc. All Rights Reserved.

#include "TunaSweeper.h"
#include "Modules/ModuleManager.h"
#include "Platform/TunaSweeperStove.h"

class FTunaSweeperGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		TunaSweeperStove::Startup();
	}
	virtual void ShutdownModule() override
	{
		TunaSweeperStove::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FTunaSweeperGameModule, TunaSweeper, "TunaSweeper" );
