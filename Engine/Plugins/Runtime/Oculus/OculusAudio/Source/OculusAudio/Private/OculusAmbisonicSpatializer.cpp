// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAmbisonicSpatializer.h"
#include "OculusAudioMixer.h"

FOculusAmbisonicsMixer::FOculusAmbisonicsMixer()
	: Context(nullptr)
	, SampleRate(0)
{
}

void FOculusAmbisonicsMixer::Shutdown()
{
	ovrAudio_DestroyContext(Context);
}

int32 FOculusAmbisonicsMixer::GetNumChannelsForAmbisonicsFormat(UAmbisonicsSubmixSettingsBase* InSettings)
{
	return 4;
}

void FOculusAmbisonicsMixer::OnOpenEncodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InSettings)
{
	UOculusAmbisonicsSettings* Settings = CastChecked<UOculusAmbisonicsSettings>(InSettings);

	ovrAudioAmbisonicStream NewStream = nullptr;
	ovrAudioAmbisonicFormat StreamFormat = (Settings->ChannelOrder == EAmbisonicFormat::AmbiX ? ovrAudioAmbisonicFormat_AmbiX : ovrAudioAmbisonicFormat_FuMa);
	ovrResult OurResult = ovrAudio_CreateAmbisonicStream(Context, SampleRate, BufferLength, StreamFormat, 1, &NewStream);
	check(OurResult == 0);
	check(NewStream != nullptr);
	OpenStreams.Add(StreamId, NewStream);
}

void FOculusAmbisonicsMixer::OnCloseEncodingStream(const uint32 SourceId)
{
	ovrAudioAmbisonicStream* InStream = OpenStreams.Find(SourceId);
	if (InStream != nullptr)
	{
		ovrAudio_DestroyAmbisonicStream(*InStream);
		OpenStreams.Remove(SourceId);
	}
	else
	{
		checkf(false, TEXT("Tried to close a stream we did not open."));
	}
}

void FOculusAmbisonicsMixer::EncodeToAmbisonics(const uint32 SourceId, const FAmbisonicsEncoderInputData& InputData, FAmbisonicsEncoderOutputData& OutputData, UAmbisonicsSubmixSettingsBase* InParams)
{
	//Oculus only supports encoding mono streams.
	if (InputData.NumChannels == 1)
	{
		//TODO: Implement;
		//ovrAudio_MonoToAmbisonic(InputData.AudioBuffer->GetData())
	}
}

void FOculusAmbisonicsMixer::OnOpenDecodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InSettings, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions)
{
	UOculusAmbisonicsSettings* Settings = CastChecked<UOculusAmbisonicsSettings>(InSettings);

	ovrAudioAmbisonicStream NewStream = nullptr;
	ovrAudioAmbisonicFormat StreamFormat = (Settings->ChannelOrder == EAmbisonicFormat::AmbiX ? ovrAudioAmbisonicFormat_AmbiX : ovrAudioAmbisonicFormat_FuMa);
	ovrResult OurResult = ovrAudio_CreateAmbisonicStream(Context, SampleRate, BufferLength, StreamFormat, 1, &NewStream);

	check(OurResult == 0);
	check(NewStream != nullptr);

	ovrAudioAmbisonicSpeakerLayout SpeakerLayout = (Settings->SpatializationMode == EAmbisonicMode::SphericalHarmonics) ?
		ovrAudioAmbisonicSpeakerLayout_SphericalHarmonics : ovrAudioAmbisonicSpeakerLayout_Icosahedron;
	OurResult = ovrAudio_SetAmbisonicSpeakerLayout(NewStream, SpeakerLayout);

	check(OurResult == 0);
	OpenStreams.Add(StreamId, NewStream);
}

void FOculusAmbisonicsMixer::OnCloseDecodingStream(const uint32 StreamId)
{
	ovrAudioAmbisonicStream* InStream = OpenStreams.Find(StreamId);
	if (InStream != nullptr)
	{
		ovrAudio_DestroyAmbisonicStream(*InStream);
		OpenStreams.Remove(StreamId);
	}
	else
	{
		checkf(false, TEXT("Tried to close a stream we did not open."));
	}
}

void FOculusAmbisonicsMixer::DecodeFromAmbisonics(const uint32 StreamId, const FAmbisonicsDecoderInputData& InputData, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions, FAmbisonicsDecoderOutputData& OutputData)
{
	//Currently, Oculus only decodes first-order to stereo.
	if (SpecifiedOutputPositions.OutputNumChannels == 2 && InputData.NumChannels == 4)
	{
		//Get ambisonics stream
		ovrAudioAmbisonicStream* ThisStream = OpenStreams.Find(StreamId);
		if (ThisStream != nullptr)
		{
			//convert ambisonics rotation to listener rotation:
			{
				// Translate the input position to OVR coordinates
				FQuat& ListenerRotation = SpecifiedOutputPositions.ListenerRotation;
				FVector OvrListenerForward = OculusAudioSpatializationAudioMixer::ToOVRVector(ListenerRotation.GetForwardVector());
				FVector OvrListenerUp = OculusAudioSpatializationAudioMixer::ToOVRVector(ListenerRotation.GetUpVector());

				ovrResult Result = ovrAudio_SetListenerVectors(Context,
					0.0f, 0.0f, 0.0f,
					OvrListenerForward.X, OvrListenerForward.Y, OvrListenerForward.Z,
					OvrListenerUp.X, OvrListenerUp.Y, OvrListenerUp.Z);
			}
			ovrResult DecodeResult= ovrAudio_ProcessAmbisonicStreamInterleaved(Context, *ThisStream, InputData.AudioBuffer->GetData(), OutputData.AudioBuffer.GetData(), InputData.AudioBuffer->Num() / InputData.NumChannels);
			check(DecodeResult == 0);
		}
		else
		{
			checkf(false, TEXT("Invalid stream ID."));
		}
	}
	else
	{
		checkf(false, TEXT("Invalid number of channels. Input channels were %d but should be 4, output channels were %d but should be 2."), InputData.NumChannels, SpecifiedOutputPositions.OutputNumChannels);
	}

#define AMBISONICS_SINE_TEST 0

#if AMBISONICS_SINE_TEST
	static float n = 0.0f;
	for (int32 Index = 0; Index < OutputData.AudioBuffer.Num(); Index+= SpecifiedOutputPositions.OutputNumChannels)
	{
		// Do a sine test before we're all done
		for (int32 ChannelIndex = 0; ChannelIndex < SpecifiedOutputPositions.OutputNumChannels; ChannelIndex++)
		{
			OutputData.AudioBuffer[Index + ChannelIndex] = FMath::Sin(440.0f * n * 2.0f * PI / 48000.0f);
		}
		n += 1.0f;
	}
#endif
}

bool FOculusAmbisonicsMixer::ShouldReencodeBetween(UAmbisonicsSubmixSettingsBase* SourceSubmixSettings, UAmbisonicsSubmixSettingsBase* DestinationSubmixSettings)
{
	//stub
	return true;
}

void FOculusAmbisonicsMixer::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
	SampleRate = InitializationParams.SampleRate;
	BufferLength = InitializationParams.BufferLength;


	ovrAudioContextConfiguration ContextConfig;
	ContextConfig.acc_Size = sizeof(ovrAudioContextConfiguration);

	// TODO: Check if MaxNumSources also handles ambisonics stuff.
	ContextConfig.acc_MaxNumSources = 1;
	ContextConfig.acc_SampleRate = SampleRate;
	ContextConfig.acc_BufferLength = BufferLength;
	ovrResult Result = ovrAudio_CreateContext(&Context, &ContextConfig);
	OVR_AUDIO_CHECK(Result, "Failed to create ambisonic context");
}

UClass* FOculusAmbisonicsMixer::GetCustomSettingsClass()
{
	return UOculusAmbisonicsSettings::StaticClass();
}

UAmbisonicsSubmixSettingsBase* FOculusAmbisonicsMixer::GetDefaultSettings()
{
	static UOculusAmbisonicsSettings* DefaultSettingsPtr = nullptr;
	if (DefaultSettingsPtr == nullptr)
	{
		DefaultSettingsPtr = NewObject<UOculusAmbisonicsSettings>();
		DefaultSettingsPtr->ChannelOrder = EAmbisonicFormat::AmbiX;
		DefaultSettingsPtr->SpatializationMode = EAmbisonicMode::SphericalHarmonics;
		DefaultSettingsPtr->AddToRoot();
	}

	return DefaultSettingsPtr;
}
