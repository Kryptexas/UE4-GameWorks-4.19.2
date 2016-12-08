// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ISpeedTreeImporter.h"

/**
 * SpeedTreeImporter module implementation (private)
 */
class FSpeedTreeImporterModule : public ISpeedTreeImporter
{
public:
	virtual void StartupModule() override
	{
	}


	virtual void ShutdownModule() override
	{
	}

};

IMPLEMENT_MODULE(FSpeedTreeImporterModule, SpeedTreeImporter);
