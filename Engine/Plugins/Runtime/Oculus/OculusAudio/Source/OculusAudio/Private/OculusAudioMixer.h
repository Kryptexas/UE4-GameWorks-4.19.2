// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OVR_Audio.h"
#include "Misc/Paths.h"
#include "OculusAudioDllManager.h"
#include "OculusAudioSettings.h"
#include "Containers/Ticker.h"

/************************************************************************/
/* OculusAudioSpatializationAudioMixer                                  */
/* This implementation of IAudioSpatialization uses the Oculus Audio    */
/* library to render audio sources with HRTF spatialization.            */
/************************************************************************/
class OculusAudioSpatializationAudioMixer : public IAudioSpatialization
{
public:
	OculusAudioSpatializationAudioMixer();
	~OculusAudioSpatializationAudioMixer();

    void SetContext(ovrAudioContext* SharedContext);

	//~ Begin IAudioSpatialization 
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
	virtual void Shutdown() override;
	virtual bool IsSpatializationEffectInitialized() const override;
    virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings) override;
    virtual void SetSpatializationParameters(uint32 VoiceId, const FSpatializationParams& InParams) override;
	virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;
	//~ End IAudioSpatialization

	// Helper function to convert from UE coords to OVR coords.
	static FVector ToOVRVector(const FVector& InVec)
	{
		return FVector(InVec.Y, InVec.Z, -InVec.X);
	}

private:

    void ApplyOculusAudioSettings(const UOculusAudioSettings* Settings);

	// Whether or not the OVR audio context is initialized. We defer initialization until the first audio callback.
	bool bOvrContextInitialized;

	TArray<FSpatializationParams> Params;

	ovrAudioContext* Context;
    FAudioPluginInitializationParams InitParams;
};