// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ModuleInterface.h"

class ULocalizationTarget;

class FLocalizationModule : public IModuleInterface
{
public:
	ULocalizationTarget* GetLocalizationTargetByName(FString TargetName, bool bIsEngineTarget);
};