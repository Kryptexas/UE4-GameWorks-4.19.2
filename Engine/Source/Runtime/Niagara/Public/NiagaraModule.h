// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"

/**
* Foliage Edit mode module interface
*/
class INiagaraModule : public IModuleInterface
{
public:

	virtual void StartupModule()override;
};

