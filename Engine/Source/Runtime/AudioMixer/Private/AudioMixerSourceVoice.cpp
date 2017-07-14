// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceVoice.h"
#include "AudioMixerSource.h"
#include "AudioMixerSourceManager.h"
#include "AudioMixerDevice.h"

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
		, LPFFrequency(-1.0f)
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

	void FMixerSourceVoice::SubmitBuffer(FMixerSourceBufferPtr InSourceVoiceBuffer, const bool bSubmitSynchronously)
	{
		NumBuffersQueued.Increment();
		SourceManager->SubmitBuffer(SourceId, InSourceVoiceBuffer, bSubmitSynchronously);
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

	void FMixerSourceVoice::SetChannelMap(TArray<float>& InChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		ChannelMap = InChannelMap;
		SourceManager->SetChannelMap(SourceId, InChannelMap, bInIs3D, bInIsCenterChannelOnly);
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

	bool FMixerSourceVoice::IsSourceEffectTailsDone() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return SourceManager->IsEffectTailsDone(SourceId);
	}

	bool FMixerSourceVoice::NeedsSpeakerMap() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return SourceManager->NeedsSpeakerMap(SourceId);
	}

	int64 FMixerSourceVoice::GetNumFramesPlayed() const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		return SourceManager->GetNumFramesPlayed(SourceId);
	}

	void FMixerSourceVoice::MixOutputBuffers(AlignedFloatBuffer& OutWetBuffer, const float SendLevel) const
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		return SourceManager->MixOutputBuffers(SourceId, OutWetBuffer, SendLevel);
	}

	void FMixerSourceVoice::SetSubmixSendInfo(FMixerSubmixPtr Submix, const float SendLevel)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		FMixerSourceSubmixSend* SubmixSend = SubmixSends.Find(Submix->GetId());
		bool bChanged = false;
		if (!SubmixSend)
		{
			FMixerSourceSubmixSend NewSubmixSend;
			NewSubmixSend.Submix = Submix;
			NewSubmixSend.SendLevel = SendLevel;
			NewSubmixSend.bIsMainSend = false;
			SubmixSends.Add(Submix->GetId(), NewSubmixSend);
			bChanged = true;
		}
		else
		{
			if (!FMath::IsNearlyEqual(SubmixSend->SendLevel, SendLevel))
			{
				SubmixSend->SendLevel = SendLevel;
				bChanged = true;
			}
		}

		if (bChanged)
		{
			SourceManager->SetSubmixSendInfo(SourceId, Submix, SendLevel);
		}
	}


}