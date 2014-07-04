// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "OpusAudioInfo.h"
#include "IAudioFormat.h"
#include "Sound/SoundWave.h"

#define WITH_OPUS (PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_LINUX)

#if WITH_OPUS
#include "opus.h"
#endif

/*------------------------------------------------------------------------------------
FOpusDecoderWrapper
------------------------------------------------------------------------------------*/
#define USE_UE4_MEM_ALLOC 1
#define OPUS_SAMPLE_RATE 48000
#define OPUS_MAX_FRAME_SIZE 5760
#define SAMPLE_SIZE			( ( uint32 )sizeof( short ) )

struct FOpusDecoderWrapper
{
	FOpusDecoderWrapper(uint8 NumChannels)
	{
#if WITH_OPUS
#if USE_UE4_MEM_ALLOC
		int32 DecSize = opus_decoder_get_size(NumChannels);
		Decoder = (OpusDecoder*)FMemory::Malloc(DecSize);
		DecError = opus_decoder_init(Decoder, OPUS_SAMPLE_RATE, NumChannels);
#else
		Decoder = opus_decoder_create(OPUS_SAMPLE_RATE, NumChannels, &DecError);
#endif
#endif
	}

	~FOpusDecoderWrapper()
	{
#if WITH_OPUS
#if USE_UE4_MEM_ALLOC
		FMemory::Free(Decoder);
#else
		opus_encoder_destroy(Decoder);
#endif
#endif
	}

#if WITH_OPUS
	OpusDecoder* Decoder;
#endif
	int32 DecError;
};

/*------------------------------------------------------------------------------------
FOpusAudioInfo.
------------------------------------------------------------------------------------*/
FOpusAudioInfo::FOpusAudioInfo(void)
	: OpusDecoderWrapper(NULL)
	, SrcBufferData(NULL)
	, SrcBufferDataSize(0)
	, SrcBufferOffset(0)
	, AudioDataOffset(0)
	, TrueSampleCount(0)
	, CurrentSampleCount(0)
	, NumChannels(0)
	, SampleStride(0)
	, LastPCMByteSize(0)
	, LastPCMOffset(0)
	, bStoringEndOfFile(false)
	, StreamingSoundWave(NULL)
	, CurrentChunkIndex(0)
{
}

FOpusAudioInfo::~FOpusAudioInfo(void)
{
	if (OpusDecoderWrapper != NULL)
	{
		delete OpusDecoderWrapper;
	}
}

size_t FOpusAudioInfo::Read(void *ptr, uint32 size)
{
	check(SrcBufferOffset < SrcBufferDataSize);

	size_t BytesToRead = FMath::Min(size, SrcBufferDataSize - SrcBufferOffset);
	FMemory::Memcpy(ptr, SrcBufferData + SrcBufferOffset, BytesToRead);
	SrcBufferOffset += BytesToRead;
	return(BytesToRead);
}

bool FOpusAudioInfo::ReadCompressedInfo(const uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo)
{
	SrcBufferData = InSrcBufferData;
	SrcBufferDataSize = InSrcBufferDataSize;
	SrcBufferOffset = 0;
	CurrentSampleCount = 0;

	// Read Identifier, True Sample Count, Number of channels and Frames to Encode first
	if (FCStringAnsi::Strcmp((char*)InSrcBufferData, OPUS_ID_STRING) != 0)
	{
		return false;
	}
	else
	{
		SrcBufferOffset += FCStringAnsi::Strlen(OPUS_ID_STRING) + 1;
	}
	Read(&TrueSampleCount, sizeof(uint32));
	Read(&NumChannels, sizeof(uint8));
	uint16 SerializedFrames = 0;
	Read(&SerializedFrames, sizeof(uint16));
	AudioDataOffset = SrcBufferOffset;
	SampleStride = SAMPLE_SIZE * NumChannels;

	OpusDecoderWrapper = new FOpusDecoderWrapper(NumChannels);
	LastDecodedPCM.Empty();
	LastDecodedPCM.AddUninitialized(OPUS_MAX_FRAME_SIZE * SampleStride);
#if WITH_OPUS
	if (OpusDecoderWrapper->DecError != OPUS_OK)
	{
		delete OpusDecoderWrapper;
		return false;
	}
#else
	return false;
#endif

	if (QualityInfo)
	{
		QualityInfo->SampleRate = OPUS_SAMPLE_RATE;
		QualityInfo->NumChannels = NumChannels;
		QualityInfo->SampleDataSize = TrueSampleCount * QualityInfo->NumChannels * SAMPLE_SIZE;
		QualityInfo->Duration = (float)TrueSampleCount / QualityInfo->SampleRate;
	}

	return true;
}

bool FOpusAudioInfo::ReadCompressedData(uint8* Destination, bool bLooping, uint32 BufferSize /*= 0*/)
{
	check(OpusDecoderWrapper);
	check(Destination);

	SCOPE_CYCLE_COUNTER(STAT_OpusDecompressTime);

	// Write out any PCM data that was decoded during the last request
	uint32 RawPCMOffset = WriteFromDecodedPCM(Destination, BufferSize);

	bool bLooped = false;

	if (bStoringEndOfFile && LastPCMByteSize > 0)
	{
		// delayed returning looped because we hadn't read the entire buffer
		bLooped = true;
		bStoringEndOfFile = false;
	}

	while (RawPCMOffset < BufferSize)
	{
		uint16 FrameSize = 0;
		Read(&FrameSize, sizeof(uint16));
		int32 DecodedSamples = DecompressToPCMBuffer(FrameSize);
		if (DecodedSamples < 0)
		{
			LastPCMByteSize = 0;
			return false;
		}
		else
		{
			LastPCMByteSize = IncrementCurrentSampleCount(DecodedSamples) * SampleStride;
			RawPCMOffset += WriteFromDecodedPCM(Destination + RawPCMOffset, BufferSize - RawPCMOffset);

			if (SrcBufferOffset >= SrcBufferDataSize)
			{
				// check whether all decoded PCM was written
				if (LastPCMByteSize == 0)
				{
					bLooped = true;
				}
				else
				{
					bStoringEndOfFile = true;
				}
				if (bLooping)
				{
					SrcBufferOffset = AudioDataOffset;
					CurrentSampleCount = 0;
				}
				else
				{
					RawPCMOffset += ZeroBuffer(Destination + RawPCMOffset, BufferSize - RawPCMOffset);
				}
			}
		}
	}

	return bLooped;
}

void FOpusAudioInfo::ExpandFile(uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo)
{
	check(OpusDecoderWrapper);
	check(DstBuffer);
	check(QualityInfo);
	check(QualityInfo->SampleDataSize <= SrcBufferDataSize);

	// Ensure we're at the start of the audio data
	SrcBufferOffset = AudioDataOffset;

	uint32 RawPCMOffset = 0;

	while (RawPCMOffset < QualityInfo->SampleDataSize)
	{
		uint16 FrameSize = 0;
		Read(&FrameSize, sizeof(uint16));
		int32 DecodedSamples = DecompressToPCMBuffer(FrameSize);

		if (DecodedSamples < 0)
		{
			RawPCMOffset += ZeroBuffer(DstBuffer + RawPCMOffset, QualityInfo->SampleDataSize - RawPCMOffset);
		}
		else
		{
			LastPCMByteSize = IncrementCurrentSampleCount(DecodedSamples) * SampleStride;
			RawPCMOffset += WriteFromDecodedPCM(DstBuffer + RawPCMOffset, QualityInfo->SampleDataSize - RawPCMOffset);
		}
	}
}

bool FOpusAudioInfo::StreamCompressedInfo(USoundWave* Wave, struct FSoundQualityInfo* QualityInfo)
{
	StreamingSoundWave = Wave;

	// Get the first chunk of audio data (should always be loaded)
	CurrentChunkIndex = 0;
	const uint8* FirstChunk = IStreamingManager::Get().GetAudioStreamingManager().GetLoadedChunk(StreamingSoundWave, CurrentChunkIndex);

	if (FirstChunk)
	{
		return ReadCompressedInfo(FirstChunk, Wave->RunningPlatformData->Chunks[0].DataSize, QualityInfo);
	}

	return false;
}

bool FOpusAudioInfo::StreamCompressedData(uint8* Destination, bool bLooping, uint32 BufferSize)
{
	check(OpusDecoderWrapper);
	check(Destination);

	SCOPE_CYCLE_COUNTER(STAT_OpusDecompressTime);

	// Write out any PCM data that was decoded during the last request
	uint32 RawPCMOffset = WriteFromDecodedPCM(Destination, BufferSize);

	// If next chunk wasn't loaded when last one finished reading, try to get it again now
	if (SrcBufferData == NULL)
	{
		SrcBufferData = IStreamingManager::Get().GetAudioStreamingManager().GetLoadedChunk(StreamingSoundWave, CurrentChunkIndex);
		if (SrcBufferData)
		{
			SrcBufferDataSize = StreamingSoundWave->RunningPlatformData->Chunks[CurrentChunkIndex].DataSize;
			SrcBufferOffset = CurrentChunkIndex == 0 ? AudioDataOffset : 0;
		}
		else
		{
			// Still not loaded, should probably have some kind of error, zero current buffer
			ZeroBuffer(Destination + RawPCMOffset, BufferSize - RawPCMOffset);
			return false;
		}
	}

	bool bLooped = false;

	if (bStoringEndOfFile && LastPCMByteSize > 0)
	{
		// delayed returning looped because we hadn't read the entire buffer
		bLooped = true;
		bStoringEndOfFile = false;
	}

	while (RawPCMOffset < BufferSize)
	{
		uint16 FrameSize = 0;
		Read(&FrameSize, sizeof(uint16));
		int32 DecodedSamples = DecompressToPCMBuffer(FrameSize);

		if (DecodedSamples < 0)
		{
			LastPCMByteSize = 0;
			return false;
		}
		else
		{
			LastPCMByteSize = IncrementCurrentSampleCount(DecodedSamples) * SampleStride;

			RawPCMOffset += WriteFromDecodedPCM(Destination + RawPCMOffset, BufferSize - RawPCMOffset);

			// Have we reached the end of buffer
			if (SrcBufferOffset >= SrcBufferDataSize)
			{
				// Special case for the last chunk of audio
				if (CurrentChunkIndex == StreamingSoundWave->RunningPlatformData->NumChunks - 1)
				{
					// check whether all decoded PCM was written
					if (LastPCMByteSize == 0)
					{
						bLooped = true;
					}
					else
					{
						bStoringEndOfFile = true;
					}
					if (bLooping)
					{
						CurrentChunkIndex = 0;
						SrcBufferOffset = AudioDataOffset;
						CurrentSampleCount = 0;
					}
					else
					{
						RawPCMOffset += ZeroBuffer(Destination + RawPCMOffset, BufferSize - RawPCMOffset);
					}
				}
				else
				{
					CurrentChunkIndex++;
					SrcBufferOffset = 0;
				}

				SrcBufferData = IStreamingManager::Get().GetAudioStreamingManager().GetLoadedChunk(StreamingSoundWave, CurrentChunkIndex);
				if (SrcBufferData)
				{
					SrcBufferDataSize = StreamingSoundWave->RunningPlatformData->Chunks[CurrentChunkIndex].DataSize;
				}
				else
				{
					SrcBufferDataSize = 0;
					RawPCMOffset += ZeroBuffer(Destination + RawPCMOffset, BufferSize - RawPCMOffset);
				}
			}
		}
	}

	return bLooped;
}

int32 FOpusAudioInfo::DecompressToPCMBuffer(uint16 FrameSize)
{
	check(SrcBufferOffset + FrameSize <= SrcBufferDataSize);

	const uint8* SrcPtr = SrcBufferData + SrcBufferOffset;
	SrcBufferOffset += FrameSize;
	LastPCMOffset = 0;
#if WITH_OPUS
	return opus_decode(OpusDecoderWrapper->Decoder, SrcPtr, FrameSize, (int16*)(LastDecodedPCM.GetTypedData()), OPUS_MAX_FRAME_SIZE, 0);
#else
	return -1;
#endif
}

uint32 FOpusAudioInfo::IncrementCurrentSampleCount(uint32 NewSamples)
{
	if (CurrentSampleCount + NewSamples > TrueSampleCount)
	{
		NewSamples = TrueSampleCount - CurrentSampleCount;
		CurrentSampleCount = TrueSampleCount;
	}
	else
	{
		CurrentSampleCount += NewSamples;
	}
	return NewSamples;
}

uint32 FOpusAudioInfo::WriteFromDecodedPCM(uint8* Destination, uint32 BufferSize)
{
	uint32 BytesToCopy = FMath::Min(BufferSize, LastPCMByteSize - LastPCMOffset);
	if (BytesToCopy > 0)
	{
		FMemory::Memcpy(Destination, LastDecodedPCM.GetTypedData() + LastPCMOffset, BytesToCopy);
		LastPCMOffset += BytesToCopy;
		if (LastPCMOffset >= LastPCMByteSize)
		{
			LastPCMOffset = 0;
			LastPCMByteSize = 0;
		}
	}
	return BytesToCopy;
}

uint32	FOpusAudioInfo::ZeroBuffer(uint8* Destination, uint32 BufferSize)
{
	check(Destination);

	if (BufferSize > 0)
	{
		FMemory::Memzero(Destination, BufferSize);
		return BufferSize;
	}
	return 0;
}
