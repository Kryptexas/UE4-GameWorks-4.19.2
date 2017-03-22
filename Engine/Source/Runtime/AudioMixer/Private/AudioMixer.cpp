// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "HAL/RunnableThread.h"
#include "Misc/ConfigCacheIni.h"

DEFINE_STAT(STAT_AudioMixerRenderAudio);
DEFINE_STAT(STAT_AudioMixerSourceManagerUpdate);
DEFINE_STAT(STAT_AudioMixerSourceBuffers);
DEFINE_STAT(STAT_AudioMixerSourceEffectBuffers);
DEFINE_STAT(STAT_AudioMixerSourceOutputBuffers);
DEFINE_STAT(STAT_AudioMixerSubmixes);
DEFINE_STAT(STAT_AudioMixerSubmixChildren);
DEFINE_STAT(STAT_AudioMixerSubmixSource);
DEFINE_STAT(STAT_AudioMixerSubmixEffectProcessing);
DEFINE_STAT(STAT_AudioMixerMasterReverb);
DEFINE_STAT(STAT_AudioMixerMasterEQ);

namespace Audio
{
	/**
	 * IAudioMixerPlatformInterface
	 */

	IAudioMixerPlatformInterface::IAudioMixerPlatformInterface()
		: AudioRenderThread(nullptr)
		, AudioRenderEvent(nullptr)
		, AudioFadeEvent(nullptr)
		, CurrentBufferIndex(0)
		, LastError(TEXT("None"))
		, bAudioDeviceChanging(false)
		, bFadingIn(true)
		, bFadingOut(false)
		, bFadedOut(false)
		, FadeEnvelopeValue(0.0f)
	{
	}

	IAudioMixerPlatformInterface::~IAudioMixerPlatformInterface()
	{
		check(AudioStreamInfo.StreamState == EAudioOutputStreamState::Closed);
	}

	void IAudioMixerPlatformInterface::FadeOut()
	{
		if (FadeEnvelopeValue == 0.0f)
		{
			return;
		}

		bFadingOut = true;
		AudioFadeEvent->Wait();
	}

	void IAudioMixerPlatformInterface::PerformFades()
	{
		// Perform fade in and fade out global attenuation to avoid clicks/pops on startup/shutdown
		if (bFadingIn)
		{
			if (FadeEnvelopeValue >= 1.0f)
			{
				bFadingIn = false;
				FadeEnvelopeValue = 1.0f;
			}
			else
			{
				TArray<float>& Buffer = OutputBuffers[CurrentBufferIndex];
				const float Slope = 1.0f / Buffer.Num();
				for (int32 i = 0; i < Buffer.Num(); ++i)
				{
					Buffer[i] *= FadeEnvelopeValue;
					FadeEnvelopeValue += Slope;
				}
			}
		}

		if (bFadingOut)
		{
			if (FadeEnvelopeValue <= 0.0f)
			{
				bFadingOut = false;
				bFadedOut = true;
				FadeEnvelopeValue = 0.0f;

				// We're done fading out, trigger the thread waiting to fade to continue
				AudioFadeEvent->Trigger();
			}
			else
			{
				TArray<float>& Buffer = OutputBuffers[CurrentBufferIndex];
				const float Slope = 1.0f / Buffer.Num();
				for (int32 i = 0; i < Buffer.Num(); ++i)
				{
					Buffer[i] *= FadeEnvelopeValue;
					FadeEnvelopeValue -= Slope;
				}
			}
		}

		if (bFadedOut)
		{
			// If we're faded out, then just zero the data.
			TArray<float>& Buffer = OutputBuffers[CurrentBufferIndex];
			FPlatformMemory::Memzero(Buffer.GetData(), sizeof(float)*Buffer.Num());
		}
	}

	void IAudioMixerPlatformInterface::ReadNextBuffer()
	{
		// Don't read any more audio if we're not running
		if (AudioStreamInfo.StreamState != EAudioOutputStreamState::Running)
		{
			return;
		}


		// Don't submit anything if we're switching audio devices
		if (bAudioDeviceChanging)
		{
			return;
		}

		PerformFades();

		{
			FScopeLock Lock(&AudioRenderCritSect);

			SubmitBuffer(OutputBuffers[CurrentBufferIndex]);

			// Increment the buffer index
			CurrentBufferIndex = (CurrentBufferIndex + 1) % NumMixerBuffers;
		}

		if (AudioRenderEvent)
		{
			AudioRenderEvent->Trigger();
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

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Running;
		check(AudioRenderEvent == nullptr);
		AudioRenderEvent = FPlatformProcess::GetSynchEventFromPool();

		check(AudioFadeEvent == nullptr);
		AudioFadeEvent = FPlatformProcess::GetSynchEventFromPool();

		check(AudioRenderThread == nullptr);
		AudioRenderThread = FRunnableThread::Create(this, TEXT("AudioMixerRenderThread"), 0, TPri_AboveNormal);
	}

	void IAudioMixerPlatformInterface::StopGeneratingAudio()
	{
		// Stop the FRunnable thread
		{
			FScopeLock Lock(&AudioRenderCritSect);

			if (AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopped)
			{
				AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopping;
			}

			// Make sure the thread wakes up
			AudioRenderEvent->Trigger();
		}

		AudioRenderThread->WaitForCompletion();
		check(AudioStreamInfo.StreamState == EAudioOutputStreamState::Stopped);

		delete AudioRenderThread;
		AudioRenderThread = nullptr;

		FPlatformProcess::ReturnSynchEventToPool(AudioRenderEvent);
		AudioRenderEvent = nullptr;

		FPlatformProcess::ReturnSynchEventToPool(AudioFadeEvent);
		AudioFadeEvent = nullptr;
	}

	uint32 IAudioMixerPlatformInterface::Run()
	{
		while (AudioStreamInfo.StreamState != EAudioOutputStreamState::Stopping)
		{
			{
				FScopeLock Lock(&AudioRenderCritSect);

				SCOPE_CYCLE_COUNTER(STAT_AudioMixerRenderAudio);

				// Zero the current output buffer
				FPlatformMemory::Memzero(OutputBuffers[CurrentBufferIndex].GetData(), OutputBuffers[CurrentBufferIndex].Num() * sizeof(float));
				AudioStreamInfo.AudioMixer->OnProcessAudioStream(OutputBuffers[CurrentBufferIndex]);
			}

			// Note that this wait has to be outside the scope lock to avoid deadlocking
			AudioRenderEvent->Wait();
		}

		// Do one more process
		FPlatformMemory::Memzero(OutputBuffers[CurrentBufferIndex].GetData(), OutputBuffers[CurrentBufferIndex].Num() * sizeof(float));
		AudioStreamInfo.AudioMixer->OnProcessAudioStream(OutputBuffers[CurrentBufferIndex]);

		AudioStreamInfo.StreamState = EAudioOutputStreamState::Stopped;
		return 0;
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

	/** The default channel orderings to use when using pro audio interfaces while still supporting surround sound. */
	static EAudioMixerChannel::Type DefaultChannelOrder[AUDIO_MIXER_MAX_OUTPUT_CHANNELS];

	static void InitializeDefaultChannelOrder()
	{
		static bool bInitialized = false;
		if (bInitialized)
		{
			return;
		}

		bInitialized = true;

		// Create a hard-coded default channel order
		check(ARRAY_COUNT(DefaultChannelOrder) == AUDIO_MIXER_MAX_OUTPUT_CHANNELS);
		DefaultChannelOrder[0] = EAudioMixerChannel::FrontLeft;
		DefaultChannelOrder[1] = EAudioMixerChannel::FrontRight;
		DefaultChannelOrder[2] = EAudioMixerChannel::FrontCenter;
		DefaultChannelOrder[3] = EAudioMixerChannel::LowFrequency;
		DefaultChannelOrder[4] = EAudioMixerChannel::SideLeft;
		DefaultChannelOrder[5] = EAudioMixerChannel::SideRight;
		DefaultChannelOrder[6] = EAudioMixerChannel::BackLeft;
		DefaultChannelOrder[7] = EAudioMixerChannel::BackRight;

		bool bOverridden = false;
		EAudioMixerChannel::Type ChannelMapOverride[AUDIO_MIXER_MAX_OUTPUT_CHANNELS];
		for (int32 i = 0; i < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++i)
		{
			ChannelMapOverride[i] = DefaultChannelOrder[i];
		}

		// Now check the ini file to see if this is overridden
		for (int32 i = 0; i < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++i)
		{
			int32 ChannelPositionOverride = 0;

			const TCHAR* ChannelName = EAudioMixerChannel::ToString(DefaultChannelOrder[i]);
			if (GConfig->GetInt(TEXT("AudioDefaultChannelOrder"), ChannelName, ChannelPositionOverride, GEngineIni))
			{
				if (ChannelPositionOverride >= 0 && ChannelPositionOverride < AUDIO_MIXER_MAX_OUTPUT_CHANNELS)
				{
					bOverridden = true;
					ChannelMapOverride[ChannelPositionOverride] = DefaultChannelOrder[i];
				}
				else
				{
					UE_LOG(LogAudioMixer, Error, TEXT("Invalid channel index '%d' in AudioDefaultChannelOrder in ini file."), i);
					bOverridden = false;
					break;
				}
			}
		}

		// Now validate that there's no duplicates.
		if (bOverridden)
		{
			bool bIsValid = true;
			for (int32 i = 0; i < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++i)
			{
				for (int32 j = 0; j < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++j)
				{
					if (j != i && ChannelMapOverride[j] == ChannelMapOverride[i])
					{
						bIsValid = false;
						break;
					}
				}
			}

			if (!bIsValid)
			{
				UE_LOG(LogAudioMixer, Error, TEXT("Invalid channel index or duplicate entries in AudioDefaultChannelOrder in ini file."));
			}
			else
			{
				for (int32 i = 0; i < AUDIO_MIXER_MAX_OUTPUT_CHANNELS; ++i)
				{
					DefaultChannelOrder[i] = ChannelMapOverride[i];
				}
			}
		}
	}

	bool IAudioMixerPlatformInterface::GetChannelTypeAtIndex(const int32 Index, EAudioMixerChannel::Type& OutType)
	{
		InitializeDefaultChannelOrder();

		if (Index >= 0 && Index < AUDIO_MIXER_MAX_OUTPUT_CHANNELS)
		{
			OutType = DefaultChannelOrder[Index];
			return true;
		}
		return false;
	}
}