// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==========================================================================================================
	MacPlatformCompilerPreSetup.h: setup for the compiler that needs to be defined before everything else
===========================================================================================================*/

#pragma once

#ifndef DISABLE_DEPRECATION
	#pragma clang diagnostic warning "-Wdeprecated-declarations"

	// Sample usage (note slightly unintuitive syntax for classes and structs) :
	// 
	// DEPRECATED(4.6, "Message")
	// void Function();
	// 
	// struct DEPRECATED(4.6, "Message") MyStruct;
	// class DEPRECATED(4.6, "Message") MyClass;
	#define DEPRECATED(VERSION, MESSAGE) __attribute__((deprecated(MESSAGE " Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.")))
#endif // DISABLE_DEPRECATION
