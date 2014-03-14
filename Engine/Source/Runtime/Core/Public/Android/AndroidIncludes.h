// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	AndroidIncludes.h: Includes the platform specific headers for Android
==================================================================================*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <wctype.h>
#include <pthread.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>

// Set up compiler pragmas, etc
#include "Android/AndroidCompilerSetup.h"

//@todo android: verify malloc alignment or change?
#define _aligned_malloc(Size,Align) malloc(Size)
#define _aligned_realloc(Ptr,Size,Align) realloc(Ptr,Size)
#define _aligned_free(Ptr) free(Ptr)

int vswprintf( TCHAR *dst, int count, const TCHAR *fmt, va_list arg );

/*
// Define TEXT early	//@todo android:
#define TEXT(x) L##x

// map the Windows functions (that UE4 unfortunately uses be default) to normal functions
#define _alloca alloca
*/

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
#include "Android/AndroidMemory.h"
#include "Android/AndroidString.h"
#include "Android/AndroidMisc.h"
#include "Android/AndroidStackWalk.h"
#include "Android/AndroidMath.h"
#include "Android/AndroidTime.h"
#include "Android/AndroidProcess.h"
#include "Android/AndroidOutputDevices.h"
#include "Android/AndroidAtomics.h"
#include "Android/AndroidTLS.h"
#include "Android/AndroidSplash.h"
#include "Android/AndroidFile.h"
#include "Android/AndroidSurvey.h"
#include "Android/AndroidHttp.h"

// include platform properties and typedef it for the runtime
#include "AndroidProperties.h"
typedef FAndroidPlatformProperties FPlatformProperties;
