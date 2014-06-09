// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "TargetPlatform.h"
#include "AudioDecompress.h"
#include "AudioFormatOpus.h"

// Need to define this so that resampler.h compiles - probably a way around this somehow
#define OUTSIDE_SPEEX

#include "opus.h"
#include "speex_resampler.h"

/** Use UE4 memory allocation or Opus */
#define USE_UE4_MEM_ALLOC 1
#define SAMPLE_SIZE			( ( uint32 )sizeof( short ) )

static FName NAME_OPUS(TEXT("OPUS"));

/**
 * IAudioFormat, audio compression abstraction
**/
class FAudioFormatOpus : public IAudioFormat
{
	enum
	{
		/** Version for OPUS format, this becomes part of the DDC key. */
		UE_AUDIO_OPUS_VER = 1,
	};

public:
	virtual bool AllowParallelBuild() const
	{
		return false;
	}

	virtual uint16 GetVersion(FName Format) const OVERRIDE
	{
		check(Format == NAME_OPUS);
		return UE_AUDIO_OPUS_VER;
	}


	virtual void GetSupportedFormats(TArray<FName>& OutFormats) const
	{
		OutFormats.Add(NAME_OPUS);
	}

	virtual bool Cook(FName Format, const TArray<uint8>& SrcBuffer, FSoundQualityInfo& QualityInfo, TArray<uint8>& CompressedDataStore) const
	{
		check(Format == NAME_OPUS);

		// Constant sample rate for simplicity
		const int32 kOpusSampleRate = 48000;
		// Frame size must be one of 2.5, 5, 10, 20, 40 or 60 ms
		const int32 kOpusFrameSizeMs = 60;
		// Calculate frame size required by Opus
		const int32 kOpusFrameSizeSamples = (kOpusSampleRate * kOpusFrameSizeMs) / 1000;
		const uint32 kSampleStride = SAMPLE_SIZE * QualityInfo.NumChannels;
		const int32 kBytesPerFrame = kOpusFrameSizeSamples * kSampleStride;

		// Check whether source has compatible sample rate
		TArray<uint8> SrcBufferCopy;
		if (QualityInfo.SampleRate != kOpusSampleRate)
		{
			// Initialize resampler to convert to desired rate for Opus
			int32 err = 0;
			SpeexResamplerState* resampler = speex_resampler_init(QualityInfo.NumChannels, QualityInfo.SampleRate, kOpusSampleRate, SPEEX_RESAMPLER_QUALITY_DESKTOP, &err);
			if (err != RESAMPLER_ERR_SUCCESS)
			{
				return false;
			}

			// Calculate extra space required for sample rate
			float Duration = (float)SrcBuffer.Num() / (QualityInfo.SampleRate * kSampleStride);
			int32 SafeCopySize = (Duration + 1) * kOpusSampleRate * kSampleStride;
			SrcBufferCopy.AddUninitialized(SafeCopySize);
			uint32 SrcSamples = SrcBuffer.Num() / kSampleStride;
			uint32 CopySamples = SrcBufferCopy.Num() / kSampleStride;

			// Do resampling and check results
			if (QualityInfo.NumChannels == 1)
			{
				err = speex_resampler_process_int(resampler, 0, (const short*)(SrcBuffer.GetTypedData()), &SrcSamples, (short*)(SrcBufferCopy.GetTypedData()), &CopySamples);
			}
			else
			{
				err = speex_resampler_process_interleaved_int(resampler, (const short*)(SrcBuffer.GetTypedData()), &SrcSamples, (short*)(SrcBufferCopy.GetTypedData()), &CopySamples);
			}

			speex_resampler_destroy(resampler);
			if (err != RESAMPLER_ERR_SUCCESS)
			{
				return false;
			}

			int32 WrittenBytes = (int32)(CopySamples * kSampleStride);
			if (WrittenBytes < SrcBufferCopy.Num())
			{
				SrcBufferCopy.SetNum(WrittenBytes);
			}
		}
		else
		{
			// Take a copy of the source regardless
			SrcBufferCopy = SrcBuffer;
		}

		// Initialise the Opus encoder
		OpusEncoder* Encoder = NULL;
		int32 EncError = 0;
#if USE_UE4_MEM_ALLOC
		int32 EncSize = opus_encoder_get_size(QualityInfo.NumChannels);
		Encoder = (OpusEncoder*)FMemory::Malloc(EncSize);
		EncError = opus_encoder_init(Encoder, kOpusSampleRate, QualityInfo.NumChannels, OPUS_APPLICATION_AUDIO);
#else
		Encoder = opus_encoder_create(kOpusSampleRate, QualityInfo.NumChannels, OPUS_APPLICATION_AUDIO, &EncError);
#endif
		if (EncError != OPUS_OK)
		{
			Destroy(Encoder);
			return false;
		}

		// Basic attempt at mapping quality to bit rate range similar to what vorbis would give
		int32 BitRate = FMath::GetMappedRangeValue(FVector2D(1, 100), FVector2D(64000, 400000), QualityInfo.Quality);
		opus_encoder_ctl(Encoder, OPUS_SET_BITRATE(BitRate));

		// Create a buffer to store compressed data
		CompressedDataStore.Empty();
		FMemoryWriter CompressedData(CompressedDataStore);
		int32 SrcBufferOffset = 0;

		// Calc frame and sample count
		int32 FramesToEncode = SrcBufferCopy.Num() / kBytesPerFrame;
		uint32 TrueSampleCount = SrcBufferCopy.Num() / kSampleStride;

		// Pad the end of data with zeroes if it isn't exactly the size of a frame.
		if (SrcBufferCopy.Num() % kBytesPerFrame != 0)
		{
			int32 FrameDiff = kBytesPerFrame - (SrcBufferCopy.Num() % kBytesPerFrame);
			SrcBufferCopy.AddZeroed(FrameDiff);
			FramesToEncode++;
		}

		// Store True Sample Count, Number of channels and Frames to Encode first
		CompressedData.Serialize(&TrueSampleCount, sizeof(uint32));
		check(QualityInfo.NumChannels < MAX_uint8)
		uint8 SerializedChannels = QualityInfo.NumChannels;
		CompressedData.Serialize(&SerializedChannels, sizeof(uint8));
		check(FramesToEncode < MAX_uint16);
		uint16 SerializedFrames = FramesToEncode;
		CompressedData.Serialize(&SerializedFrames, sizeof(uint16));

		// Temporary storage with more than enough to store any compressed frame
		TArray<uint8> TempCompressedData;
		TempCompressedData.AddUninitialized(kBytesPerFrame);

		while (SrcBufferOffset < SrcBufferCopy.Num())
		{
			int32 CompressedLength = opus_encode(Encoder, (const opus_int16*)(SrcBufferCopy.GetTypedData() + SrcBufferOffset), kOpusFrameSizeSamples, TempCompressedData.GetTypedData(), TempCompressedData.Num());

			if (CompressedLength < 0)
			{
				//const char* ErrorStr = opus_strerror(CompressedLength);
				//UE_LOG(LogAudio, Warning, TEXT("Failed to encode: [%d] %s"), CompressedLength, ANSI_TO_TCHAR(ErrorStr));

				Destroy(Encoder);

				CompressedDataStore.Empty();
				return false;
			}
			else
			{
				// Store frame length and copy compressed data before incrementing pointers
				check(CompressedLength < MAX_uint16);
				uint16 SerializedFrameLength = CompressedLength;
				CompressedData.Serialize(&SerializedFrameLength, sizeof(uint16));
				CompressedData.Serialize(TempCompressedData.GetTypedData(), CompressedLength);
				SrcBufferOffset += kBytesPerFrame;
			}
		}

		Destroy(Encoder);

		return CompressedDataStore.Num() > 0;
	}

	virtual bool CookSurround(FName Format, const TArray<TArray<uint8> >& SrcBuffers, FSoundQualityInfo& QualityInfo, TArray<uint8>& CompressedDataStore) const
	{
		check(Format == NAME_OPUS);
		{
			// Create a buffer to store compressed data
			CompressedDataStore.Empty();
			FMemoryWriter CompressedData(CompressedDataStore);
			uint32 BufferOffset = 0;
		}
		return CompressedDataStore.Num() > 0;

	}

	virtual int32 Recompress(FName Format, const TArray<uint8>& SrcBuffer, FSoundQualityInfo& QualityInfo, TArray<uint8>& OutBuffer) const
	{
		check(Format == NAME_OPUS);
		FOpusAudioInfo	AudioInfo;

		// Cannot quality preview multichannel sounds
		if( QualityInfo.NumChannels > 2 )
		{
			return 0;
		}

		TArray<uint8> CompressedDataStore;
		if( !Cook( Format, SrcBuffer, QualityInfo, CompressedDataStore ) )
		{
			return 0;
		}

		// Parse the opus header for the relevant information
		if( !AudioInfo.ReadCompressedInfo( CompressedDataStore.GetTypedData(), CompressedDataStore.Num(), &QualityInfo ) )
		{
			return 0;
		}

		// Decompress all the sample data
		OutBuffer.Empty(QualityInfo.SampleDataSize);
		OutBuffer.AddZeroed(QualityInfo.SampleDataSize);
		AudioInfo.ExpandFile( OutBuffer.GetTypedData(), &QualityInfo );

		return CompressedDataStore.Num();
	}

	virtual bool SplitDataForStreaming(const TArray<uint8>& SrcBuffer, TArray<TArray<uint8>>& OutBuffers) const OVERRIDE
	{
		// 16K chunks - don't really have an idea of what's best for loading from disc yet
		const int32 kMaxChunkSizeBytes = 16384;
		if (SrcBuffer.Num() == 0)
		{
			return false;
		}

		uint32 ReadOffset = 0;
		uint32 WriteOffset = 0;
		uint16 ProcessedFrames = 0;
		const uint8* LockedSrc = SrcBuffer.GetTypedData();

		// Read True Sample Count, Number of channels and Frames to Encode first
		uint32 TrueSampleCount = *((uint32*)(LockedSrc + ReadOffset));
		ReadOffset += sizeof(uint32);
		uint8 NumChannels = *(LockedSrc + ReadOffset);
		ReadOffset += sizeof(uint8);
		uint16 SerializedFrames = *((uint16*)(LockedSrc + ReadOffset));
		ReadOffset += sizeof(uint16);

		// Should always be able to store basic info in a single chunk
		check(ReadOffset - WriteOffset < kMaxChunkSizeBytes)

		while (ProcessedFrames < SerializedFrames)
		{
			uint16 FrameSize = *((uint16*)(LockedSrc + ReadOffset));

			if ( (ReadOffset + sizeof(uint16) + FrameSize) - WriteOffset >= kMaxChunkSizeBytes)
			{
				// need to write some data
				int32 OutIndex = OutBuffers.Num();
				int32 BytesToCopy = ReadOffset - WriteOffset;
				OutBuffers.Add(TArray<uint8>());
				OutBuffers[OutIndex].Empty(BytesToCopy);
				OutBuffers[OutIndex].AddUninitialized(BytesToCopy);
				void* NewChunkData = OutBuffers[OutIndex].GetTypedData();
				FMemory::Memcpy(NewChunkData, LockedSrc+WriteOffset, BytesToCopy);
				WriteOffset += BytesToCopy;
			}

			ReadOffset += sizeof(uint16) + FrameSize;
			ProcessedFrames++;
		}
		if (WriteOffset < ReadOffset)
		{
			// need to write some data
			int32 OutIndex = OutBuffers.Num();
			int32 BytesToCopy = ReadOffset - WriteOffset;
			OutBuffers.Add(TArray<uint8>());
			OutBuffers[OutIndex].Empty(BytesToCopy);
			OutBuffers[OutIndex].AddUninitialized(BytesToCopy);
			void* NewChunkData = OutBuffers[OutIndex].GetTypedData();
			FMemory::Memcpy(NewChunkData, LockedSrc + WriteOffset, BytesToCopy);
			WriteOffset += BytesToCopy;
		}

		return true;
	}

	void Destroy(OpusEncoder* Encoder) const
	{
#if USE_UE4_MEM_ALLOC
		FMemory::Free(Encoder);
#else
		opus_encoder_destroy(Encoder);
#endif
	}
};


/**
 * Module for opus audio compression
 */

static IAudioFormat* Singleton = NULL;

class FAudioPlatformOpusModule : public IAudioFormatModule
{
public:
	virtual ~FAudioPlatformOpusModule()
	{
		delete Singleton;
		Singleton = NULL;
	}
	virtual IAudioFormat* GetAudioFormat()
	{
		if (!Singleton)
		{
			Singleton = new FAudioFormatOpus();
		}
		return Singleton;
	}
};

IMPLEMENT_MODULE( FAudioPlatformOpusModule, AudioFormatOpus);
