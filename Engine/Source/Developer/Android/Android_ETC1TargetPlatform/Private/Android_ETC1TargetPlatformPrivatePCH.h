// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	Android_ETC1TargetPlatformPrivatePCH.h: Pre-compiled header file for the Android_ETC1TargetPlatform module.
=============================================================================*/

#pragma once

/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "Messaging.h"
#include "Runtime/Core/Public/Android/AndroidProperties.h"

#if WITH_ENGINE
	#include "Engine.h"
	#include "TextureCompressorModule.h"
#endif
#include "TargetDeviceServices.h"
#include "TargetPlatform.h"


/* Private includes
 *****************************************************************************/

#include "AndroidTargetDevice.h"
#include "AndroidTargetPlatform.h"
