// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPCH.h"
#include "AudioMixerModule.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY(LogAudioMixer);
DEFINE_LOG_CATEGORY(LogAudioMixerDebug);

class FAudioMixerModule : public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
	}
};

IMPLEMENT_MODULE(FAudioMixerModule, AudioMixer);