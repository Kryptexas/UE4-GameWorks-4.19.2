// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AudioCapture.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "AudioCaptureInternal.h"

DEFINE_LOG_CATEGORY(LogAudioCapture);

namespace Audio 
{

FAudioCapture::FAudioCapture()
{
	Impl = CreateImpl();
}

FAudioCapture::~FAudioCapture()
{
}

bool FAudioCapture::GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo)
{
	if (Impl.IsValid())
	{
		return Impl->GetDefaultCaptureDeviceInfo(OutInfo);
	}

	return false;
}

bool FAudioCapture::OpenDefaultCaptureStream(FAudioCaptureStreamParam& StreamParams)
{
	if (Impl.IsValid())
	{
		return Impl->OpenDefaultCaptureStream(StreamParams);
	}

	return false;
}

bool FAudioCapture::CloseStream()
{
	if (Impl.IsValid())
	{
		return Impl->CloseStream();
	}
	return false;
}

bool FAudioCapture::StartStream()
{
	if (Impl.IsValid())
	{
		return Impl->StartStream();
	}
	return false;
}

bool FAudioCapture::StopStream()
{
	if (Impl.IsValid())
	{
		return Impl->StopStream();
	}
	return false;
}

bool FAudioCapture::AbortStream()
{
	if (Impl.IsValid())
	{
		return Impl->AbortStream();
	}
	return false;
}

bool FAudioCapture::GetStreamTime(double& OutStreamTime)
{
	if (Impl.IsValid())
	{
		return Impl->GetStreamTime(OutStreamTime);
	}
	return false;
}

bool FAudioCapture::IsStreamOpen()
{
	if (Impl.IsValid())
	{
		return Impl->IsStreamOpen();
	}
	return false;
}

bool FAudioCapture::IsCapturing()
{
	if (Impl.IsValid())
	{
		return Impl->IsCapturing();
	}
	return false;
}

static const float MAX_LOOK_AHEAD_DELAY_MSEC = 100.9f;

FAudioCaptureSynth::FAudioCaptureSynth()
	: bSreamOpened(false)
	, bInitialized(false)
{
	CurrentInputWriteIndex.Set(1);
	CurrentOutputReadIndex.Set(0);
}

FAudioCaptureSynth::~FAudioCaptureSynth()
{
}

void FAudioCaptureSynth::OnAudioCapture(float* AudioData, int32 NumFrames, int32 NumChannels, double StreamTime, bool bOverflow)
{
	// If our read and write indices are the same, bump up our write index
	if (CurrentInputWriteIndex.GetValue() == CurrentOutputReadIndex.GetValue())
	{
		CurrentInputWriteIndex.Set((CurrentInputWriteIndex.GetValue() + 1) % NumBuffers);
	}

	// Get the write buffer
	int32 WriteIndex = CurrentInputWriteIndex.GetValue();
	TArray<float>& WriteBuffer = AudioDataBuffers[WriteIndex];

	int32 NumSamples = NumChannels * NumFrames;
	int32 Index = WriteBuffer.AddUninitialized(NumSamples);
	float* WriteBufferPtr = WriteBuffer.GetData();
	FMemory::Memcpy(&WriteBufferPtr[Index], AudioData, NumSamples * sizeof(float));
}

bool FAudioCaptureSynth::GetDefaultCaptureDeviceInfo(FCaptureDeviceInfo& OutInfo)
{
	return AudioCapture.GetDefaultCaptureDeviceInfo(OutInfo);
}

bool FAudioCaptureSynth::OpenDefaultStream()
{
	check(!AudioCapture.IsStreamOpen());

	FAudioCaptureStreamParam StreamParam;
	StreamParam.Callback = this;
	StreamParam.NumFramesDesired = 1024;

	return AudioCapture.OpenDefaultCaptureStream(StreamParam);
}

bool FAudioCaptureSynth::StartCapturing()
{
	for (int32 i = 0; i < NumBuffers; ++i)
	{
		AudioDataBuffers[i].Reset();
	}

	check(AudioCapture.IsStreamOpen());
	check(!AudioCapture.IsCapturing());

	AudioCapture.StartStream();

	return true;
}

void FAudioCaptureSynth::StopCapturing()
{
	check(AudioCapture.IsStreamOpen());
	check(AudioCapture.IsCapturing());
	AudioCapture.StopStream();
	bSreamOpened = false;
}

void FAudioCaptureSynth::AbortCapturing()
{
	AudioCapture.AbortStream();
	AudioCapture.CloseStream();
	bSreamOpened = false;
	bInitialized = false;
}

bool FAudioCaptureSynth::IsStreamOpen()
{
	return AudioCapture.IsStreamOpen();
}

bool FAudioCaptureSynth::IsCapturing()
{
	return AudioCapture.IsCapturing();
}

bool FAudioCaptureSynth::GetAudioData(TArray<float>& OutAudioData)
{
	// Reset the output buffer, hopefully the caller is re-using the same TArray to avoid thrashing.
	OutAudioData.Reset();

	bool bHadData = false;

	// If these indices are the same then we're retrieving audio faster than we're producing
	if (CurrentOutputReadIndex.GetValue() != CurrentInputWriteIndex.GetValue())
	{
		// Get the current buffer index value
		int32 CurrentIndex = CurrentOutputReadIndex.GetValue();

		// Get the audio data at this index.
		TArray<float>& Data = AudioDataBuffers[CurrentIndex];

		// If the data is non-empty, that means we've generated non-zero audio
		if (Data.Num())
		{
			// Append the data to the output buffer
			OutAudioData.Append(Data);

			// And now reset the data so if no audio is generated in the future, it'll be registered as a silent audio buffer
			Data.Reset();

			bHadData = true;
		}

		// Publish that we've read from the buffer so the next audio callback will now write into this buffer
		CurrentOutputReadIndex.Set((CurrentIndex + 1) % NumBuffers);
	}

	return bHadData;
}

} // namespace audio

void FAudioCaptureModule::StartupModule()
{
}

void FAudioCaptureModule::ShutdownModule()
{
}


IMPLEMENT_MODULE(FAudioCaptureModule, AudioCapture);