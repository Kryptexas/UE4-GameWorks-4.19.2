// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	GenericPlatformCompilerPreSetup.h: pragmas, version checks and other things for generic compiler
==============================================================================================*/

#pragma once

#ifndef DEPRECATED
	// Sample usage (note slightly unintuitive syntax for classes and structs) :
	// 
	// DEPRECATED(4.6, "Message")
	// void Function();
	// 
	// struct DEPRECATED(4.6, "Message") MyStruct;
	// class DEPRECATED(4.6, "Message") MyClass;
	#define DEPRECATED(VERSION, MESSAGE)
#endif // DEPRECATED

#ifndef EMIT_DEPRECATED_WARNING_MESSAGE
	#define EMIT_DEPRECATED_WARNING_MESSAGE(Msg)
#endif // EMIT_DEPRECATED_WARNING_MESSAGE