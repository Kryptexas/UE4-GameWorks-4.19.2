// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

// you can enable this plug-in on Windows by compiling the Engine and Editor
// against Windows 7. By default, UE4 is targeting Windows Vista. The following
// changes have to be made in SetupEnvironment() in UEBuildWindows.cs:
//
//   // Windows Vista or higher required
//   InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_WIN32_WINNT=0x0601");
//   InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WINVER=0x0601");

#define MFMEDIA_SUPPORTED_PLATFORM (PLATFORM_XBOXONE || (PLATFORM_WINDOWS && WINVER >= 0x0601))


#include "Runtime/Core/Public/CoreMinimal.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Runtime/Core/Public/Misc/ScopeLock.h"

#if MFMEDIA_SUPPORTED_PLATFORM
	#include "Runtime/Core/Public/Async/Async.h"
	#include "Runtime/Media/Public/IMediaAudioSink.h"
	#include "Runtime/Media/Public/IMediaModule.h"
	#include "Runtime/Media/Public/IMediaOptions.h"
	#include "Runtime/Media/Public/IMediaOverlaySink.h"
	#include "Runtime/Media/Public/IMediaPlayerFactory.h"
	#include "Runtime/Media/Public/IMediaTextureSink.h"
	#include "Runtime/RenderCore/Public/RenderingThread.h"

	#if PLATFORM_WINDOWS
		#include "WindowsHWrapper.h"
	#endif
	#include "Runtime/Core/Public/Windows/AllowWindowsPlatformTypes.h"
	#if PLATFORM_WINDOWS
		#include <windows.h>
		#include <propvarutil.h>
		#include <shlwapi.h>
	#endif
	#include <mfapi.h>
	#include <mferror.h>
	#include <mfidl.h>
	#include <Mfreadwrite.h>
	#include "Runtime/Core/Public/Windows/COMPointer.h"
	#include "Runtime/Core/Public/Windows/HideWindowsPlatformTypes.h"

#elif PLATFORM_WINDOWS
	#include "CoreDefines.h"
	#pragma message("Skipping MfMedia (requires WINVER >= 0x0601, but WINVER is " PREPROCESSOR_TO_STRING(WINVER) ")")

#endif //MFMEDIA_SUPPORTED_PLATFORM


/** Log category for the MfMedia module. */
DECLARE_LOG_CATEGORY_EXTERN(LogMfMedia, Log, All);
