// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==========================================================================================================
	MacPlatformCompilerPreSetup.h: setup for the compiler that needs to be defined before everything else
===========================================================================================================*/

#pragma once

#ifndef DISABLE_DEPRECATION
#define DEPRECATED(VERSION, MESSAGE) __attribute__((deprecated(MESSAGE " Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.")))
#else
	#define DEPRECATED(VERSION, MESSAGE)
#endif // DISABLE_DEPRECATION
