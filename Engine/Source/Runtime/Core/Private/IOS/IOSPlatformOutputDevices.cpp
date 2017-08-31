// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSPlatformOutputDevices.mm: iOS implementations of OutputDevices functions
=============================================================================*/

#include "IOSPlatformOutputDevices.h"
#include "Misc/OutputDeviceFile.h"

//////////////////////////////////
// FIOSPlatformOutputDevices
//////////////////////////////////

class FOutputDevice*	FIOSPlatformOutputDevices::GetLog()
{
    static FOutputDeviceFile Singleton(nullptr, true);
    return &Singleton;
}

//class FFeedbackContext*				FIOSPlatformOutputDevices::GetWarn()
//{
//	static FOutputDeviceIOSDebug Singleton;
//	return &Singleton;
//}
