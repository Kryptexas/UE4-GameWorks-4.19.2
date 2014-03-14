// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "IAssetTypeActions.h"
#include "IAssetTools.h"

class FAssetToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();

	virtual IAssetTools& Get() const;

private:
	IAssetTools* AssetTools;
	class FAssetToolsConsoleCommands* ConsoleCommands;
	TArray<TSharedRef<IAssetTypeActions>> AssetTypeActionsList;
};
