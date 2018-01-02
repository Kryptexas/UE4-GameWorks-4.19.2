// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioPluginUtilities.h"
#include "OVR_Audio.h"

class FOculusAudioContextManager : public IAudioPluginListener
{
public:
    FOculusAudioContextManager();
	virtual ~FOculusAudioContextManager() override;

    virtual void OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld) override;
    virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;
private:
    ovrAudioContext Context;
};