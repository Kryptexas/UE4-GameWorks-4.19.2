// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/** 
 * Playable sound object for wave files that are streamed, particularly VOIP
 */

#pragma once
#include "SoundWaveStreaming.generated.h"

DECLARE_DELEGATE_TwoParams( FOnSoundWaveStreamingUnderflow, class USoundWaveStreaming*, int32 );

UCLASS(MinimalAPI)
class USoundWaveStreaming : public USoundWave
{
	GENERATED_UCLASS_BODY()

private:
	TArray<uint8> QueuedAudio;

public:

	// Begin UObject interface. 
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface. 

	// Begin USoundWave interface.
	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded) OVERRIDE;
	virtual FByteBulkData* GetCompressedData(FName Format) OVERRIDE;
	virtual void InitAudioResource( FByteBulkData& CompressedData ) OVERRIDE;
	virtual bool InitAudioResource(FName Format) OVERRIDE;
	virtual int32 GetResourceSizeForFormat(FName Format) OVERRIDE;
	// End USoundWave interface.


	/** Add data to the FIFO that feeds the audio device. */
	void ENGINE_API QueueAudio(const uint8* AudioData, const int32 BufferSize);

	/** Remove all queued data from the FIFO. This is only necessary if you want to start over, or GeneratePCMData() isn't going to be called, since that will eventually drain it. */
	void ResetAudio();

	/** Query bytes queued for playback */
	int32 ENGINE_API GetAvailableAudioByteCount();

	/** Called when GeneratePCMData is called but not enough data is available. Allows more data to be added, and will try again */
	FOnSoundWaveStreamingUnderflow OnSoundWaveStreamingUnderflow;

};