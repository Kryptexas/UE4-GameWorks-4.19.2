// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==========================================================================================================
	WindowsPlatformCompilerPreSetup.h: setup for the compiler that needs to be defined before everything else
===========================================================================================================*/

#pragma once

#ifndef DISABLE_DEPRECATION
	#define DEPRECATED(VERSION, MESSAGE) __declspec(deprecated(MESSAGE))
#else
	#define DEPRECATED(VERSION, MESSAGE)
#endif // DISABLE_DEPRECATION
