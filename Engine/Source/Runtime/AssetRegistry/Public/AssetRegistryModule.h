// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "ModuleInterface.h"
#include "IAssetRegistry.h"

/**
 * Asset registry module
 */
class FAssetRegistryModule : public IModuleInterface
{

public:

	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() OVERRIDE;

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() OVERRIDE;

	/** Gets the asset registry singleton */
	virtual IAssetRegistry& Get() const;

	/** Tick the asset registry with the supplied timestep */
	static void TickAssetRegistry(float DeltaTime)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistryModule.Get().Tick(DeltaTime);
	}

	/** Notifies the asset registry of a new in-memory asset */
	static void AssetCreated(UObject* NewAsset)
	{
		if ( GIsEditor )
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().AssetCreated(NewAsset);
		}
	}

	/** Notifies the asset registry than an in-memory asset was deleted */
	static void AssetDeleted(UObject* DeletedAsset)
	{
		if ( GIsEditor )
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().AssetDeleted(DeletedAsset);
		}
	}

	/** Notifies the asset registry that an in-memory asset was renamed */
	static void AssetRenamed(const UObject* RenamedAsset, const FString& OldObjectPath)
	{
		if ( GIsEditor )
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
			AssetRegistryModule.Get().AssetRenamed(RenamedAsset, OldObjectPath);
		}
	}

private:
	IAssetRegistry* AssetRegistry;
	class FAssetRegistryConsoleCommands* ConsoleCommands;
};
