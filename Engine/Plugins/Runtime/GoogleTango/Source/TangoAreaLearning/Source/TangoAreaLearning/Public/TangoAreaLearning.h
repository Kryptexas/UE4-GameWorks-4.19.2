// Copyright 2017 Google Inc.

#pragma once

#include "ModuleManager.h"

class FTangoAreaLearningModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

DEFINE_LOG_CATEGORY_STATIC(LogTangoAreaLearning, Log, All);
