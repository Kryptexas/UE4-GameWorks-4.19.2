// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MediaSampleQueue.h"
#include "Sound/SoundWave.h"
#include "MediaSoundWave.generated.h"


/**
 * Implements a playable sound object for audio streams from media assets.
 */
UCLASS(hidecategories=(Compression, Sound, SoundWave, Subtitles))
class MEDIAASSETS_API UMediaSoundWave
	: public USoundWave
{
	GENERATED_UCLASS_BODY()

	/** The index of the media asset's audio track to get the wave data from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaAsset)
	int32 AudioTrackIndex;

	/** The media asset to stream audio from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MediaAsset)
	UMediaAsset* MediaAsset;

public:

	/** Destructor. */
	~UMediaSoundWave( );

public:

	/**
	 * Sets the media asset to be used for this texture.
	 *
	 * @param InMediaAsset The asset to set.
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaSound")
	void SetMediaAsset( UMediaAsset* InMediaAsset );

public:

	/**
	 * Gets the media player associated with the assigned media asset.
	 *
	 * @return Media player, or nullptr if no player is available.
	 */
	TSharedPtr<class IMediaPlayer> GetMediaPlayer( ) const;

public:

	// USoundWave overrides

	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded) override;
	virtual FByteBulkData* GetCompressedData(FName Format) override;
	virtual int32 GetResourceSizeForFormat( FName Format ) override;
	virtual void InitAudioResource( FByteBulkData& CompressedData ) override;
	virtual bool InitAudioResource( FName Format ) override;

public:

	// UObject overrides

	virtual void GetAssetRegistryTags( TArray<FAssetRegistryTag>& OutTags ) const override;
	virtual SIZE_T GetResourceSize( EResourceSizeMode::Type Mode ) override;
	virtual void Serialize( FArchive& Ar ) override;

protected:

	/** Initializes the audio stream. */
	void InitializeStream( );

private:

	// Callback for when the media player unloads media.
	void HandleMediaPlayerClosing( FString ClosingUrl );

	// Callback for when the media player unloads media.
	void HandleMediaPlayerOpened( FString OpenedUrl );

private:

	/** The audio sample queue. */
	FMediaSampleQueueRef AudioQueue;

	/** Holds the selected audio track. */
	IMediaTrackPtr AudioTrack;

	/** Holds the media asset currently being used. */
	UMediaAsset* CurrentMediaAsset;

	/** Holds queued audio samples. */
	TArray<uint8> QueuedAudio;
};
