// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "LinearTimecodePlugin.h"

class FLinearTimecodePlugin : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

void FLinearTimecodePlugin::StartupModule()
{
}

void FLinearTimecodePlugin::ShutdownModule()
{
}

IMPLEMENT_MODULE(FLinearTimecodePlugin, LinearTimecode)
