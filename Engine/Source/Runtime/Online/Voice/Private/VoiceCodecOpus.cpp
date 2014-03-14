// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "VoicePrivatePCH.h"
#include "VoiceCodecOpus.h"

#if PLATFORM_SUPPORTS_VOICE_CAPTURE

#include "opus.h"

/** Turn on extra logging for codec */
#define DEBUG_OPUS 0
/** Use UE4 memory allocation or Opus */
#define USE_UE4_MEM_ALLOC 1
/** Maximum number of frames in a single Opus packet */
#define MAX_OPUS_FRAMES_PER_PACKET 48
/** Number of max frames for buffer calculation purposes */
#define MAX_OPUS_FRAMES 6

/**
 * Number of samples per channel of available space in PCM during decompression.  
 * If this is less than the maximum packet duration (120ms; 5760 for 48kHz), opus will not be capable of decoding some packets.
 */
#define MAX_OPUS_FRAME_SIZE MAX_OPUS_FRAMES * 320
/** Hypothetical maximum for buffer validation */
#define MAX_OPUS_UNCOMPRESSED_BUFFER_SIZE 48 * 1024
/** 20ms frame sizes are a good choice for most applications (1000ms / 20ms = 50) */
#define NUM_OPUS_FRAMES_PER_SEC 50

/**
 * Output debug information regarding the state of the Opus encoder
 */
void DebugEncoderInfo(OpusEncoder* Encoder)
{
	int32 ErrCode = 0;

	int32 BitRate = 0;
	ErrCode = opus_encoder_ctl(Encoder, OPUS_GET_BITRATE(&BitRate));

	int32 Vbr = 0;
	ErrCode = opus_encoder_ctl(Encoder, OPUS_GET_VBR(&Vbr));

	int32 SampleRate = 0;
	ErrCode = opus_encoder_ctl(Encoder, OPUS_GET_SAMPLE_RATE(&SampleRate));

	int32 Application = 0;
	ErrCode = opus_encoder_ctl(Encoder, OPUS_GET_APPLICATION(&Application));

	int32 Complexity = 0;
	ErrCode = opus_encoder_ctl(Encoder, OPUS_GET_COMPLEXITY(&Complexity));

	UE_LOG(LogVoice, Verbose, TEXT("Opus Encoder Details:\n Application: %d\n Bitrate: %d\n SampleRate: %d\n VBR: %d\n Complexity: %d"),
		Application, BitRate, SampleRate, Vbr, Complexity);
}

/**
 * Output debug information regarding the state of a single Opus packet
 */
void DebugFrameInfo(const uint8* PacketData, uint32 PacketLength, uint32 SampleRate)
{
	int32 NumFrames = opus_packet_get_nb_frames(PacketData, PacketLength);
	int32 NumSamples = opus_packet_get_nb_samples(PacketData, PacketLength, SampleRate);
	int32 NumSamplesPerFrame = opus_packet_get_samples_per_frame(PacketData, SampleRate);
	int32 Bandwidth = opus_packet_get_bandwidth(PacketData);

	/*
	 *	0
	 *	0 1 2 3 4 5 6 7
	 *	+-+-+-+-+-+-+-+-+
	 *	| config  |s| c |
	 *	+-+-+-+-+-+-+-+-+
	 */
	uint8 TOC;
	// (max 48 x 2.5ms frames in a packet = 120ms)
	const uint8* frames[48];
	int16 size[48];
	int32 payload_offset = 0;
	int32 NumFramesParsed = opus_packet_parse(PacketData, PacketLength, &TOC, frames, size, &payload_offset);

	// Frame Encoding see http://tools.ietf.org/html/rfc6716#section-3.1
	int32 TOCEncoding = (TOC & 0xf8) >> 3;

	// Number of channels
	bool TOCStereo = (TOC & 0x4) != 0 ? true : false;

	// Number of frames and their configuration
	// 0: 1 frame in the packet
	// 1: 2 frames in the packet, each with equal compressed size
    // 2: 2 frames in the packet, with different compressed sizes
	// 3: an arbitrary number of frames in the packet
	int32 TOCMode = TOC & 0x3;

	UE_LOG(LogVoice, Verbose, TEXT("PacketLength: %d NumFrames: %d NumSamples: %d Bandwidth: %d Encoding: %d Stereo: %d FrameDesc: %d"),
			PacketLength, NumFrames, NumSamples, Bandwidth, TOCEncoding, TOCStereo, TOCMode);
}

FVoiceEncoderOpus::FVoiceEncoderOpus() :
	SampleRate(0),
	NumChannels(0),
	FrameSize(0),
	Encoder(NULL),
	LastEntropyIdx(0)
{
	FMemory::Memzero(Entropy, NUM_ENTROPY_VALUES * sizeof(uint32));
}

FVoiceEncoderOpus::~FVoiceEncoderOpus()
{
	Destroy();
}

bool FVoiceEncoderOpus::Init(int32 InSampleRate, int32 InNumChannels)
{
	UE_LOG(LogVoice, Log, TEXT("EncoderVersion: %s"), ANSI_TO_TCHAR(opus_get_version_string()));

	SampleRate = InSampleRate;
	NumChannels = InNumChannels;

	// 20ms frame sizes are a good choice for most applications (1000ms / 20ms = 50)
	FrameSize = SampleRate / NUM_OPUS_FRAMES_PER_SEC;
	//MaxFrameSize = FrameSize * MAX_OPUS_FRAMES;

	int32 EncError = 0;

#if USE_UE4_MEM_ALLOC
	int32 EncSize = opus_encoder_get_size(NumChannels);
	Encoder = (OpusEncoder*)FMemory::Malloc(EncSize);
	EncError = opus_encoder_init(Encoder, SampleRate, NumChannels, OPUS_APPLICATION_VOIP);
#else
	Encoder = opus_encoder_create(SampleRate, NumChannels, OPUS_APPLICATION_VOIP, &EncError);
#endif

	if (EncError != OPUS_OK)
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to init Opus Encoder: %s"), ANSI_TO_TCHAR(opus_strerror(EncError)));
		Destroy();
	}

	// Turn on variable bit rate encoding
	int32 UseVbr = 1;
	opus_encoder_ctl(Encoder, OPUS_SET_VBR(UseVbr));

	// Turn of constrained VBR
	int32 UseCVbr = 0;
	opus_encoder_ctl(Encoder, OPUS_SET_VBR_CONSTRAINT(UseCVbr));

	// Complexity (1-10)
	int32 Complexity = 10;
	opus_encoder_ctl(Encoder, OPUS_SET_COMPLEXITY(Complexity));

	// Forward error correction
	int32 InbandFEC = 0;
	opus_encoder_ctl(Encoder, OPUS_SET_INBAND_FEC(InbandFEC));

#if DEBUG_OPUS
	DebugEncoderInfo(Encoder);
#endif // DEBUG_OPUS
	return EncError == OPUS_OK;
}

int32 FVoiceEncoderOpus::Encode(const uint8* RawPCMData, uint32 RawDataSize, uint8* OutCompressedData, uint32& OutCompressedDataSize)
{
	check(Encoder);
	SCOPE_CYCLE_COUNTER(STAT_Voice_Encoding);

	int32 BytesPerFrame = FrameSize * NumChannels * sizeof(opus_int16);
	int32 MaxFramesEncoded = MAX_OPUS_UNCOMPRESSED_BUFFER_SIZE / BytesPerFrame;

	// total bytes / bytes per frame
	int32 NumFramesToEncode = FMath::Min((int32)RawDataSize / BytesPerFrame, MaxFramesEncoded); 
	int32 DataRemainder = RawDataSize % BytesPerFrame;
	int32 RawDataStride = BytesPerFrame;

	int32 AvailableBufferSize = OutCompressedDataSize;
	int32 CompressedBufferOffset = 0;

	// Store the number of frames to be encoded
	check(NumFramesToEncode < MAX_uint8);
	OutCompressedData[0] = NumFramesToEncode;
	
	// Store the offset to each encoded frame
	uint16* CompressedOffsets = (uint16*)(OutCompressedData + 1);

	// Start of the actual compressed data
	uint8* CompressedDataStart = (OutCompressedData + 1) + (NumFramesToEncode * sizeof(uint16));
 
	for (int32 i = 0; i < NumFramesToEncode; i++)
	{
		int32 CompressedLength = 0;
		CompressedLength = opus_encode(Encoder, (const opus_int16*)(RawPCMData + (i * RawDataStride)), FrameSize, CompressedDataStart + CompressedBufferOffset, AvailableBufferSize);

		if (CompressedLength < 0)
		{
			const char* ErrorStr = opus_strerror(CompressedLength);
			UE_LOG(LogVoice, Warning, TEXT("Failed to encode: [%d] %s"), CompressedLength, ANSI_TO_TCHAR(ErrorStr));

			OutCompressedData[0] = 0;
			OutCompressedDataSize = 0;
			return 0;
		}
		else if (CompressedLength != 1)
		{
			opus_encoder_ctl(Encoder, OPUS_GET_FINAL_RANGE(&Entropy[LastEntropyIdx]));
			LastEntropyIdx = (LastEntropyIdx + 1) % NUM_ENTROPY_VALUES;

#if DEBUG_OPUS
			DebugFrameInfo(OutCompressedData + CompressedBufferOffset, CompressedLength, SampleRate);
#endif // DEBUG_OPUS

			AvailableBufferSize -= CompressedLength;
			CompressedBufferOffset += CompressedLength;

			check(CompressedBufferOffset < MAX_uint16); 
			CompressedOffsets[i] = (uint16)CompressedBufferOffset;
		}
		else
		{
			UE_LOG(LogVoice, Warning, TEXT("Nothing to encode!"));
		}
	}

	// End of buffer
	OutCompressedDataSize = CompressedBufferOffset;

	UE_LOG(LogVoice, VeryVerbose, TEXT("RawSize: %d CompressedSize: %d NumFramesEncoded: %d Waste: %d"), RawDataSize, OutCompressedDataSize, NumFramesToEncode, DataRemainder);
	return DataRemainder;
}

void FVoiceEncoderOpus::Destroy()
{
#if USE_UE4_MEM_ALLOC
	FMemory::Free(Encoder);
#else
	opus_encoder_destroy(Encoder);
#endif
	Encoder = NULL;
}

FVoiceDecoderOpus::FVoiceDecoderOpus() :
	SampleRate(0),
	NumChannels(0),
	FrameSize(0),
	Decoder(NULL),
	LastEntropyIdx(0)
{
	FMemory::Memzero(Entropy, NUM_ENTROPY_VALUES * sizeof(uint32));
}

FVoiceDecoderOpus::~FVoiceDecoderOpus()
{
	Destroy();
}

bool FVoiceDecoderOpus::Init(int32 InSampleRate, int32 InNumChannels)
{
	UE_LOG(LogVoice, Log, TEXT("DecoderVersion: %s"), ANSI_TO_TCHAR(opus_get_version_string()));

	SampleRate = InSampleRate;
	NumChannels = InNumChannels;

	FrameSize = SampleRate / 50;

	int32 DecSize = 0;
	int32 DecError = 0;

#if USE_UE4_MEM_ALLOC
	DecSize = opus_decoder_get_size(NumChannels);
	Decoder = (OpusDecoder*)FMemory::Malloc(DecSize);
	DecError = opus_decoder_init(Decoder, SampleRate, NumChannels);
#else
	Decoder = opus_decoder_create(SampleRate, NumChannels, &DecError);
#endif
	if (DecError != OPUS_OK)
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to init Opus Decoder: %s"), ANSI_TO_TCHAR(opus_strerror(DecError)));
		Destroy();
	}

	return DecError == OPUS_OK;
}

void FVoiceDecoderOpus::Destroy()
{
#if USE_UE4_MEM_ALLOC
	FMemory::Free(Decoder);
#else
	opus_decoder_destroy(Decoder);
#endif 
	Decoder = NULL;
}

void FVoiceDecoderOpus::Decode(const uint8* InCompressedData, uint32 CompressedDataSize, uint8* OutRawPCMData, uint32& OutRawDataSize)
{
	SCOPE_CYCLE_COUNTER(STAT_Voice_Decoding);

	int32 BytesPerFrame = FrameSize * NumChannels * sizeof(opus_int16);
	int32 MaxFramesEncoded = MAX_OPUS_UNCOMPRESSED_BUFFER_SIZE / BytesPerFrame;

	int32 NumFramesToDecode = InCompressedData[0];
	if (NumFramesToDecode > 0 && NumFramesToDecode < MaxFramesEncoded)
	{
		// Start of compressed data offsets
		const uint16* CompressedOffsets = (const uint16*)(InCompressedData + 1);

		// Start of compressed data
		const uint8* CompressedDataStart = (InCompressedData + 1) + (NumFramesToDecode * sizeof(uint16));
		
		int32 CompressedBufferOffset = 0;
		int32 DecompressedBufferOffset = 0;
		uint16 LastCompressedOffset = 0;

		for (int32 i=0; i < NumFramesToDecode; i++)
		{
			int32 UncompressedBufferAvail = OutRawDataSize - DecompressedBufferOffset;

			if (UncompressedBufferAvail >= (MAX_OPUS_FRAMES * BytesPerFrame))
			{
				int32 CompressedBufferSize = CompressedOffsets[i] - LastCompressedOffset;

				check(Decoder);
				int32 NumDecompressedSamples = opus_decode(Decoder, 
					CompressedDataStart + CompressedBufferOffset, CompressedBufferSize, 
					(opus_int16*)(OutRawPCMData + DecompressedBufferOffset), MAX_OPUS_FRAME_SIZE, 0);

				if (NumDecompressedSamples < 0)
				{
					OutRawDataSize = 0;
					const char* ErrorStr = opus_strerror(NumDecompressedSamples);
					UE_LOG(LogVoice, Warning, TEXT("Failed to decode: [%d] %s"), NumDecompressedSamples, ANSI_TO_TCHAR(ErrorStr));
					return;
				}
				else
				{
#if DEBUG_OPUS
					if (NumDecompressedSamples != FrameSize)
					{
						UE_LOG(LogVoice, Warning, TEXT("Unexpected decode result NumSamplesDecoded %d != FrameSize %d"), NumDecompressedSamples, FrameSize);
					}

					DebugFrameInfo(InCompressedData, CompressedDataSize, SampleRate);
#endif // DEBUG_OPUS
					
					opus_decoder_ctl(Decoder, OPUS_GET_FINAL_RANGE(&Entropy[LastEntropyIdx]));
					LastEntropyIdx = (LastEntropyIdx + 1) % NUM_ENTROPY_VALUES;

					DecompressedBufferOffset += NumDecompressedSamples * sizeof(opus_int16);
					CompressedBufferOffset += CompressedBufferSize;
					LastCompressedOffset = CompressedOffsets[i];
				}
			}
			else
			{
				UE_LOG(LogVoice, Warning, TEXT("Decompression buffer too small to decode voice"));
				break;
			}
		}

		OutRawDataSize = DecompressedBufferOffset;
	}
	else
	{
		UE_LOG(LogVoice, Warning, TEXT("Failed to decode: buffer corrupted"));
		OutRawDataSize = 0;
	}
}

#endif // PLATFORM_SUPPORTS_VOICE_CAPTURE