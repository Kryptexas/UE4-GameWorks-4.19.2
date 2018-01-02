// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#if PLATFORM_WINDOWS
#pragma once
#include "OVR_Audio.h"
#include "XAudio2Device.h"
#include "AudioEffect.h"
#include "XAudio2Effects.h"
#include "OculusAudioEffect.h"
#include "Runtime/Windows/XAudio2/Private/XAudio2Support.h"
#include "Misc/Paths.h"
#include "OculusAudioDllManager.h"
#include "OculusAudioSettings.h"

/************************************************************************/
/* OculusAudioLegacySpatialization                                      */
/* This spatialization plugin is used in the non-audiomixer engine,     */
/* XAudio2's HRTFEffect plugin system directly.                         */
/************************************************************************/
class OculusAudioLegacySpatialization : public IAudioSpatialization
{
public:
	OculusAudioLegacySpatialization();
	~OculusAudioLegacySpatialization();

	/** Begin IAudioSpatialization implementation */
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
	virtual void Shutdown() override;

	virtual bool IsSpatializationEffectInitialized() const override;
    virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings) override;
	virtual void ProcessSpatializationForVoice(uint32 VoiceIndex, float* InSamples, float* OutSamples, const FVector& Position) override;
	virtual bool CreateSpatializationEffect(uint32 VoiceId) override;
	virtual void* GetSpatializationEffect(uint32 VoiceId) override;
	virtual void SetSpatializationParameters(uint32 VoiceId, const FSpatializationParams& InParams) override;
	virtual void GetSpatializationParameters(uint32 VoiceId, FSpatializationParams& OutParams) override;
	/** End IAudioSpatialization implementation */

private:
	void ProcessAudioInternal(ovrAudioContext AudioContext, uint32 VoiceIndex, float* InSamples, float* OutSamples, const FVector& Position);
    void ApplyOculusAudioSettings(const UOculusAudioSettings* Settings);

    // Helper function to convert from UE coords to OVR coords.
    FORCEINLINE FVector ToOVRVector(const FVector& InVec) const
    {
        return FVector(InVec.Y, InVec.Z, -InVec.X);
    }
	
private:
	/* Whether or not the OVR audio context is initialized. We defer initialization until the first audio callback.*/
	bool bOvrContextInitialized;

	/** The OVR Audio context initialized to "Fast" algorithm. */
	ovrAudioContext OvrAudioContext;

	/** Xaudio2 effects for the oculus plugin */
	TArray<class FXAudio2HRTFEffect*> HRTFEffects;
	TArray<FSpatializationParams> Params;

	FCriticalSection ParamCriticalSection;
};

#endif // #if PLATFORM_WINDOWS