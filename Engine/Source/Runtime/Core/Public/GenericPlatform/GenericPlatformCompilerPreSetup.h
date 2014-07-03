// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	GenericPlatformCompilerPreSetup.h: pragmas, version checks and other things for generic compiler
==============================================================================================*/

#pragma once

#ifndef DEPRECATED
	#define DEPRECATED(VERSION, MESSAGE)
#endif // DEPRECATED

#ifndef EMIT_DEPRECATED_WARNING_MESSAGE
	#define EMIT_DEPRECATED_WARNING_MESSAGE(Msg)
#endif // EMIT_DEPRECATED_WARNING_MESSAGE