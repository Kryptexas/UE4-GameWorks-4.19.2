// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#if PLATFORM_WINDOWS

#include "OculusAudioLegacy.h"
#include "OculusAudioSourceSettings.h"

OculusAudioLegacySpatialization::OculusAudioLegacySpatialization()
	: bOvrContextInitialized(false)
	, OvrAudioContext(nullptr)
{
	
}

OculusAudioLegacySpatialization::~OculusAudioLegacySpatialization()
{
}

void OculusAudioLegacySpatialization::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
	check(HRTFEffects.Num() == 0);
	check(Params.Num() == 0);

	for (int32 EffectIndex = 0; EffectIndex < (int32)InitializationParams.NumSources; ++EffectIndex)
	{
		/* Hack: grab main audio device. */
		FXAudio2HRTFEffect* NewHRTFEffect = new FXAudio2HRTFEffect(EffectIndex, InitializationParams.AudioDevicePtr);
		/* End hack. */
		NewHRTFEffect->Initialize(nullptr, 0);
		HRTFEffects.Add(NewHRTFEffect);

		Params.Add(FSpatializationParams());
	}

	if (bOvrContextInitialized)
	{
		return;
	}

	ovrAudioContextConfiguration ContextConfig;
	ContextConfig.acc_Size = sizeof(ovrAudioContextConfiguration);

	// First initialize the Fast algorithm context
	ContextConfig.acc_MaxNumSources = InitializationParams.NumSources;
	ContextConfig.acc_SampleRate = InitializationParams.SampleRate;
	//XAudio2 sets the buffer callback size to a 100th of the sample rate:
	ContextConfig.acc_BufferLength = InitializationParams.SampleRate / 100;

	check(OvrAudioContext == nullptr);
	// Create the OVR Audio Context with a given quality
	ovrResult Result = ovrAudio_CreateContext(&OvrAudioContext, &ContextConfig);
	OVR_AUDIO_CHECK(Result, "Failed to create simple context");

    const UOculusAudioSettings* Settings = GetDefault<UOculusAudioSettings>();
    ApplyOculusAudioSettings(Settings);

	// Now initialize the high quality algorithm context
	bOvrContextInitialized = true;
}

void OculusAudioLegacySpatialization::Shutdown()
{
	// Release all the effects for the oculus spatialization effect
	for (FXAudio2HRTFEffect* Effect : HRTFEffects)
	{
		if (Effect)
		{
			delete Effect;
		}
	}
	HRTFEffects.Empty();

	// Destroy the contexts if we created them
	if (bOvrContextInitialized)
	{
		if (OvrAudioContext != nullptr)
		{
			ovrAudio_DestroyContext(OvrAudioContext);
			OvrAudioContext = nullptr;
		}

		bOvrContextInitialized = false;
	}
}

bool OculusAudioLegacySpatialization::IsSpatializationEffectInitialized() const
{
	return bOvrContextInitialized;
}

void OculusAudioLegacySpatialization::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings)
{
    if (InSettings != nullptr)
    {
        UOculusAudioSourceSettings* Settings = CastChecked<UOculusAudioSourceSettings>(InSettings);

        uint32 Flags = 0;
        if (!Settings->EarlyReflectionsEnabled)
            Flags |= ovrAudioSourceFlag_ReflectionsDisabled;

        ovrResult Result = ovrAudio_SetAudioSourceFlags(OvrAudioContext, SourceId, Flags);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source flags");

        ovrAudioSourceAttenuationMode mode = Settings->AttenuationEnabled ? ovrAudioSourceAttenuationMode_InverseSquare : ovrAudioSourceAttenuationMode_None;
        Result = ovrAudio_SetAudioSourceAttenuationMode(OvrAudioContext, SourceId, mode, 1.0f);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source attenuation mode");

        Result = ovrAudio_SetAudioSourceRange(OvrAudioContext, SourceId, Settings->AttenuationRangeMinimum, Settings->AttenuationRangeMaximum);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source attenuation range");

        Result = ovrAudio_SetAudioSourceRadius(OvrAudioContext, SourceId, Settings->VolumetricRadius);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source volumetric radius");
    }
}

void OculusAudioLegacySpatialization::ProcessSpatializationForVoice(uint32 VoiceIndex, float* InSamples, float* OutSamples, const FVector& Position)
{
	if (OvrAudioContext)
	{
		ProcessAudioInternal(OvrAudioContext, VoiceIndex, InSamples, OutSamples, Position);
	}
}

bool OculusAudioLegacySpatialization::CreateSpatializationEffect(uint32 VoiceId)
{
	// If an effect for this voice has already been created, then leave
	if ((int32)VoiceId >= HRTFEffects.Num())
	{
		return false;
	}
	return true;
}

void* OculusAudioLegacySpatialization::GetSpatializationEffect(uint32 VoiceId)
{
	if ((int32)VoiceId < HRTFEffects.Num())
	{
		return (void*)HRTFEffects[VoiceId];
	}
	return nullptr;
}

void OculusAudioLegacySpatialization::SetSpatializationParameters(uint32 VoiceId, const FSpatializationParams& InParams)
{
	check((int32)VoiceId < Params.Num());

	FScopeLock ScopeLock(&ParamCriticalSection);
	Params[VoiceId] = InParams;
}

void OculusAudioLegacySpatialization::GetSpatializationParameters(uint32 VoiceId, FSpatializationParams& OutParams)
{
	check((int32)VoiceId < Params.Num());

	FScopeLock ScopeLock(&ParamCriticalSection);
	OutParams = Params[VoiceId];
}

void OculusAudioLegacySpatialization::ProcessAudioInternal(ovrAudioContext AudioContext, uint32 VoiceIndex, float* InSamples, float* OutSamples, const FVector& Position)
{
    const float OVR_AUDIO_SCALE = 0.01f; // convert from centimeters to meters

    // Translate the input position to OVR coordinates
    FVector OvrListenerPosition = ToOVRVector(Params[VoiceIndex].ListenerPosition * OVR_AUDIO_SCALE);
    FVector OvrListenerForward = ToOVRVector(Params[VoiceIndex].ListenerOrientation.GetForwardVector());
    FVector OvrListenerUp = ToOVRVector(Params[VoiceIndex].ListenerOrientation.GetUpVector());

    ovrResult Result = ovrAudio_SetListenerVectors(OvrAudioContext,
        OvrListenerPosition.X, OvrListenerPosition.Y, OvrListenerPosition.Z,
        OvrListenerForward.X, OvrListenerForward.Y, OvrListenerForward.Z,
        OvrListenerUp.X, OvrListenerUp.Y, OvrListenerUp.Z);
    OVR_AUDIO_CHECK(Result, "Failed to set listener position and rotation");

    // Translate the input position to OVR coordinates
    FVector OvrPosition = ToOVRVector(Params[VoiceIndex].EmitterWorldPosition * OVR_AUDIO_SCALE);

	// Set the source position to current audio position
	Result = ovrAudio_SetAudioSourcePos(AudioContext, VoiceIndex, OvrPosition.X, OvrPosition.Y, OvrPosition.Z);
	OVR_AUDIO_CHECK(Result, "Failed to set audio source position");

	// Perform the processing
	uint32 Status;
	Result = ovrAudio_SpatializeMonoSourceInterleaved(AudioContext, VoiceIndex, ovrAudioSpatializationFlag_None, &Status, OutSamples, InSamples);
	OVR_AUDIO_CHECK(Result, "Failed to spatialize mono source interleaved");
}

void OculusAudioLegacySpatialization::ApplyOculusAudioSettings(const UOculusAudioSettings* Settings)
{
    ovrResult Result = ovrAudio_Enable(OvrAudioContext, ovrAudioEnable_SimpleRoomModeling, Settings->EarlyReflections);
    OVR_AUDIO_CHECK(Result, "Failed to enable reflections");

    Result = ovrAudio_Enable(OvrAudioContext, ovrAudioEnable_LateReverberation, Settings->LateReverberation);
    OVR_AUDIO_CHECK(Result, "Failed to enable reverb");

    ovrAudioBoxRoomParameters Room = { 0 };
    Room.brp_Size = sizeof(Room);
    Room.brp_Width = Settings->Width;
    Room.brp_Height = Settings->Height;
    Room.brp_Depth = Settings->Depth;
    Room.brp_ReflectLeft = Settings->ReflectionCoefLeft;
    Room.brp_ReflectRight = Settings->ReflectionCoefRight;
    Room.brp_ReflectUp = Settings->ReflectionCoefUp;
    Room.brp_ReflectDown = Settings->ReflectionCoefDown;
    Room.brp_ReflectBehind = Settings->ReflectionCoefBack;
    Room.brp_ReflectFront = Settings->ReflectionCoefFront;

    Result = ovrAudio_SetSimpleBoxRoomParameters(OvrAudioContext, &Room);
    OVR_AUDIO_CHECK(Result, "Failed to set room parameters");
}

#endif // #if PLATFORM_WINDOWS