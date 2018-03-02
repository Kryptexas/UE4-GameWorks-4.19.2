// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ImgMediaSettings.h"
#include "Logging/LogMacros.h"
#include "Stats/Stats.h"
#include "UObject/NameTypes.h"

class FQueuedThreadPool;


/** The OpenEXR image reader is supported on macOS and Windows only. */
#define IMGMEDIA_EXR_SUPPORTED_PLATFORM (PLATFORM_MAC || PLATFORM_WINDOWS)

/** Default number of frames per second for image sequences. */
#define IMGMEDIA_DEFAULT_FPS 24.0

/** Whether to use a separate thread pool for image frame deallocations. */
#define USE_IMGMEDIA_DEALLOC_POOL UE_BUILD_DEBUG


/** Log category for the this module. */
DECLARE_LOG_CATEGORY_EXTERN(LogImgMedia, Log, All);


#if USE_IMGMEDIA_DEALLOC_POOL
	/** Thread pool used for deleting image frame buffers. */
	extern FQueuedThreadPool* GImgMediaThreadPoolSlow;
#endif


namespace ImgMedia
{
	/** Name of the FramesPerSecondAttribute media option. */
	static FName FramesPerSecondAttributeOption("FramesPerSecondAttribute");

	/** Name of the FramesPerSecondOverride media option. */
	static FName FramesPerSecondOverrideOption("FramesPerSecondOverride");

	/** Name of the ProxyOverride media option. */
	static FName ProxyOverrideOption("ProxyOverride");
}
