// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	GenericPlatformCompilerSetup.h: pragmas, version checks and other things for generic compiler
==============================================================================================*/

#pragma once

#ifndef DEPRECATED

	#ifndef DISABLE_DEPRECATION
		#define DEPRECATED(VERSION, MESSAGE)
	#else
		#define DEPRECATED(VERSION, MESSAGE)
	#endif // DISABLE_DEPRECATION

#endif // DEPRECATED