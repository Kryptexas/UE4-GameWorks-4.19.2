// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceVoice.h"
#include "AudioMixerSource.h"
#include "AudioMixerSourceManager.h"

namespace Audio
{

	/**
	* FMixerSourceVoice Implementation
	*/

	FMixerSourceVoice::FMixerSourceVoice(FMixerDevice* InMixerDevice, FMixerSourceManager* InSourceManager)
		: SourceManager(InSourceManager)
		, MixerDevice(InMixerDevice)
		, NumBuffersQueued(0)
		, Pitch(-1.0f)
		, Volume(-1.0f)
		, Distance(-1.0f)
		, SourceId(INDEX_NONE)
		, bIsPlaying(false)
		, bIsPaused(false)
		, bIsActive(false)
	{
		AUDIO_MIXER_CHECK(SourceManager != nullptr);
	}

	FMixerSourceVoice::~FMixerSourceVoice()
	{
	}

	bool FMixerSourceVoice::Init(const FMixerSourceVoiceInitParams& InitParams)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (SourceManager->GetFreeSourceId(SourceId))
		{
			AUDIO_MIXER_CHECK(InitParams.BufferQueueListener != nullptr);
			AUDIO_MIXER_CHECK(InitParams.NumInputChannels > 0);

			for (int32 i = 0; i < InitParams.SubmixSends.Num(); ++i)
			{
				SubmixSends.Add(InitParams.SubmixSends[i].Submix->GetId(), InitParams.SubmixSends[i]);
			}

			SourceManager->InitSource(SourceId, InitParams);
			return true;
		}
		return false;
	}

	void FMixerSourceVoice::Release()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		SourceManager->ReleaseSourceId(SourceId);
	}

	void FMixerSourceVoice::SubmitBuffer(FMixerSourceBufferPtr InSourceVoiceBuffer)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		NumBuffersQueued.Increment();
		SourceManager->SubmitBuffer(SourceId, InSourceVoiceBuffer);
	}

	void FMixerSourceVoice::SubmitBufferAudioThread(FMixerSourceBufferPtr InSourceVoiceBuffer)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		NumBuffersQueued.Increment();
		SourceManager->SubmitBufferAudioThread(SourceId, InSourceVoiceBuffer);
	}

	int32 FMixerSourceVoice::GetNumBuffersQueued() const
	{
		return NumBuffersQueued.GetValue();
	}

	void FMixerSourceVoice::SetPitch(const float InPitch)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (Pitch != InPitch)
		{
			Pitch = InPitch;
			SourceManager->SetPitch(SourceId, InPitch);
		}
	}

	void FMixerSourceVoice::SetVolume(const float InVolume)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (Volume != InVolume)
		{
			Volume = InVolume;
			SourceManager->SetVolume(SourceId, InVolume);
		}
	}

	void FMixerSourceVoice::SetLPFFrequency(const float InLPFFrequency)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (LPFFrequency != InLPFFrequency)
		{
			LPFFrequency = InLPFFrequency;
			SourceManager->SetLPFFrequency(SourceId, LPFFrequency);
		}
	}

	void FMixerSourceVoice::SetChannelMap(TArray<float>& InChannelMap)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (ChannelMap != InChannelMap)
		{
			ChannelMap = InChannelMap;
			SourceManager->SetChannelMap(SourceId, InChannelMap);
		}
	}

	void FMixerSourceVoice::SetSpatializationParams(const FSpatializationParams& InParams)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		SourceManager->SetSpatializationParams(SourceId, InParams);
	}

	void FMixerSourceVoice::Play()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		bIsPlaying = true;
		bIsPaused = false;
		bIsActive = true;
		SourceManager->Play(SourceId);
	}

	void FMixerSourceVoice::Stop()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		bIsPlaying = false;
		bIsPaused = false;
		bIsActive = false;
		SourceManager->Stop(SourceId);
	}

	void FMixerSourceVoice::Pause()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		bIsPaused = true;
		bIsActive = false;
		SourceManager->Pause(SourceId);
	}

	bool FMixerSourceVoice::IsPlaying() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return bIsPlaying;
	}

	bool FMixerSourceVoice::IsPaused() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return bIsPaused;
	}

	bool FMixerSourceVoice::IsActive() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return bIsActive;
	}

	bool FMixerSourceVoice::IsDone() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return SourceManager->IsDone(SourceId);
	}

	int64 FMixerSourceVoice::GetNumFramesPlayed() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return SourceManager->GetNumFramesPlayed(SourceId);
	}

	void FMixerSourceVoice::MixOutputBuffers(TArray<float>& OutDryBuffer, TArray<float>& OutWetBuffer, const float DryLevel, const float WetLevel) const
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		return SourceManager->MixOutputBuffers(SourceId, OutDryBuffer, OutWetBuffer, DryLevel, WetLevel);
	}

	void FMixerSourceVoice::SetSubmixSendInfo(FMixerSubmixPtr Submix, const float DryLevel, const float WetLevel)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		FMixerSourceSubmixSend* SubmixSend = SubmixSends.Find(Submix->GetId());
		if (!SubmixSend)
		{
			FMixerSourceSubmixSend NewSubmixSend;
			NewSubmixSend.Submix = Submix;
			NewSubmixSend.WetLevel = WetLevel;
			NewSubmixSend.DryLevel = DryLevel;
			SubmixSends.Add(Submix->GetId(), NewSubmixSend);
		}
		else
		{
			SubmixSend->WetLevel = WetLevel;
			SubmixSend->DryLevel = DryLevel;
		}

		SourceManager->SetSubmixSendInfo(SourceId, Submix, DryLevel, WetLevel);
	}


}