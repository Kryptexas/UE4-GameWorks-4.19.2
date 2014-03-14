// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	HTML5Includes.h: Includes the platform specific headers for HTML5
==================================================================================*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <wctype.h>
#include <limits.h>

// Set up compiler pragmas, etc
#include "HTML5/HTML5PlatformCompilerSetup.h"

// Define TEXT early
#define TEXT_PASTE(x) L ## x
#define TEXT_HELPER(a,b)	a ## b
#define TEXT(s)				TEXT_HELPER(L, s)

// map the Windows functions (that UE4 unfortunately uses be default) to normal functions
#if PLATFORM_HTML5_WIN32

#include <intrin.h>

#else

#define _alloca alloca

#endif

struct RECT
{
	int32 left;
	int32 top;
	int32 right;
	int32 bottom;
};

#define OUT
#define IN

// include platform implementations
#include "HTML5/HTML5PlatformMemory.h"
#include "HTML5/HTML5PlatformString.h"
#include "HTML5/HTML5PlatformMisc.h"
#include "HTML5/HTML5PlatformStackWalk.h"
#include "HTML5/HTML5PlatformMath.h"
#include "HTML5/HTML5PlatformTime.h"
#include "HTML5/HTML5PlatformProcess.h"
#include "HTML5/HTML5PlatformOutputDevices.h"
#include "HTML5/HTML5PlatformAtomics.h"
#include "HTML5/HTML5PlatformTLS.h"
#include "HTML5/HTML5PlatformSplash.h"
#include "HTML5/HTML5PlatformSurvey.h"
#include "HTML5/HTML5PlatformHttp.h"

// include platform properties and typedef it for the runtime
#include "HTML5PlatformProperties.h"
typedef FHTML5PlatformProperties FPlatformProperties;


