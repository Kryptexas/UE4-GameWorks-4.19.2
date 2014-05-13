// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SpeedTreeImporterPrivatePCH.h"
#include "ModuleManager.h"

/**
 * SpeedTreeImporter module implementation (private)
 */
class FSpeedTreeImporterModule : public ISpeedTreeImporter
{
public:
	virtual void StartupModule() OVERRIDE
	{
	}


	virtual void ShutdownModule() OVERRIDE
	{
	}

};

IMPLEMENT_MODULE(FSpeedTreeImporterModule, SpeedTreeImporter);
