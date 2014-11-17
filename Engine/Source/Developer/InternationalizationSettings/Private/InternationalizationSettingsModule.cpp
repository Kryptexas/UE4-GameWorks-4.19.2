// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "InternationalizationSettingsModulePrivatePCH.h"

class FInternationalizationSettingsModule : public IInternationalizationSettingsModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
};

IMPLEMENT_MODULE( FInternationalizationSettingsModule, InternationalizationSettingsModule )

void FInternationalizationSettingsModule::StartupModule()
{
}

void FInternationalizationSettingsModule::ShutdownModule()
{
}