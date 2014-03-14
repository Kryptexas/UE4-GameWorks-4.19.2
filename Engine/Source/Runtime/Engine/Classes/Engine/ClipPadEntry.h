// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/* epic ===============================================
* class ClipPadEntry
*
* A block of text that can be pasted into the level.
 */

#pragma once
#include "ClipPadEntry.generated.h"

UCLASS(deprecated, hidecategories=Object, MinimalAPI)
class UDEPRECATED_ClipPadEntry : public UObject
{
	GENERATED_UCLASS_BODY()

	/** User specified name */
	UPROPERTY(EditAnywhere, Category=ClipPadEntry)
	FString Title;

	/** The text copied/pasted */
	UPROPERTY(EditAnywhere, Category=ClipPadEntry)
	FString Text;

};



