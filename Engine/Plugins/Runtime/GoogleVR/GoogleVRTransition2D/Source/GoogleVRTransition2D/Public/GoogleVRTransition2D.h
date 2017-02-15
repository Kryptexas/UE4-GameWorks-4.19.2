// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogGoogleVRTransition2D, Log, All);

class FGoogleVRTransition2DModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};