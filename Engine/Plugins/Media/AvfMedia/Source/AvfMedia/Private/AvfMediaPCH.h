// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Runtime/Core/Public/Core.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Runtime/Media/Public/IMediaAudioSink.h"
#include "Runtime/Media/Public/IMediaModule.h"
#include "Runtime/Media/Public/IMediaStringSink.h"
#include "Runtime/Media/Public/IMediaPlayerFactory.h"
#include "Runtime/Media/Public/IMediaTextureSink.h"

#import <AVFoundation/AVFoundation.h>

#if PLATFORM_MAC
	#include "CocoaThread.h"
#endif


/** Log category for the AvfMedia module. */
DECLARE_LOG_CATEGORY_EXTERN(LogAvfMedia, Log, All);
