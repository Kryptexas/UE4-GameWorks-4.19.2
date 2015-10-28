// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusInput.h"

#if OCULUS_TOUCH_SUPPORTED_PLATFORMS

#if PLATFORM_WINDOWS
	// Required for OVR_CAPIShim.c
	#include "AllowWindowsPlatformTypes.h"
#endif

#if !IS_MONOLITHIC // otherwise it will clash with OculusRift
#include <OVR_CAPIShim.c>
#endif // #if !IS_MONOLITHIC

#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif

#endif	// OCULUS_TOUCH_SUPPORTED_PLATFORMS