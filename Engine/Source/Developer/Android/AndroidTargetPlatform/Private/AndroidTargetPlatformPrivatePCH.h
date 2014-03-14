// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================
	AndroidTargetPlatformPrivatePCH.h: Pre-compiled header file for the AndroidTargetPlatform module.
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
