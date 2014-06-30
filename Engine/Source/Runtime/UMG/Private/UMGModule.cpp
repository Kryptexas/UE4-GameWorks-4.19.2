// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "IUMGModule.h"
#include "ModuleManager.h"

class FUMGModule : public IUMGModule
{
public:
	/** Constructor, set up console commands and variables **/
	FUMGModule()
	{
	}

	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() override
	{
#if WITH_EDITOR	
		FUMGStyle::Initialize();

		// This is done so that the compiler is available in non-cooked builds when the widget blueprint is 
		// compiled again in the running game.
		if ( FParse::Param(FCommandLine::Get(), TEXT("umg")) )
		{
			FModuleManager::Get().LoadModule(TEXT("UMGEditor"));
		}
#endif
	}

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FUMGStyle::Shutdown();
#endif
	}
};

IMPLEMENT_MODULE(FUMGModule, UMG);
