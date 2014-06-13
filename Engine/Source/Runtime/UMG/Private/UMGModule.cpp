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
		if ( FParse::Param(FCommandLine::Get(), TEXT("umg")) )
		{
			FModuleManager::Get().LoadModule(TEXT("UMGEditor"));
		}
#endif
	}

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FUMGModule, UMG);
