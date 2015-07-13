// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Core.h"

/**
* CookingStats module interface
*/
class ICookingStatsModule : public IModuleInterface
{
public:
	/**
	 * Tries to gets a pointer to the active AssetRegistryInterface implementation. 
	 */
	static inline ICookingStatsModule * GetPtr()
	{
		static FName CookingStats("CookingStats");
		auto Module = FModuleManager::GetModulePtr<ICookingStatsModule>(CookingStats);
		if (Module == nullptr)
		{
			Module = &FModuleManager::LoadModuleChecked<ICookingStatsModule>(CookingStats);
		}
		return reinterpret_cast<ICookingStatsModule*>(Module);
	}

};

