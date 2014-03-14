// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	WindowsPlatformOutputDevices.h: Windows platform OutputDevices functions
==============================================================================================*/

#pragma once

struct CORE_API FWindowsPlatformOutputDevices : public FGenericPlatformOutputDevices
{
	static class FOutputDevice*			GetEventLog();
	static class FOutputDeviceConsole*	GetLogConsole();
	static class FOutputDeviceError*	GetError();
	static class FFeedbackContext*		GetWarn();
};

typedef FWindowsPlatformOutputDevices FPlatformOutputDevices;

