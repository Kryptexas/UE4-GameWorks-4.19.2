// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacTargetPlatformPrivatePCH.h: Pre-compiled header file for the MacTargetPlatform module.
=============================================================================*/

#pragma once


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "Settings.h"
#include "Runtime/Core/Public/Mac/MacPlatformProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif

#include "TargetPlatform.h"

/* Private includes
 *****************************************************************************/

#include "MacTargetSettings.h"
#include "LocalMacTargetDevice.h"
#include "GenericMacTargetPlatform.h"