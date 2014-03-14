// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformOutputDevices.h: Generic platform OutputDevices classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once

/**
* Generic implementation for most platforms
**/
struct CORE_API FGenericPlatformOutputDevices
{
	static FString						GetAbsoluteLogFilename();

	static class FOutputDevice*			GetLog();
	static class FOutputDevice*			GetEventLog()
	{
		return NULL; // normally only used for dedicated servers
	}
	static class FOutputDeviceConsole*	GetLogConsole()
	{
		return NULL; // normally only used for PC
	}
	static class FOutputDeviceError*	GetError();
	static class FFeedbackContext*		GetWarn();
};

