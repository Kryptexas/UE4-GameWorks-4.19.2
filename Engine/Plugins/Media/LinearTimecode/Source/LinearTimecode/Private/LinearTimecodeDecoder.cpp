// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LinearTimecodeDecoder.h"
#include "HAL/UnrealMemory.h"

FLinearTimecodeDecoder::FLinearTimecodeDecoder()
{
	Reset();
}

FLinearTimecodeDecoder::~FLinearTimecodeDecoder()
{
}

void FLinearTimecodeDecoder::DecodeBitStream(uint16* InBitStream, EDecodePattern* InDecodePattern, int32 InForward, FDropTimecode& OutTimeCode)
{
	FMemory::Memset(&OutTimeCode, 0, sizeof(FDropTimecode));
	OutTimeCode.Forward = InForward;

	auto DecodeBits = [InBitStream](EDecodePattern* inDecodePatern)
	{
		return (InBitStream[inDecodePatern->Index] & inDecodePatern->Mask) ? inDecodePatern->Addition : 0;
	};

	while (InDecodePattern->Type != EDecodePattern::EDecodeType::End)
	{
		switch (InDecodePattern->Type)
		{
		case EDecodePattern::EDecodeType::Hours:
			OutTimeCode.Hours += DecodeBits(InDecodePattern);
			break;
		case EDecodePattern::EDecodeType::Minutes:
			OutTimeCode.Minutes += DecodeBits(InDecodePattern);
			break;
		case EDecodePattern::EDecodeType::Seconds:
			OutTimeCode.Seconds += DecodeBits(InDecodePattern);
			break;
		case EDecodePattern::EDecodeType::Frames:
			OutTimeCode.Frame += DecodeBits(InDecodePattern);
			break;
		case EDecodePattern::EDecodeType::DropFrame:
			OutTimeCode.Drop += DecodeBits(InDecodePattern);
			break;
		case EDecodePattern::EDecodeType::ColorFrame:
			OutTimeCode.Color += DecodeBits(InDecodePattern);;
			break;
		case EDecodePattern::EDecodeType::FrameRate:
			if (InForward)
			{
				if (FrameMax < OutTimeCode.Frame)
				{
					FrameMax = OutTimeCode.Frame;
				}
				if (FrameMax > OutTimeCode.Frame)
				{
					FrameRate = FrameMax;
				}
				OutTimeCode.FrameRate = FrameRate;
			}
			else
			{
				if (FrameMax < OutTimeCode.Frame && FrameRate < OutTimeCode.Frame)
				{
					FrameRate = OutTimeCode.Frame;
				}
				FrameMax = OutTimeCode.Frame;
			}
			OutTimeCode.FrameRate = FrameRate;
		}
		++InDecodePattern;
	}
}

FLinearTimecodeDecoder::EDecodePattern FLinearTimecodeDecoder::ForwardPattern[] = {

	{ EDecodePattern::EDecodeType::Hours, 3, 0x8000, 1 },
	{ EDecodePattern::EDecodeType::Hours, 3, 0x4000, 2 },
	{ EDecodePattern::EDecodeType::Hours, 3, 0x2000, 4 },
	{ EDecodePattern::EDecodeType::Hours, 3, 0x1000, 8 },
	{ EDecodePattern::EDecodeType::Hours, 3, 0x0080, 10 },
	{ EDecodePattern::EDecodeType::Hours, 3, 0x0040, 20 },

	{ EDecodePattern::EDecodeType::Minutes, 2, 0x8000, 1 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x4000, 2 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x2000, 4 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x1000, 8 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0080, 10 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0040, 20 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0020, 40 },

	{ EDecodePattern::EDecodeType::Seconds, 1, 0x8000, 1 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x4000, 2 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x2000, 4 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x1000, 8 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x0080, 10 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x0040, 20 },
	{ EDecodePattern::EDecodeType::Seconds, 1, 0x0020, 40 },

	{ EDecodePattern::EDecodeType::Frames, 0, 0x8000, 1 },
	{ EDecodePattern::EDecodeType::Frames, 0, 0x4000, 2 },
	{ EDecodePattern::EDecodeType::Frames, 0, 0x2000, 4 },
	{ EDecodePattern::EDecodeType::Frames, 0, 0x1000, 8 },
	{ EDecodePattern::EDecodeType::Frames, 0, 0x0080, 10 },
	{ EDecodePattern::EDecodeType::Frames, 0, 0x0040, 20 },

	{ EDecodePattern::EDecodeType::FrameRate, 0, 0, 0 },

	{ EDecodePattern::EDecodeType::DropFrame, 0, 0x0020, 1 },
	{ EDecodePattern::EDecodeType::ColorFrame, 0, 0x0010, 1 },

	{ EDecodePattern::EDecodeType::End, 0, 0, 0},
};

FLinearTimecodeDecoder::EDecodePattern FLinearTimecodeDecoder::BackwardPattern[] = {

	{ EDecodePattern::EDecodeType::Hours, 1, 0x0001, 1 },
	{ EDecodePattern::EDecodeType::Hours, 1, 0x0002, 2 },
	{ EDecodePattern::EDecodeType::Hours, 1, 0x0004, 4 },
	{ EDecodePattern::EDecodeType::Hours, 1, 0x0008, 8 },
	{ EDecodePattern::EDecodeType::Hours, 1, 0x0100, 10 },
	{ EDecodePattern::EDecodeType::Hours, 1, 0x0200, 20 },

	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0001, 1 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0002, 2 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0004, 4 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0008, 8 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0100, 10 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0200, 20 },
	{ EDecodePattern::EDecodeType::Minutes, 2, 0x0400, 40 },

	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0001, 1 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0002, 2 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0004, 4 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0008, 8 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0100, 10 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0200, 20 },
	{ EDecodePattern::EDecodeType::Seconds, 3, 0x0400, 40 },

	{ EDecodePattern::EDecodeType::Frames, 4, 0x0001, 1 },
	{ EDecodePattern::EDecodeType::Frames, 4, 0x0002, 2 },
	{ EDecodePattern::EDecodeType::Frames, 4, 0x0004, 4 },
	{ EDecodePattern::EDecodeType::Frames, 4, 0x0008, 8 },
	{ EDecodePattern::EDecodeType::Frames, 4, 0x0100, 10 },
	{ EDecodePattern::EDecodeType::Frames, 4, 0x0200, 20 },

	{ EDecodePattern::EDecodeType::FrameRate, 0, 0, 0 },

	{ EDecodePattern::EDecodeType::DropFrame, 4, 0x0400, 1 },
	{ EDecodePattern::EDecodeType::ColorFrame, 4, 0x0800, 1 },
	
	{ EDecodePattern::EDecodeType::End, 0, 0, 0 },
};

void FLinearTimecodeDecoder::ShiftAndInsert(uint16* InBitStream, bool InBit) const
{
	InBitStream[0] = (InBitStream[0] << 1) + ((InBitStream[1] & 0x8000) ? 1 : 0);
	InBitStream[1] = (InBitStream[1] << 1) + ((InBitStream[2] & 0x8000) ? 1 : 0);
	InBitStream[2] = (InBitStream[2] << 1) + ((InBitStream[3] & 0x8000) ? 1 : 0);
	InBitStream[3] = (InBitStream[3] << 1) + ((InBitStream[4] & 0x8000) ? 1 : 0);
	InBitStream[4] = (InBitStream[4] << 1) + (InBit ? 1 : 0);
}

bool FLinearTimecodeDecoder::HasCompleteFrame(uint16* InBitStream) const
{
	return (InBitStream[4] == 0x3ffd) || (InBitStream[4] == 0xbffc);
}

bool FLinearTimecodeDecoder::DecodeFrame(uint16* InBitStream, FDropTimecode& OutTimeCode)
{
	// forward stream
	if (InBitStream[4] == 0x3ffd)
	{
		DecodeBitStream(InBitStream, ForwardPattern, 1, OutTimeCode);
		return true;
	}
	// backward stream
	if (InBitStream[0] == 0xbffc)
	{
		DecodeBitStream(InBitStream, BackwardPattern, 0, OutTimeCode);
		return true;
	}

	return false;
}

void FLinearTimecodeDecoder::Reset(void)
{
	Center = 0.0f;
	bCurrent = false;

	bFlip = false;
	Clock = 0;
	Cycles = 0;

	FrameMax = 0;
	FrameRate = 0;

	TimecodeBits[0] = TimecodeBits[1] = TimecodeBits[2] = TimecodeBits[3] = TimecodeBits[4] = 0;
}

int32 FLinearTimecodeDecoder::AdjustCycles(int32 InClock) const
{
	return (Cycles + InClock * 4) / 5;
}

bool FLinearTimecodeDecoder::Sample(float InSample, FDropTimecode& OutTimeCode)
{
	++Clock;
	if ((!bCurrent && InSample > Center) || (bCurrent && InSample < Center))
	{		
		bCurrent = !bCurrent;

		if (bFlip)
		{
			bFlip = false;
			Cycles = AdjustCycles(Clock);
			Clock = 0;
		}
		else
		{
			if (Clock < (3 * Cycles) / 4)
			{
				ShiftAndInsert(TimecodeBits, true);
				bFlip = true;
			}
			else
			{
				ShiftAndInsert(TimecodeBits, false);
				Cycles = AdjustCycles(Clock);
				Clock = 0;
			}
			return DecodeFrame(TimecodeBits, OutTimeCode);
		}
	}
	return false;
}

