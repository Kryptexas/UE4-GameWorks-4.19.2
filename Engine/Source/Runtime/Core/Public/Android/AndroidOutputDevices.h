// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	AndroidOutputDevices.h: Android platform OutputDevices functions
==============================================================================================*/

#pragma once
struct CORE_API FAndroidOutputDevices : public FGenericPlatformOutputDevices
{
	static class FOutputDevice*			GetLog();
	static class FOutputDeviceError*	GetError();
};

typedef FAndroidOutputDevices FPlatformOutputDevices;
