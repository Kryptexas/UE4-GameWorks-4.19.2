// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "AssetRegistryModule.h"
#include "AssetRegistry.h"
#include "AssetRegistryConsoleCommands.h"

IMPLEMENT_MODULE( FAssetRegistryModule, AssetRegistry );

void FAssetRegistryModule::StartupModule()
{
	AssetRegistry = MakeWeakObjectPtr(const_cast<UAssetRegistryImpl*>(GetDefault<UAssetRegistryImpl>()));
	ConsoleCommands = new FAssetRegistryConsoleCommands(*this);
}


void FAssetRegistryModule::ShutdownModule()
{
	AssetRegistry = nullptr;

	if ( ConsoleCommands )
	{
		delete ConsoleCommands;
		ConsoleCommands = NULL;
	}
}

IAssetRegistry& FAssetRegistryModule::Get() const
{
	check(AssetRegistry.IsValid());
	return *AssetRegistry;
}

