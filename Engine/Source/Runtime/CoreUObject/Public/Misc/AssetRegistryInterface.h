// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "CoreUObject.h"

/**
* HotReload module interface
*/
class IAssetRegistryInterface : public IModuleInterface
{
public:
	/**
	 * Tries to gets a pointer to the active AssetRegistryInterface implementation. 
	 */
	static inline IAssetRegistryInterface* GetPtr()
	{
		static FName AssetRegistry("AssetRegistry");
		auto Module = FModuleManager::GetModulePtr<IAssetRegistryInterface>(AssetRegistry);
		if (Module == nullptr)
		{
			Module = &FModuleManager::LoadModuleChecked<IAssetRegistryInterface>(AssetRegistry);
		}
		return reinterpret_cast<IAssetRegistryInterface*>(Module);
	}

	/**
	* Lookup dependencies for the given package name and fill OutDependencies with direct dependencies
	*/
	virtual void GetDependencies(FName InPackageName, TArray<FName>& OutDependencies) = 0;
};

