// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPCH.h"
#include "AudioMixerDevice.h"
#include "AudioMixer.h"

namespace Audio
{
	/**
	 * FAudioMixerCallbackWorker
	 */

	FAudioMixerCallbackWorker::FAudioMixerCallbackWorker(IAudioMixerPlatformInterface* InMixerPlatform, TArray<float>* InOutBuffer)
		: MixerPlatform(InMixerPlatform)
		, OutBuffer(InOutBuffer)
	{
		check(MixerPlatform);
		check(OutBuffer);
	}

	void FAudioMixerCallbackWorker::DoWork()
	{
		// Zero the buffer before processing a new one
		check(OutBuffer);
		FPlatformMemory::Memzero((void*)OutBuffer->GetData(), OutBuffer->Num() * sizeof(float));

		// Call into multi-platform mixer for new buffer
		MixerPlatform->GenerateBuffer(*OutBuffer);
	}

	/**
	 * IAudioMixerPlatformInterface
	 */

	IAudioMixerPlatformInterface::IAudioMixerPlatformInterface()
		: CallbackTask(nullptr)
		, CurrentBufferIndex(0)
		, LastError(TEXT("None"))
	{
	}

	void IAudioMixerPlatformInterface::GenerateBuffer(TArray<float>& Buffer)
	{
		// Call into platform independent code to process next stream
		AudioStreamInfo.AudioMixer->OnProcessAudioStream(Buffer);
	}

	void IAudioMixerPlatformInterface::ReadNextBuffer()
	{
		if (AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopping)
		{
			// Make sure we have a pending task in the read index
			check(CallbackTask);

			// Make sure it's done
			CallbackTask->EnsureCompletion();

			// Get the processed buffer
			const TArray<float>* Buffer = CallbackTask->GetTask().GetBuffer();

			// Submit the buffer to the platform output device
			check(Buffer);
			SubmitBuffer(*Buffer);

			// Clean up the task
			delete CallbackTask;

			// Create a new task and start it
			CallbackTask = new FAsyncAudioMixerCallbackTask(this, &OutputBuffers[CurrentBufferIndex]);
			CallbackTask->StartBackgroundTask();

			// Increment the buffer index
			CurrentBufferIndex = (CurrentBufferIndex + 1) % NumMixerBuffers;
		}
	}

	void IAudioMixerPlatformInterface::BeginGeneratingAudio()
	{
		FAudioPlatformDeviceInfo & DeviceInfo = AudioStreamInfo.DeviceInfo;

		// Setup the output buffers
		for (int32 Index = 0; Index < NumMixerBuffers; ++Index)
		{
			OutputBuffers[Index].SetNumZeroed(DeviceInfo.NumSamples);
		}

		// Submit the first empty buffer. This will begin audio callback notifications on some platforms.
		SubmitBuffer(OutputBuffers[CurrentBufferIndex++]);

		// Launch the task to begin generating audio
		CallbackTask = new FAsyncAudioMixerCallbackTask(this, &OutputBuffers[CurrentBufferIndex]);
		CallbackTask->StartBackgroundTask();
	}


	uint32 IAudioMixerPlatformInterface::GetNumBytesForFormat(const EAudioMixerStreamDataFormat::Type DataFormat)
	{
		switch (DataFormat)
		{
			case EAudioMixerStreamDataFormat::Float:	return 4;
			case EAudioMixerStreamDataFormat::Double:	return 8;
			case EAudioMixerStreamDataFormat::Int16:	return 2;
			case EAudioMixerStreamDataFormat::Int24:	return 3;
			case EAudioMixerStreamDataFormat::Int32:	return 4;

			default:
			checkf(false, TEXT("Unknown or unsupported data format."))
				return 0;
		}
	}
}