// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "AudioDecompress.h"
#include "AudioDevice.h"
#include "Sound/SoundWave.h"
#include "IAudioFormat.h"

/**
 * Worker for decompression on a separate thread
 */
FAsyncAudioDecompressWorker::FAsyncAudioDecompressWorker(USoundWave* InWave)
	: Wave(InWave)
	, AudioInfo(NULL)
{
	if (GEngine && GEngine->GetAudioDevice())
	{
		AudioInfo = GEngine->GetAudioDevice()->CreateCompressedAudioInfo(Wave);
	}
}

void FAsyncAudioDecompressWorker::DoWork( void )
{
	if (AudioInfo)
	{
		FSoundQualityInfo	QualityInfo = { 0 };

		// Parse the audio header for the relevant information
		if (AudioInfo->ReadCompressedInfo(Wave->ResourceData, Wave->ResourceSize, &QualityInfo))
		{

#if PLATFORM_ANDROID
			// Handle resampling
			if (QualityInfo.SampleRate > 48000)
			{
				UE_LOG( LogAudio, Warning, TEXT("Resampling file %s from %d"), *(Wave->GetName()), QualityInfo.SampleRate);
				UE_LOG( LogAudio, Warning, TEXT("  Size %d"), QualityInfo.SampleDataSize);
				uint32 SampleCount = QualityInfo.SampleDataSize / (QualityInfo.NumChannels * sizeof(uint16));
				QualityInfo.SampleRate = QualityInfo.SampleRate / 2;
				SampleCount /= 2;
				QualityInfo.SampleDataSize = SampleCount * QualityInfo.NumChannels * sizeof(uint16);
				AudioInfo->EnableHalfRate(true);
			}
#endif
			// Extract the data
			Wave->SampleRate = QualityInfo.SampleRate;
			Wave->NumChannels = QualityInfo.NumChannels;
			Wave->Duration = QualityInfo.Duration;

			Wave->RawPCMDataSize = QualityInfo.SampleDataSize;
			Wave->RawPCMData = ( uint8* )FMemory::Malloc( Wave->RawPCMDataSize );

			// Decompress all the sample data into preallocated memory
			AudioInfo->ExpandFile(Wave->RawPCMData, &QualityInfo);

			const SIZE_T ResSize = Wave->GetResourceSize(EResourceSizeMode::Exclusive);
			INC_DWORD_STAT_BY( STAT_AudioMemorySize, ResSize );
			INC_DWORD_STAT_BY( STAT_AudioMemory, ResSize );
		}

		// Delete the compressed data
		Wave->RemoveAudioResource();

		delete AudioInfo;
	}
}

// end
