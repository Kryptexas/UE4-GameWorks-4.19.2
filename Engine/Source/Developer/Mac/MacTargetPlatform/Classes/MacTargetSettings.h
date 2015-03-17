// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacTargetSettings.h: Declares the UMacTargetSettings class.
=============================================================================*/

#pragma once

#include "MacTargetSettings.generated.h"


/**
 * Implements the settings for the Mac target platform.
 */
UCLASS()
class MACTARGETPLATFORM_API UMacTargetSettings
	: public UObject
{
	GENERATED_BODY()
public:
	UMacTargetSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
