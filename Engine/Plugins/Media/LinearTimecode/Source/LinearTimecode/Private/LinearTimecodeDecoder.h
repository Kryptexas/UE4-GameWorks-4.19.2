// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "UObject/Object.h"

#include "DropTimecode.h"

/** Process audio samples to extract Linear Timecode Signal
*/

class FLinearTimecodeDecoder
{
	// public interface, methods
public:
	/** TimeCode structure used by reader
	*/
	FLinearTimecodeDecoder();
	virtual ~FLinearTimecodeDecoder();

	/** Flush internal state of timecode reader 
	*/
	void Reset(void);
	/** Analyze a single sample, looking for a time code pattern
	* @param InSample - single normalized sample, 1.0 to -1.0 
	* @param OutTimeCode - returned timecode to this point, untouched if not end of sequence sample
	* @return true if sample was last in a timecode frame
	*/
	bool Sample(float InSample, FDropTimecode& OutTimeCode);

protected:
	// used to extract timecode from bit stream
	struct EDecodePattern
	{
		enum class EDecodeType
		{
			Hours,
			Minutes,
			Seconds,
			Frames,
			DropFrame,
			ColorFrame,	
			FrameRate,
			End,
		};
		EDecodeType Type;
		int32 Index;
		uint16 Mask;
		int32 Addition;
	};

protected:
	void ShiftAndInsert(uint16* InBitStream, bool InBit) const;
	void DecodeBitStream(uint16* InBitStream, EDecodePattern* InDecodePatern, int32 InForward, FDropTimecode& OutTimeCode);

	bool HasCompleteFrame(uint16* InBitStream) const;
	bool DecodeFrame(uint16* InBitStream, FDropTimecode& OutTimeCode);

	int32 AdjustCycles(int32 InClock) const;

protected:
	static EDecodePattern ForwardPattern[];
	static EDecodePattern BackwardPattern[];

	float Center;
	bool bCurrent;

	int32 Clock;
	int32 Cycles;
	bool bFlip;
	int32 FrameMax;
	int32 FrameRate;

	uint16 TimecodeBits[5];
};

