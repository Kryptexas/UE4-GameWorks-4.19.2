// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#ifndef STUBBED
#define STUBBED(x)	\
	do																								\
	{																								\
		static bool AlreadySeenThisStubbedSection = false;											\
		if (!AlreadySeenThisStubbedSection)															\
		{																							\
			AlreadySeenThisStubbedSection = true;													\
			fprintf(stderr, "STUBBED: %s at %s:%d (%s)\n", x, __FILE__, __LINE__, __FUNCTION__);	\
		}																							\
	} while (0)
#endif

/*----------------------------------------------------------------------------
Metadata macros.
----------------------------------------------------------------------------*/

#define CPP       1
#define STRUCTCPP 1
#define DEFAULTS  0


/*-----------------------------------------------------------------------------
Seek-free defines.
-----------------------------------------------------------------------------*/

#define STANDALONE_SEEKFREE_SUFFIX	TEXT("_SF")


/*-----------------------------------------------------------------------------
Enabling formatting string type checking on varargs.  Intended to be defined
on permanently when all usage is fixed.
-----------------------------------------------------------------------------*/

#define USE_FORMAT_STRING_TYPE_CHECKING 0
