// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/**
	Concrete implementation of FAudioDevice for Apple's CoreAudio

*/

#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "AudioMixerPlatformCoreAudio.h"


class FAudioMixerModuleCoreAudio : public IAudioDeviceModule
{
public:

	virtual bool IsAudioMixerModule() const override { return true; }

	virtual FAudioDevice* CreateAudioDevice() override
	{
		return new Audio::FMixerDevice(new Audio::FMixerPlatformCoreAudio());
	}
};

IMPLEMENT_MODULE(FAudioMixerModuleCoreAudio, AudioMixerCoreAudio);
