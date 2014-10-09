// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IUndoHistoryModule.h"

static const FName UndoHistoryTabName("UndoHistory");

/**
* Implements the UndoHistory module.
*/
class FUndoHistoryModule : public IUndoHistoryModule
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override;

	// End IModuleInterface interface

	//static /*virtual */void ExecuteOpenUndoHistory() /*const override*/;
	static void ExecuteOpenUndoHistory() /*const*/
	{
		FGlobalTabmanager::Get()->InvokeTab(UndoHistoryTabName);
	}

private:
	// Handles creating the project settings tab.
	TSharedRef<SDockTab> HandleSpawnSettingsTab(const FSpawnTabArgs& SpawnTabArgs);
};