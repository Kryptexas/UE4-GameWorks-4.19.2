// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAudioContextManager.h"
#include "OculusAmbisonicSpatializer.h"
#include "OculusAudioReverb.h"
#include "Features/IModularFeatures.h"
#include "AudioMixerDevice.h"
#include "IOculusAudioPlugin.h"

FOculusAudioContextManager::FOculusAudioContextManager()
    : Context(nullptr)
{
    // empty
}

FOculusAudioContextManager::~FOculusAudioContextManager()
{
	ovrAudio_DestroyContext(Context);
	Context = nullptr;
}

void FOculusAudioContextManager::OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld)
{
    // Inject the Context into the spatailizer (and reverb) if they're enabled

    FOculusAudioPlugin* Plugin = &FModuleManager::GetModuleChecked<FOculusAudioPlugin>("OculusAudio");
    if (Plugin == nullptr)
    {
        return; // PAS is this needed?
    }

    FString OculusSpatializerPluginName = Plugin->GetSpatializationPluginFactory()->GetDisplayName();
    FString CurrentSpatializerPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::SPATIALIZATION, AudioPluginUtilities::CurrentPlatform);
    if (CurrentSpatializerPluginName.Equals(OculusSpatializerPluginName)) // we have a match!
    {
        OculusAudioSpatializationAudioMixer* Spatializer = 
            static_cast<OculusAudioSpatializationAudioMixer*>(AudioDevice->SpatializationPluginInterface.Get());
        Spatializer->SetContext(&Context);
    }

    FString OculusReverbPluginName = Plugin->GetReverbPluginFactory()->GetDisplayName();
    FString CurrentReverbPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::REVERB, AudioPluginUtilities::CurrentPlatform);
    if (CurrentReverbPluginName.Equals(OculusReverbPluginName))
    {
        OculusAudioReverb* Reverb = static_cast<OculusAudioReverb*>(AudioDevice->ReverbPluginInterface.Get());
        Reverb->SetContext(&Context);
    }
}

void FOculusAudioContextManager::OnListenerShutdown(FAudioDevice* AudioDevice)
{
    
}
