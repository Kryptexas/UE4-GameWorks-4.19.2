// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureLightProfile.cpp: Implementation of UTextureLightProfile.
=============================================================================*/

#include "EnginePrivate.h"


/*-----------------------------------------------------------------------------
	UTextureLightProfile
-----------------------------------------------------------------------------*/
UTextureLightProfile::UTextureLightProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NeverStream = true;
	Brightness = -1;
}

