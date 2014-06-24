// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CookerSettings.h: Declares the UCookerSettings class.
=============================================================================*/

#pragma once

#include "CookerSettings.generated.h"


/**
 * Implements per-project cooker settings exposed to the editor
 */
UCLASS(config=Engine)
class UNREALED_API UCookerSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Quality of 0 means fastest, 4 means best */
	UPROPERTY(config, EditAnywhere, Category=Textures)
	int32 DefaultPVRTCQuality;

	/** Whether or not to compile shaders for Metal on IOS (in beta 8). */
	UPROPERTY(config, EditAnywhere, Category=IOS)
	bool bGenerateMetalShaders;
};
