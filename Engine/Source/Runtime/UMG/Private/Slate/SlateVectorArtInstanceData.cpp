// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "SlateVectorArtInstanceData.h"

void FSlateVectorArtInstanceData::SetPositionFixedPoint16(FVector2D Position)
{
	// We are effectively doing fixed point positioning
	// With 13 bits for the whole part and 3 bits of decimal precision.
	// xxxxxxxxxxxxx.xxx
	//     [0..8191].[0..7]
	{
		uint32 CurrentDataX = static_cast<uint32>(GetData().X) & 0xFFFF0000;
		GetData().X = CurrentDataX | FMath::RoundToInt(Position.X*8.0f) << 0;
	}

	{
		uint32 CurrentDataY = static_cast<uint32>(GetData().Y) & 0xFFFF0000;
		GetData().Y = CurrentDataY | FMath::RoundToInt(Position.Y*8.0f) << 0;
	}
}

void FSlateVectorArtInstanceData::SetPosition(FVector2D Position)
{
	GetData().X = Position.X;
	GetData().Y = Position.Y;
}

void FSlateVectorArtInstanceData::SetScale(float Scale)
{
	GetData().Z = Scale;
}
