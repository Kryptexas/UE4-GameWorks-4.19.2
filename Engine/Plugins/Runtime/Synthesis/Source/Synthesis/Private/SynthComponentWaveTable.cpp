// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SynthComponents/SynthComponentWaveTable.h"
#include "AudioDecompress.h"
#include "AudioDevice.h"
#include "DSP/SampleRateConverter.h"
#include "UObject/Package.h"

USynthSamplePlayer::USynthSamplePlayer(const FObjectInitializer& ObjInitializer)
	: Super(ObjInitializer)
	, SoundWave(nullptr)
	, SampleDurationSec(0.0f)
	, SamplePlaybackProgressSec(0.0F)
	, bTransferPendingToSound(false)
	, bIsLoaded(false)
	, bIsLoadedBroadcast(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

USynthSamplePlayer::~USynthSamplePlayer()
{
}

void USynthSamplePlayer::Init(const int32 SampleRate)
{
	NumChannels = 2;

	SampleBufferReader.Init(SampleRate);
	bIsLoaded = false;
}

void USynthSamplePlayer::SetPitch(float InPitch, float InTimeSec)
{
	SynthCommand([this, InPitch, InTimeSec]()
	{
		SampleBufferReader.SetPitch(InPitch, InTimeSec);
	});
}

void USynthSamplePlayer::SeekToTime(float InTimeSecs, ESamplePlayerSeekType InSeekType)
{
	Audio::ESeekType::Type SeekType;
	switch (InSeekType)
	{
		default:
		case ESamplePlayerSeekType::FromBeginning:
			SeekType = Audio::ESeekType::FromBeginning;
			break;

		case ESamplePlayerSeekType::FromCurrentPosition:
			SeekType = Audio::ESeekType::FromCurrentPosition;
			break;

		case ESamplePlayerSeekType::FromEnd:
			SeekType = Audio::ESeekType::FromEnd;
			break;
	}

	SynthCommand([this, InTimeSecs, SeekType]()
	{
		SampleBufferReader.SeekTime(InTimeSecs, SeekType);
	});
}

void USynthSamplePlayer::SetScrubMode(bool bScrubMode)
{
	SynthCommand([this, bScrubMode]()
	{
		SampleBufferReader.SetScrubMode(bScrubMode);
	});
}

void USynthSamplePlayer::SetScrubTimeWidth(float InScrubTimeWidthSec)
{
	SynthCommand([this, InScrubTimeWidthSec]()
	{
		SampleBufferReader.SetScrubTimeWidth(InScrubTimeWidthSec);
	});
}

float USynthSamplePlayer::GetSampleDuration() const
{
	return SampleDurationSec;
}

bool USynthSamplePlayer::IsLoaded() const
{
	return bIsLoaded;
}

float USynthSamplePlayer::GetCurrentPlaybackProgressTime() const
{
	return SamplePlaybackProgressSec;
}

float USynthSamplePlayer::GetCurrentPlaybackProgressPercent() const
{
	if (SampleDurationSec > 0.0f)
	{
		return SamplePlaybackProgressSec / SampleDurationSec;
	}
	return 0.0f;
}

void USynthSamplePlayer::SetSoundWave(USoundWave* InSoundWave)
{
	PendingSoundWaveSet = DuplicateObject<USoundWave>(InSoundWave, GetTransientPackage());
	PendingSoundWaveSet->AddToRoot();

	SynthCommand([this]()
	{
		SampleBufferReader.ClearBuffer();

		SampleBuffer.Init(GetAudioDevice());
		SampleBuffer.LoadSoundWave(PendingSoundWaveSet);

		bTransferPendingToSound = true;
	});
}

void USynthSamplePlayer::OnRegister()
{
	Super::OnRegister();

	SetComponentTickEnabled(true);
	RegisterComponent();

	if (SoundWave)
	{
		SoundWaveCopy = DuplicateObject<USoundWave>(SoundWave, GetTransientPackage());
		SoundWaveCopy->AddToRoot();

		// Load the sound wave right here synchronously
		SampleBuffer.Init(GetAudioDevice());
		SampleBuffer.LoadSoundWave(SoundWaveCopy);
	}
}

void USynthSamplePlayer::OnUnregister()
{
	Super::OnUnregister();

	if (IsValid(SoundWaveCopy))
	{
		SoundWaveCopy->RemoveFromRoot();
		SoundWaveCopy = nullptr;
	}
}

void USynthSamplePlayer::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (bIsLoaded && !bIsLoadedBroadcast)
	{
		bIsLoadedBroadcast = true;
		OnSampleLoaded.Broadcast();
	}

	OnSamplePlaybackProgress.Broadcast(GetCurrentPlaybackProgressTime(), GetCurrentPlaybackProgressPercent());

	if (bTransferPendingToSound)
	{
		bTransferPendingToSound = false;
		check(PendingSoundWaveSet);
		if (SoundWaveCopy)
		{
			SoundWaveCopy->RemoveFromRoot();
		}
		SoundWaveCopy = PendingSoundWaveSet;
		PendingSoundWaveSet = nullptr;
	}
}

void USynthSamplePlayer::OnGenerateAudio(TArray<float>& OutAudio)
{
	SampleBuffer.UpdateLoading();

	if (SampleBuffer.IsLoaded() && !SampleBufferReader.HasBuffer())
	{
		const int16* BufferData = SampleBuffer.GetData();
		const int32 BufferNumSamples = SampleBuffer.GetNumSamples();
		const int32 BufferNumChannels = SampleBuffer.GetNumChannels();
		const int32 BufferSampleRate = SampleBuffer.GetSampleRate();
		SampleBufferReader.SetBuffer(&BufferData, BufferNumSamples, BufferNumChannels, BufferSampleRate);
		SampleDurationSec = (float)(BufferNumSamples) / BufferSampleRate;
		bIsLoaded = true;
	}

	if (bIsLoaded)
	{
		const int32 NumFrames = OutAudio.Num() / NumChannels;
		SampleBufferReader.Generate(OutAudio, NumFrames, NumChannels, true);
		SamplePlaybackProgressSec = SampleBufferReader.GetPlaybackProgress();
	}
	else
	{
		for (int32 Sample = 0; Sample < OutAudio.Num(); ++Sample)
		{
			OutAudio[Sample] = 0.0f;
		}
	}
}
