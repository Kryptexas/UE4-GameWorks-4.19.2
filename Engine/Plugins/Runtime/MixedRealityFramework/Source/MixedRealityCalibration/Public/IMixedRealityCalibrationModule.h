// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "IConsoleManager.h" // for TAutoConsoleVariable<>

class IConsoleVariable;

/**
 * 
 */
class IMixedRealityCalibrationModule : public IModuleInterface
{
public:
	/** Virtual destructor. */
	virtual ~IMixedRealityCalibrationModule() {}
};

