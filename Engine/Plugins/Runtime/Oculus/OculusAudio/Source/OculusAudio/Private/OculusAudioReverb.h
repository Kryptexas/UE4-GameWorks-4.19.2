// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "Sound/SoundEffectSubmix.h"
#include "OVR_Audio.h"


class FSubmixEffectOculusReverbPlugin : public FSoundEffectSubmix
{
public:
    FSubmixEffectOculusReverbPlugin();

    void SetContext(ovrAudioContext* SharedContext);

    virtual void Init(const FSoundEffectSubmixInitData& InSampleRate) override
    {
        return; // PAS
    }
    virtual uint32 GetDesiredInputChannelCountOverride() const override
    {
        static const int STEREO = 2;
        return STEREO; // PAS
    }
    virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;
    virtual void OnPresetChanged() override
    {
        return; // PAS
    }
private:
    ovrAudioContext* Context;
};

/************************************************************************/
/* OculusAudioReverb                                                    */
/* This implementation of IAudioReverb uses the Oculus Audio            */
/* library to render spatial reverb.                                    */
/************************************************************************/
class OculusAudioReverb : public IAudioReverb
{
public:
    OculusAudioReverb()
        : Context(nullptr)
    {
        // empty
    }

    void SetContext(ovrAudioContext* SharedContext);

    virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels, UReverbPluginSourceSettingsBase* InSettings) override
    {
        return; // PAS
    }

    virtual void OnReleaseSource(const uint32 SourceId) override 
    {
        return; // PAS
    }

    virtual class FSoundEffectSubmix* GetEffectSubmix(class USoundSubmix* Submix) override;

    virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override
    {
        return; // PAS
    }
private:
    ovrAudioContext* Context;
    TArray<FSubmixEffectOculusReverbPlugin*> Submixes;
};