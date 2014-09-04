// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetsPrivatePCH.h"


/* UMediaSoundWave structors
 *****************************************************************************/

UMediaSoundWave::UMediaSoundWave( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, AudioQueue(MakeShareable(new FMediaSampleQueue))
{
	bLooping = false;
	bProcedural = true;
	Duration = INDEFINITELY_LOOPING_DURATION;
}


UMediaSoundWave::~UMediaSoundWave( )
{
	if (AudioTrack.IsValid())
	{
		AudioTrack->RemoveSink(AudioQueue);
	}
}


/* UMediaSoundWave interface
 *****************************************************************************/

TSharedPtr<IMediaPlayer> UMediaSoundWave::GetMediaPlayer( ) const
{
	if (MediaAsset == nullptr)
	{
		return nullptr;
	}

	return MediaAsset->GetMediaPlayer();
}


void UMediaSoundWave::SetMediaAsset( UMediaAsset* InMediaAsset )
{
	MediaAsset = InMediaAsset;

	InitializeTrack();
}


/* USoundWave overrides
 *****************************************************************************/

int32 UMediaSoundWave::GeneratePCMData( uint8* PCMData, const int32 SamplesNeeded )
{
	// drain media sample queue
	TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Sample;

	while (AudioQueue->Dequeue(Sample))
	{
		QueuedAudio.Append(*Sample);
	}

	// get requested samples
	if (SamplesNeeded <= 0)
	{
		return 0;
	}

	const int32 SamplesAvailable = QueuedAudio.Num() / sizeof(int16);
	const int32 BytesToCopy = FMath::Min<int32>(SamplesNeeded, SamplesAvailable) * sizeof(int16);

	if (BytesToCopy > 0)
	{
		FMemory::Memcpy((void*)PCMData, &QueuedAudio[0], BytesToCopy);
		QueuedAudio.RemoveAt(0, BytesToCopy);
	}

	return BytesToCopy;
}


FByteBulkData* UMediaSoundWave::GetCompressedData( FName Format )
{
	return nullptr;
}


int32 UMediaSoundWave::GetResourceSizeForFormat( FName Format )
{
	return 0;
}


void UMediaSoundWave::InitAudioResource( FByteBulkData& CompressedData )
{
	check(false); // should never be pushing compressed data to this class
}


bool UMediaSoundWave::InitAudioResource( FName Format )
{
	return true;
}


/* UObject overrides
 *****************************************************************************/

void UMediaSoundWave::GetAssetRegistryTags( TArray<FAssetRegistryTag>& OutTags ) const
{
}


SIZE_T UMediaSoundWave::GetResourceSize(EResourceSizeMode::Type Mode)
{
	return 0;
}


void UMediaSoundWave::Serialize( FArchive& Ar )
{
	// do not call the USoundWave version of serialize
	USoundBase::Serialize(Ar);
}


/* UMediaSoundWave implementation
 *****************************************************************************/

void UMediaSoundWave::InitializeTrack( )
{
	// assign new media asset
	if (CurrentMediaAsset != MediaAsset)
	{
		if (CurrentMediaAsset != nullptr)
		{
			CurrentMediaAsset->OnMediaChanged().RemoveAll(this);
		}

		CurrentMediaAsset = MediaAsset;

		if (MediaAsset != nullptr)
		{
			MediaAsset->OnMediaChanged().AddUObject(this, &UMediaSoundWave::HandleMediaAssetMediaChanged);
		}	
	}

	// disconnect from current track
	if (AudioTrack.IsValid())
	{
		AudioTrack->RemoveSink(AudioQueue);
	}

	AudioTrack.Reset();

	// initialize from new track
	if (MediaAsset != nullptr)
	{
		IMediaPlayerPtr MediaPlayer = MediaAsset->GetMediaPlayer();

		if (MediaPlayer.IsValid())
		{
			AudioTrack = MediaPlayer->GetTrackSafe(AudioTrackIndex, EMediaTrackTypes::Audio);
		}
	}

	if (AudioTrack.IsValid())
	{
		NumChannels = AudioTrack->GetAudioDetails().GetNumChannels();
		SampleRate = AudioTrack->GetAudioDetails().GetSamplesPerSecond();
	}
	else
	{
		NumChannels = 0;
		SampleRate = 0;
	}

	// connect to new track
	if (AudioTrack.IsValid())
	{
		AudioTrack->AddSink(AudioQueue);
	}
}


/* UMediaSoundWave callbacks
 *****************************************************************************/

void UMediaSoundWave::HandleMediaAssetMediaChanged( )
{
	InitializeTrack();
}
