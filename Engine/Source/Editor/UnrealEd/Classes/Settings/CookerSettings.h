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

	/** Whether or not to add support for Metal API (requires IOS8 and A7 processors) */
	UPROPERTY(config, EditAnywhere, Category=IOS)
	bool bSupportMetal;

	/** Whether or not to add support for OpenGL ES2 (if this is false, then your game should specify minimum IOS8 version and A7, in the plist) */
	UPROPERTY(config, EditAnywhere, Category = IOS)
	bool bSupportOpenGLES2;
};
