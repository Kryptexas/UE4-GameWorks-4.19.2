// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	AndroidTargetPlatformPrivatePCH.h: Pre-compiled header file for the AndroidTargetPlatform module.
=============================================================================*/

#pragma once

/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleManager.h"
#include "Settings.h"
#include "Runtime/Core/Public/Linux/LinuxPlatformProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif

#include "TargetPlatform.h"


/* Private includes
 *****************************************************************************/

#include "LinuxTargetSettings.h"
#include "LinuxTargetDevice.h"
#include "LinuxTargetPlatform.h"
