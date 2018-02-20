// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AudioCaptureComponent.h"

UAudioCaptureComponent::UAudioCaptureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSuccessfullyInitialized = false;
	bIsCapturing = false;
	CaptureAudioDataSamples = 0;
	ReadSampleIndex = 0;
	bIsDestroying = false;
	bIsReadyForForFinishDestroy = true;
	bIsStreamOpen = false;
	bOutputToBusOnly = true;
}

bool UAudioCaptureComponent::Init(int32& SampleRate)
{
	Audio::FCaptureDeviceInfo DeviceInfo;
	if (CaptureSynth.GetDefaultCaptureDeviceInfo(DeviceInfo))
	{
		SampleRate = DeviceInfo.PreferredSampleRate;
		NumChannels = DeviceInfo.InputChannels;
		// Only support mono and stereo mic inputs for now...
		if (NumChannels == 1 || NumChannels == 2)
		{
			return true;
		}
		else
		{
			UE_LOG(LogAudio, Warning, TEXT("Audio capture components only support mono and stereo mic input."));
		}
	}
	return false;
}

void UAudioCaptureComponent::BeginDestroy()
{
	Super::BeginDestroy();

	// Flag that we're beginning to be destroyed
	// This is so that if a mic capture is open, we shut it down on the render thread
	bIsDestroying = true;

	// Make sure stop is kicked off
	Stop();
}

bool UAudioCaptureComponent::IsReadyForFinishDestroy()
{
	return bIsReadyForForFinishDestroy;
}

void UAudioCaptureComponent::FinishDestroy()
{
	Super::FinishDestroy();
	bSuccessfullyInitialized = false;
	bIsCapturing = false;
	bIsDestroying = false;
	bIsStreamOpen = false;
	bOutputToBusOnly = true;
}

void UAudioCaptureComponent::OnBeginGenerate()
{
	if (!CaptureSynth.IsStreamOpen())
	{
		// This may fail if capture synths aren't supported on a given platform or if something went wrong with the capture device
		bIsStreamOpen = CaptureSynth.OpenDefaultStream();
		if (bIsStreamOpen)
		{
			check(CaptureSynth.IsStreamOpen());
			CaptureSynth.StartCapturing();
			check(CaptureSynth.IsCapturing());

			// Don't allow this component to be destroyed until the stream is closed again
			bIsReadyForForFinishDestroy = false;

			FramesSinceStarting = 0;
			ReadSampleIndex = 0;
		}
		else
		{
			UE_LOG(LogAudio, Warning, TEXT("Was not able to open a default capture stream."));
		}
	}
}

void UAudioCaptureComponent::OnEndGenerate()
{
	if (bIsStreamOpen && CaptureSynth.IsStreamOpen())
	{
		CaptureSynth.AbortCapturing();
		check(!CaptureSynth.IsCapturing());
		check(!CaptureSynth.IsStreamOpen());
	}
	bIsStreamOpen = false;
	bIsReadyForForFinishDestroy = true;
}

// Called when synth is about to start playing
void UAudioCaptureComponent::OnStart() 
{
}

void UAudioCaptureComponent::OnStop()
{
}

void UAudioCaptureComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	// Don't do anything if the stream isn't open
	if (!bIsStreamOpen || !CaptureSynth.IsStreamOpen() || !CaptureSynth.IsCapturing())
	{
		return;
	}

	// Allow the mic capture to buffer up some audio before starting to consume it
	if (FramesSinceStarting >= JitterLatencyFrames)
	{
		// Check if we need to get more audio
		if (ReadSampleIndex >= CaptureAudioDataSamples)
		{
			ReadSampleIndex = 0;
			CaptureSynth.GetAudioData(CaptureAudioData);

			CaptureAudioDataSamples = CaptureAudioData.Num();
		}

		// If there was audio available, copy it out
		if (CaptureAudioDataSamples > 0)
		{
			// Only need to copy over the non-zero audio buffer. The OutAudio ptr has already been zeroed. 
			float* CaptureAudioDataPtr = CaptureAudioData.GetData();
			int32 NumSamplesToCopy = FMath::Min(CaptureAudioDataSamples, NumSamples);
			float* SrcBufferPtr = CaptureAudioDataPtr + ReadSampleIndex;
			FMemory::Memcpy(OutAudio, SrcBufferPtr, NumSamplesToCopy * sizeof(float));

			ReadSampleIndex += NumSamplesToCopy;
		}
	}
	else
	{
		FramesSinceStarting += (NumSamples * NumChannels);
	}
}
