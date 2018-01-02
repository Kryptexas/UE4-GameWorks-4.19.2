// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

namespace EAssetRegistryDependencyType
{
	enum Type
	{
		// Dependencies which don't need to be loaded for the object to be used (i.e. string asset references)
		Soft = 0x01,

		// Dependencies which are required for correct usage of the source asset, and must be loaded at the same time
		Hard = 0x02,

		// References to specific SearchableNames inside a package
		SearchableName = 0x04,

		// Indirect management references, these are set through recursion for Primary Assets that manage packages or other primary assets
		SoftManage = 0x08,

		// Reference that says one object directly manages another object, set when Primary Assets manage things explicitly
		HardManage = 0x10,
	};

	static const Type None = (Type)(0);
	static const Type All = (Type)(Soft | Hard | SearchableName | SoftManage | HardManage);
	static const Type Packages = (Type)(Soft | Hard);
	static const Type Manage = (Type)(SoftManage | HardManage);
}

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
	virtual void GetDependencies(FName InPackageName, TArray<FName>& OutDependencies, EAssetRegistryDependencyType::Type InDependencyType = EAssetRegistryDependencyType::Packages) = 0;
};

