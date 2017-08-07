// Copyright 2017 Google Inc.

#pragma once

#include "ModuleManager.h"

class FTangoRendererModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};