// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAudioReverb.h"
#include "OculusAudioMixer.h"


void FSubmixEffectOculusReverbPlugin::SetContext(ovrAudioContext* SharedContext)
{
    Context = SharedContext;
    check(Context != nullptr);
}

FSubmixEffectOculusReverbPlugin::FSubmixEffectOculusReverbPlugin()
    : Context(nullptr)
{
}

void FSubmixEffectOculusReverbPlugin::OnProcessAudio(const FSoundEffectSubmixInputData& InputData, FSoundEffectSubmixOutputData& OutputData)
{
    if (Context != nullptr && *Context != nullptr)
    {
        int Enabled = 0;
        ovrResult Result = ovrAudio_IsEnabled(*Context, ovrAudioEnable_LateReverberation, &Enabled);
        OVR_AUDIO_CHECK(Result, "Failed to check if reverb is Enabled");

        int PerSource = 0;
        Result = ovrAudio_IsEnabled(*Context, ovrAudioEnable_PerSourceReverb, &PerSource);
        OVR_AUDIO_CHECK(Result, "Failed to check if reverb is Enabled");

        if (Enabled != 0 && PerSource == 0)
        {
            uint32_t Status = 0;
            Result = ovrAudio_MixInSharedReverbInterleaved(*Context, 0, &Status, OutputData.AudioBuffer->GetData());
            OVR_AUDIO_CHECK(Result, "Failed to process reverb");
        }
    }
}

void OculusAudioReverb::SetContext(ovrAudioContext* SharedContext)
{
    check(SharedContext != nullptr);
    Context = SharedContext;
    for (FSubmixEffectOculusReverbPlugin* Submix : Submixes)
    {
        Submix->SetContext(SharedContext);
    }
}

FSoundEffectSubmix* OculusAudioReverb::GetEffectSubmix(class USoundSubmix* Submix)
{
    FSubmixEffectOculusReverbPlugin* ReverbSubmix = new FSubmixEffectOculusReverbPlugin();
    Submixes.Add(ReverbSubmix);
    return ReverbSubmix;
}
