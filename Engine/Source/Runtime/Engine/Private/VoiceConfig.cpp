// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VoiceConfig.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

TMap<FUniqueNetIdWrapper, UVOIPTalker*> UVOIPStatics::VoiceTalkerMap;

int32 UVOIPStatics::GetVoiceSampleRate()
{
#if USE_DEFAULT_VOICE_SAMPLE_RATE
	return (int32) 16000;
#else
	static bool bRetrievedSampleRate = false;
	static int32 SampleRate = 0;

	if (bRetrievedSampleRate)
	{
		return SampleRate;
	}

	static FString DesiredSampleRateStr;
	bRetrievedSampleRate = true;

	if (GConfig->GetString(TEXT("/Script/Engine.AudioSettings"), TEXT("VoiPSampleRate"), DesiredSampleRateStr, GEngineIni))
	{
		SampleRate = (int32)GetEnumValueFromString<EVoiceSampleRate>(TEXT("EVoiceSampleRate"), DesiredSampleRateStr);
		if (SampleRate > 0)
		{		
			return SampleRate;
		}
	}

	// If we've made it here, we couldn't find a specified sample rate from properties.
	return SampleRate = (int32) EVoiceSampleRate::Low16000Hz;
#endif
}

uint32 UVOIPStatics::GetMaxVoiceDataSize()
{
	int32 SampleRate = GetVoiceSampleRate();
	// This max voice data size is based on approximations of how large the
	// encoded opus audio buffer will be.
	switch (SampleRate)
	{
		case 24000:
		{
			return 14 * 1024;
		}
		case 48000:
		{
			return 32 * 1024;
		}
		default:
		case 16000:
		{
			return 8 * 1024;
		}
	}
}

uint32 UVOIPStatics::GetMaxUncompressedVoiceDataSizePerChannel()
{
	// Maximum amount of input samples we allocate for every time we get input audio from the mic on the game thread.
	// This amounts to approximates a second of audio for the minimum voice engine tick frequency.
	// At 48 kHz, DirectSound will occasionally have to load up to 256 samples into the overflow buffer.
	// This is the reason for the added 1024 bytes.
	return GetVoiceSampleRate() * sizeof(uint16) + 1024;
}

uint32 UVOIPStatics::GetMaxCompressedVoiceDataSize()
{
	int32 SampleRate = GetVoiceSampleRate();
	// These values are carried over from MAX_COMPRESSED_VOICE_DATA_SIZE in previous revisions
	// of the Voice Engine code.
	switch (SampleRate)
	{
		case 24000:
		{
			return 2 * 1024;
		}
		case 48000:
		{
			return 4 * 1024;
		}
		default:
		case 16000:
		{
			return 1 * 1024;
		}
	}
}

float UVOIPStatics::GetRemoteTalkerTimeoutDuration()
{
	return 1.0f;
}

EAudioEncodeHint UVOIPStatics::GetAudioEncodingHint()
{
	// This may be exposed as a project settings in the future.
	return EAudioEncodeHint::VoiceEncode_Voice;
}

float UVOIPStatics::GetBufferingDelay()
{
	static bool bRetrievedBufferingDelay = false;
	static float DesiredBufferDelay = 0.2f;
	if (!bRetrievedBufferingDelay)
	{
		bRetrievedBufferingDelay = true;
		float SettingsBufferDelay;
		if (GConfig->GetFloat(TEXT("/Script/Engine.AudioSettings"), TEXT("VoipBufferingDelay"), SettingsBufferDelay, GEngineIni) && SettingsBufferDelay > 0.0f)
		{
			DesiredBufferDelay = SettingsBufferDelay;
		}
	}
	return DesiredBufferDelay;
}

int32 UVOIPStatics::GetNumBufferedPackets()
{
	// Evaluate number of packets as the total number of samples we'll need to buffer divided by
	// the number of samples per buffer, plus an arbitrary amount of packets to compensate for jitter.
	static int32 NumBufferedPackets = (GetBufferingDelay() * GetVoiceSampleRate()) / (GetMaxVoiceDataSize() / sizeof(float)) + MicSilenceDetectionConfig::PacketBufferSlack;
	return NumBufferedPackets;
}

APlayerState* UVOIPStatics::GetPlayerStateFromUniqueNetId(UWorld* InWorld, const FUniqueNetIdWrapper& InPlayerId)
{
	AGameStateBase* InBase = InWorld->GetGameState();
	TArray<APlayerState*>& Players = InBase->PlayerArray;
	for (APlayerState* Player : Players)
	{
		if (Player->UniqueId == InPlayerId)
		{
			return Player;
		}
	}

	return nullptr;
}

void UVOIPStatics::SetMicThreshold(float InThreshold)
{
	MicSilenceDetectionConfig::Threshold = InThreshold;
}

void UVOIPStatics::SetVOIPTalkerForPlayer(const FUniqueNetIdWrapper& InPlayerId, UVOIPTalker* InTalker)
{
	VoiceTalkerMap.FindOrAdd(InPlayerId);
	VoiceTalkerMap[InPlayerId] = InTalker;
}

UVOIPTalker* UVOIPStatics::GetVOIPTalkerForPlayer(const FUniqueNetIdWrapper& InUniqueId,  FVoiceSettings& OutSettings, UWorld* InWorld, APlayerState** OutPlayerState /*= nullptr*/)
{
	UVOIPTalker** FoundTalker = nullptr;

	if (InWorld != nullptr && OutPlayerState != nullptr)
	{
		*OutPlayerState = GetPlayerStateFromUniqueNetId(InWorld, InUniqueId);
	}

	FoundTalker = VoiceTalkerMap.Find(InUniqueId);
	
	if (FoundTalker != nullptr)
	{
		OutSettings = (*FoundTalker)->Settings;
		return *FoundTalker;
	}
	else
	{
		return nullptr;
	}
}

UVOIPTalker* UVOIPStatics::GetVOIPTalkerForPlayer(const FUniqueNetIdWrapper& InPlayerId)
{
	UVOIPTalker** FoundTalker = VoiceTalkerMap.Find(InPlayerId);

	if (FoundTalker)
	{
		return *FoundTalker;
	}
	else
	{
		return nullptr;
	}
}

bool UVOIPStatics::IsVOIPTalkerStillAlive(UVOIPTalker* InTalker)
{
	for (auto& TalkerElement : VoiceTalkerMap)
	{
		if(TalkerElement.Value == InTalker)
		{
			return true;
		}
	}

	return false;
}

void UVOIPStatics::ResetPlayerVoiceTalker(APlayerState* InPlayerState)
{
	if (InPlayerState && InPlayerState->UniqueId.IsValid())
	{
		VoiceTalkerMap.Remove(InPlayerState->UniqueId);
	}
}

void UVOIPStatics::ResetPlayerVoiceTalker(const FUniqueNetIdWrapper& InPlayerId)
{
	VoiceTalkerMap.Remove(InPlayerId);
}

void UVOIPStatics::ClearAllSettings()
{
	VoiceTalkerMap.Empty();
}

UVOIPTalker::UVOIPTalker(const FObjectInitializer& ObjectInitializer)
	: PlayerId()
	, CachedVolumeLevel(0.0f)
	, bIsRegistered(false)
{
}

UVOIPTalker::~UVOIPTalker()
{
	if (bIsRegistered)
	{
		UnregisterFromVoiceTalkerMap();
	}
}

UVOIPTalker* UVOIPTalker::CreateTalkerForPlayer(APlayerState* OwningState)
{
	UVOIPTalker* NewTalker = NewObject<UVOIPTalker>();
	if (NewTalker != nullptr)
	{
		NewTalker->RegisterWithPlayerState(OwningState);
	}

	return NewTalker;
}

void UVOIPTalker::RegisterWithPlayerState(APlayerState* OwningState)
{
	checkf(OwningState != nullptr, TEXT("RegisterWithPlayerState called with null pointer to OwningState"));
	if (bIsRegistered)
	{
		UnregisterFromVoiceTalkerMap();
	}

	if (OwningState->UniqueId.IsValid())
	{
		UVOIPStatics::SetVOIPTalkerForPlayer(OwningState->UniqueId, this);
		PlayerId = OwningState->UniqueId;
		bIsRegistered = true;
	}
}

void UVOIPTalker::OnAudioComponentEnvelopeValue(const UAudioComponent* InAudioComponent, const float EnvelopeValue)
{
	CachedVolumeLevel = EnvelopeValue;
}

void UVOIPTalker::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (bIsRegistered)
	{
		UnregisterFromVoiceTalkerMap();
	}
}

float UVOIPTalker::GetVoiceLevel()
{
	return CachedVolumeLevel;
}

void UVOIPTalker::BPOnTalkingBegin_Implementation(UAudioComponent* AudioComponent)
{

}

void UVOIPTalker::BPOnTalkingEnd_Implementation()
{

}

void UVOIPTalker::UnregisterFromVoiceTalkerMap()
{
	UVOIPStatics::ResetPlayerVoiceTalker(PlayerId);
	bIsRegistered = false;
}
