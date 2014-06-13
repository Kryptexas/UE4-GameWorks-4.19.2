// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==========================================================================================================
	WindowsPlatformCompilerPreSetup.h: setup for the compiler that needs to be defined before everything else
===========================================================================================================*/

#pragma once

#ifndef DISABLE_DEPRECATION
	#define DEPRECATED(VERSION, MESSAGE) __declspec(deprecated(MESSAGE " Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile."))

	#ifndef EMIT_DEPRECATED_WARNING_MESSAGE_STR
		#define EMIT_DEPRECATED_WARNING_MESSAGE_STR1(x) #x
		#define EMIT_DEPRECATED_WARNING_MESSAGE_STR(x) EMIT_DEPRECATED_WARNING_MESSAGE_STR1(x)
	#endif // EMIT_DEPRECATED_WARNING_MESSAGE_STR

	#define EMIT_DEPRECATED_WARNING_MESSAGE(Msg) __pragma(message(__FILE__ "(" EMIT_DEPRECATED_WARNING_MESSAGE_STR(__LINE__) "): warning C4996: " Msg))
#else
	#define DEPRECATED(VERSION, MESSAGE)
	#define EMIT_DEPRECATED_WARNING_MESSAGE(Msg)
#endif // DISABLE_DEPRECATION
