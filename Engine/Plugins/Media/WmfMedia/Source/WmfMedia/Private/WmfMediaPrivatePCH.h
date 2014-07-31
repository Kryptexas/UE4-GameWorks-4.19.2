// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/* Dependencies
 *****************************************************************************/

#include "ModuleManager.h"
#include "Media.h"

#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <shlwapi.h>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
#pragma warning(disable: 4191)
#ifndef DeleteFile
	#define DeleteFile DeleteFileW
#endif
#ifndef MoveFile
	#define MoveFile MoveFileW
#endif
#include "atlbase.h"
#undef DeleteFile
#undef MoveFile
#pragma warning(default: 4191)
#pragma warning(default: 4146)
#pragma warning(default: 4278)

#include "HideWindowsPlatformTypes.h"


/* Private macros
 *****************************************************************************/

DECLARE_LOG_CATEGORY_EXTERN(LogWmfMedia, Log, All);


/* Private includes
 *****************************************************************************/

#include "WmfMediaSampler.h"
#include "WmfMediaTrack.h"
#include "WmfMediaAudioTrack.h"
#include "WmfMediaCaptionTrack.h"
#include "WmfMediaVideoTrack.h"
#include "WmfMediaReadState.h"
#include "WmfMediaByteStream.h"
#include "WmfMediaSession.h"
#include "WmfMediaPlayer.h"
