// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "IAmbisonicsMixer.h"

namespace AmbisonicsStatics
{
	TArray<FVector>* GetDefaultPositionMap(int32 NumChannels)
	{
		switch (NumChannels)
		{
			// Mono speaker directly in front of listener:
			case 1:
			{
				static TArray<FVector> MonoMap = { {1.0f, 0.0f, 0.0f} };
				return &MonoMap;
			}

			// Stereo speakers to front left and right of listener:
			case 2:
			{
				static TArray<FVector> StereoMap = { {1.0f, -1.0f, 0.0f} , {1.0f, 1.0f, 0.0f} };
				return &StereoMap;
			}

			// Quadrophonic speakers at each corner.
			case 4:
			{
				static TArray<FVector> QuadMap = { {1.0f, -1.0f, 0.0f} , {1.0f, 1.0f, 0.0f} , {-1.0f, -1.0f, 0.0f} , {-1.0f, 1.0f, 0.0f} };
				return &QuadMap;
			}

			// 5.1 speakers.
			case 6:
			{
				static TArray<FVector> FiveDotOneMap = { { 1.0f,-1.0f, 0.0f } , //left
														 { 1.0f, 1.0f, 0.0f } , // right
														 { 1.0f, 0.0f, 0.0f } , //center
														 { 0.0f, 0.0f,-1.0f } , //LFE
														 {-1.0f,-1.0f, 0.0f } , //Left Rear
														 {-1.0f, 1.0f, 0.0f } };//Right Rear
				return &FiveDotOneMap;
			}

			case 8:
			{
				static TArray<FVector> SevenDotOneMap = { { 1.0f,-1.0f, 0.0f } , //left
														 { 1.0f, 1.0f, 0.0f } , // right
														 { 1.0f, 0.0f, 0.0f } , //center
														 { 0.0f, 0.0f,-1.0f } , //LFE
														 {-1.0f,-1.0f, 0.0f } , //Left Rear
														 {-1.0f, 1.0f, 0.0f } , //Right Rear
														 { 0.7f,-1.0f, 0.0f } , //Left Surround
														 { 0.7f, 1.0f, 0.0f } };//Right Surround

				return &SevenDotOneMap;
			}

			case 0:
			default:
			{
				return nullptr;
			}
		}
	};
}