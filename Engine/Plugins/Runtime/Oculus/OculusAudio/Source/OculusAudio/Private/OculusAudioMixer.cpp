// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAudioMixer.h"
#include "OculusAudioSettings.h"
#include "OculusAudioSourceSettings.h"
#include "IOculusAudioPlugin.h"


OculusAudioSpatializationAudioMixer::OculusAudioSpatializationAudioMixer()
	: bOvrContextInitialized(false)
	, Context(nullptr)
{
}

OculusAudioSpatializationAudioMixer::~OculusAudioSpatializationAudioMixer()
{
	if (bOvrContextInitialized)
	{
        // clear context from map
		Shutdown();
	}
}

void OculusAudioSpatializationAudioMixer::SetContext(ovrAudioContext* SharedContext)
{
    Context = SharedContext;

    check(Context != nullptr);

    Params.AddDefaulted(InitParams.NumSources);

    ovrAudioContextConfiguration ContextConfig;
    ContextConfig.acc_Size = sizeof(ovrAudioContextConfiguration);

    // First initialize the Fast algorithm context
    ContextConfig.acc_MaxNumSources = InitParams.NumSources;
    ContextConfig.acc_SampleRate = InitParams.SampleRate;
    ContextConfig.acc_BufferLength = InitParams.BufferLength;

    check(*Context == nullptr);
    // Create the OVR Audio Context with a given quality
    ovrResult Result = ovrAudio_CreateContext(Context, &ContextConfig);
    OVR_AUDIO_CHECK(Result, "Failed to create simple context");

    const UOculusAudioSettings* Settings = GetDefault<UOculusAudioSettings>();
    ApplyOculusAudioSettings(Settings);

    // Now initialize the high quality algorithm context
    bOvrContextInitialized = true;
}

void OculusAudioSpatializationAudioMixer::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
	if (bOvrContextInitialized)
	{
		return;
	}

    InitParams = InitializationParams;
}

void OculusAudioSpatializationAudioMixer::ApplyOculusAudioSettings(const UOculusAudioSettings* Settings)
{
    ovrResult Result = ovrAudio_Enable(*Context, ovrAudioEnable_SimpleRoomModeling, Settings->EarlyReflections);
    OVR_AUDIO_CHECK(Result, "Failed to enable reflections");

    Result = ovrAudio_Enable(*Context, ovrAudioEnable_LateReverberation, Settings->LateReverberation);
    OVR_AUDIO_CHECK(Result, "Failed to enable reverb");

    Result = ovrAudio_Enable(*Context, ovrAudioEnable_PerSourceReverb, 0);
    OVR_AUDIO_CHECK(Result, "Failed to enable shared reverb");

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

    Result = ovrAudio_SetSimpleBoxRoomParameters(*Context, &Room);
    OVR_AUDIO_CHECK(Result, "Failed to set room parameters");
}

void OculusAudioSpatializationAudioMixer::Shutdown()
{
	bOvrContextInitialized = false;
}

bool OculusAudioSpatializationAudioMixer::IsSpatializationEffectInitialized() const
{
	return bOvrContextInitialized;
}

void OculusAudioSpatializationAudioMixer::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings)
{
    check(bOvrContextInitialized);

    if (InSettings != nullptr) 
    {
        UOculusAudioSourceSettings* Settings = CastChecked<UOculusAudioSourceSettings>(InSettings);

        uint32 Flags = 0;
        if (!Settings->EarlyReflectionsEnabled)
            Flags |= ovrAudioSourceFlag_ReflectionsDisabled;

        ovrResult Result = ovrAudio_SetAudioSourceFlags(*Context, SourceId, Flags);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source flags");

        ovrAudioSourceAttenuationMode mode = Settings->AttenuationEnabled ? ovrAudioSourceAttenuationMode_InverseSquare : ovrAudioSourceAttenuationMode_None;
        Result = ovrAudio_SetAudioSourceAttenuationMode(*Context, SourceId, mode, 1.0f);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source attenuation mode");

        Result = ovrAudio_SetAudioSourceRange(*Context, SourceId, Settings->AttenuationRangeMinimum, Settings->AttenuationRangeMaximum);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source attenuation range");

        Result = ovrAudio_SetAudioSourceRadius(*Context, SourceId, Settings->VolumetricRadius);
        OVR_AUDIO_CHECK(Result, "Failed to set audio source volumetric radius");
    }
}

void OculusAudioSpatializationAudioMixer::SetSpatializationParameters(uint32 VoiceId, const FSpatializationParams& InParams)
{
	Params[VoiceId] = InParams;
}

void OculusAudioSpatializationAudioMixer::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
{
	if (InputData.SpatializationParams && *Context)
	{
		Params[InputData.SourceId] = *InputData.SpatializationParams;

        const float OVR_AUDIO_SCALE = 0.01f; // convert from centimeters to meters

        // Translate the input position to OVR coordinates
        FVector OvrListenerPosition = ToOVRVector(Params[InputData.SourceId].ListenerPosition * OVR_AUDIO_SCALE);
        FVector OvrListenerForward = ToOVRVector(Params[InputData.SourceId].ListenerOrientation.GetForwardVector());
        FVector OvrListenerUp = ToOVRVector(Params[InputData.SourceId].ListenerOrientation.GetUpVector());

        ovrResult Result = ovrAudio_SetListenerVectors(*Context,
            OvrListenerPosition.X, OvrListenerPosition.Y, OvrListenerPosition.Z,
            OvrListenerForward.X, OvrListenerForward.Y, OvrListenerForward.Z,
            OvrListenerUp.X, OvrListenerUp.Y, OvrListenerUp.Z);
        OVR_AUDIO_CHECK(Result, "Failed to set listener position and rotation");

		// Translate the input position to OVR coordinates
		FVector OvrPosition = ToOVRVector(Params[InputData.SourceId].EmitterWorldPosition * OVR_AUDIO_SCALE);

		// Set the source position to current audio position
		Result = ovrAudio_SetAudioSourcePos(*Context, InputData.SourceId, OvrPosition.X, OvrPosition.Y, OvrPosition.Z);
		OVR_AUDIO_CHECK(Result, "Failed to set audio source position");

		// Perform the processing
		uint32 Status;
		Result = ovrAudio_SpatializeMonoSourceInterleaved(*Context, InputData.SourceId, ovrAudioSpatializationFlag_None, &Status, OutputData.AudioBuffer.GetData(), InputData.AudioBuffer->GetData());
		OVR_AUDIO_CHECK(Result, "Failed to spatialize mono source interleaved");
	}
}