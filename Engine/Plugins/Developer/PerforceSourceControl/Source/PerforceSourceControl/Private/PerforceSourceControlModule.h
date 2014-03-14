// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PerforceSourceControlProvider.h"
#include "PerforceSourceControlSettings.h"

class FPerforceSourceControlModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** Access the Perforce source control settings */
	FPerforceSourceControlSettings& AccessSettings();

	/** Save the Perforce source control settings */
	void SaveSettings();

	/** Access the one and only Perforce provider */
	FPerforceSourceControlProvider& GetProvider()
	{
		return PerforceSourceControlProvider;
	}

private:
	/** The one and only Perforce source control provider */
	FPerforceSourceControlProvider PerforceSourceControlProvider;

	/** The settings for Perforce source control */
	FPerforceSourceControlSettings PerforceSourceControlSettings;
};