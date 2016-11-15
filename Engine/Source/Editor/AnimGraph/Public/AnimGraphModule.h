// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

#include "AnimGraphCommands.h"

class FAnimGraphModule : public IModuleInterface
{
public:
	/** IModuleInterface interface */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};