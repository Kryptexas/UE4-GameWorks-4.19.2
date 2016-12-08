// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class ULocalizationTarget;

class LOCALIZATION_API FLocalizationModule : public IModuleInterface
{
public:
	ULocalizationTarget* GetLocalizationTargetByName(FString TargetName, bool bIsEngineTarget);

	static FLocalizationModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FLocalizationModule>("Localization");
	}
};
