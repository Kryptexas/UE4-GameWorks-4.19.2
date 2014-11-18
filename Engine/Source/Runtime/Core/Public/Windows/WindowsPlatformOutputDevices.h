// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformOutputDevices.h"


struct CORE_API FWindowsPlatformOutputDevices
	: public FGenericPlatformOutputDevices
{
	static FOutputDevice*			GetEventLog();
	static FOutputDeviceConsole*	GetLogConsole();
	static FOutputDeviceError*		GetError();
	static FFeedbackContext*		GetWarn();
};


typedef FWindowsPlatformOutputDevices FPlatformOutputDevices;
