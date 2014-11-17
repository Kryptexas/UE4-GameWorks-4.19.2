// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsTargetSettings.h: Declares the UWindowsTargetSettings class.
=============================================================================*/

#pragma once

#include "WindowsTargetSettings.generated.h"


/**
 * Implements the settings for the Windows target platform.
 */
UCLASS(config=Engine, defaultconfig)
class WINDOWSTARGETPLATFORM_API UWindowsTargetSettings
	: public UObject
{
public:

	GENERATED_UCLASS_BODY()

	/** 
	 * The collection of RHI's we want to support on this platform.
	 * This is not always the full list of RHI we can support.
	 */
	UPROPERTY(EditAnywhere, config, Category=Rendering)
	TArray<FString> TargetedRHIs;
};
