// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxNoEditorTargetPlatformPrivatePCH.h: Pre-compiled header file for the LinuxNoEditorTargetPlatform module.
=============================================================================*/

#pragma once


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleManager.h"
#include "Runtime/Core/Public/Linux/LinuxPlatformProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif

#include "TargetPlatform.h"


/* Private includes
 *****************************************************************************/

#include "LinuxTargetDevice.h"
#include "LinuxTargetPlatform.h"
