// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

#define WMFMEDIA_SUPPORTED_PLATFORM (PLATFORM_WINDOWS && (WINVER >= 0x0600 /*Vista*/))

#if WMFMEDIA_SUPPORTED_PLATFORM
	#include "../../WmfMediaFactory/Public/WmfMediaSettings.h"

	#include "WindowsHWrapper.h"
	#include "Windows/AllowWindowsPlatformTypes.h"

	#include <windows.h>
	
	#include <D3D9.h>
	#include <mfapi.h>
	#include <mferror.h>
	#include <mfidl.h>
	#include <mmdeviceapi.h>
	#include <mmeapi.h>
	#include <nserror.h>
	#include <shlwapi.h>

	const GUID FORMAT_VideoInfo = { 0x05589f80, 0xc356, 0x11ce, { 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a } };
	const GUID FORMAT_VideoInfo2 = { 0xf72a76A0, 0xeb0a, 0x11d0, { 0xac, 0xe4, 0x00, 0x00, 0xc0, 0xcc, 0x16, 0xba } };

	#if (WINVER < _WIN32_WINNT_WIN8)
		const GUID MF_LOW_LATENCY = { 0x9c27891a, 0xed7a, 0x40e1, { 0x88, 0xe8, 0xb2, 0x27, 0x27, 0xa0, 0x24, 0xee } };
	#endif

	#include "Windows/COMPointer.h"
	#include "Windows/HideWindowsPlatformTypes.h"

#elif PLATFORM_WINDOWS && !UE_SERVER
	#pragma message("Skipping WmfMedia (requires WINVER >= 0x0600, but WINVER is " PREPROCESSOR_TO_STRING(WINVER) ")")

#endif //WMFMEDIA_SUPPORTED_PLATFORM


/** Log category for the WmfMedia module. */
DECLARE_LOG_CATEGORY_EXTERN(LogWmfMedia, Log, All);
