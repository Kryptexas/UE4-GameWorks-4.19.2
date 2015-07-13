// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "Core.h"

/**
* CookingStats module interface
*/
class ICookingStatsInterface : public IModuleInterface
{
public:
	/**
	 * Tries to gets a pointer to the active AssetRegistryInterface implementation. 
	 */
	static inline ICookingStatsInterface * GetPtr()
	{
		static FName CookingStats("CookingStats");
		auto Module = FModuleManager::GetModulePtr<ICookingStatsInterface>(CookingStats);
		if (Module == nullptr)
		{
			Module = &FModuleManager::LoadModuleChecked<ICookingStatsInterface>(CookingStats);
		}
		return reinterpret_cast<ICookingStatsInterface*>(Module);
	}

};

