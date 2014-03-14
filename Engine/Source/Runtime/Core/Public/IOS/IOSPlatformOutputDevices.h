// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	IOSPlatformOutputDevices.h: iOS platform OutputDevices functions
==============================================================================================*/

#pragma once

struct CORE_API FIOSPlatformOutputDevices : public FGenericPlatformOutputDevices
{
	static class FOutputDevice*			GetLog();
	static class FOutputDeviceError*	GetError();
};

typedef FIOSPlatformOutputDevices FPlatformOutputDevices;
