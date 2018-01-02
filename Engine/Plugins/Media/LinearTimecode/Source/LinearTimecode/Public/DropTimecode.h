// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "UObject/Object.h"
#include "DropTimecode.generated.h"

/** Hold a frame of a Linear Timecode Frame */

USTRUCT(BlueprintType)
struct LINEARTIMECODE_API FDropTimecode
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Hours;
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Minutes;
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Seconds;
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Frame;
	/** set to 1 for drop frame */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Drop;
	/** guess at incoming frame rate */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 FrameRate;
	/** timecode is synced to an external source */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Clock;
	/** sync is in phase with color burst */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Color;
	/** set to 1 when timecode is reading forward */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 Forward;
	/** is a new timecode frame */
	UPROPERTY(BlueprintReadWrite, Category = "Media")
	int32 NewFrame;
};

