// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAudio.h"
#include "OculusAmbisonicSpatializer.h"
#include "OculusAudioReverb.h"
#include "IOculusAudioPlugin.h"
#include "Features/IModularFeatures.h"

TAudioSpatializationPtr FOculusSpatializationPluginFactory::CreateNewSpatializationPlugin(FAudioDevice* OwningDevice)
{
    FOculusAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FOculusAudioPlugin>("OculusAudio");
    check(Plugin != nullptr);
    Plugin->RegisterAudioDevice(OwningDevice);

#if PLATFORM_WINDOWS
	if (OwningDevice->IsAudioMixerEnabled())
#endif
	{
		return TAudioSpatializationPtr(new OculusAudioSpatializationAudioMixer());
	}
#if PLATFORM_WINDOWS
	else
	{
		return TAudioSpatializationPtr(new OculusAudioLegacySpatialization());
	}
#endif
}


TAmbisonicsMixerPtr FOculusSpatializationPluginFactory::CreateNewAmbisonicsMixer(FAudioDevice* OwningDevice)
{
	return TAmbisonicsMixerPtr(new FOculusAmbisonicsMixer());
}

TAudioReverbPtr FOculusReverbPluginFactory::CreateNewReverbPlugin(FAudioDevice* OwningDevice)
{
    FOculusAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FOculusAudioPlugin>("OculusAudio");
    check(Plugin != nullptr);
    Plugin->RegisterAudioDevice(OwningDevice);

    return TAudioReverbPtr(new OculusAudioReverb());
}