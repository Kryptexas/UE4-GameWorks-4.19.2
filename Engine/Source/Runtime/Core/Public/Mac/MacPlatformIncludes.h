// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	MacPlatformIncludes.h: Includes the platform specific headers for Mac
==================================================================================*/

#pragma once

// Set up compiler pragmas, etc
#include "Mac/MacPlatformCompilerSetup.h"

#define FVector FVectorWorkaround
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif
#include <Carbon/Carbon.h>
#include <CoreServices/CoreServices.h>
#undef check
#undef verify
#undef FVector

#include <CoreFoundation/CoreFoundation.h>
#undef TYPE_BOOL

#include <string.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <sys/time.h>
#include <math.h>
#include <mach/mach_time.h>
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <libkern/OSAtomic.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <copyfile.h>
#include <utime.h>
#include <mach/mach_host.h>
#include <mach/task.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/sysctl.h>

// SIMD intrinsics
#include <xmmintrin.h>

struct RECT
{
	int32 left;
	int32 top;
	int32 right;
	int32 bottom;
};

#define OUT
#define IN

/*----------------------------------------------------------------------------
	Memory. On Mac OS X malloc allocates memory aligned to 16 bytes.
----------------------------------------------------------------------------*/
#define _aligned_malloc(Size,Align) malloc(Size)
#define _aligned_realloc(Ptr,Size,Align) realloc(Ptr,Size)
#define _aligned_free(Ptr) free(Ptr)

// include platform implementations
#include "Mac/MacPlatformMemory.h"
#include "Apple/ApplePlatformString.h"
#include "Apple/ApplePlatformCrashContext.h"
#include "Mac/MacPlatformMisc.h"
#include "Apple/ApplePlatformStackWalk.h"
#include "Mac/MacPlatformMath.h"
#include "Apple/ApplePlatformTime.h"
#include "Mac/MacPlatformProcess.h"
#include "Mac/MacPlatformOutputDevices.h"
#include "Apple/ApplePlatformAtomics.h"
#include "Apple/ApplePlatformTLS.h"
#include "Mac/MacPlatformSplash.h"
#include "Apple/ApplePlatformFile.h"
#include "Mac/MacPlatformSurvey.h"
#include "Mac/MacPlatformHttp.h"

// include platform properties and typedef it for the runtime

#include "Mac/MacPlatformProperties.h"

#ifndef WITH_EDITORONLY_DATA
#error "WITH_EDITORONLY_DATA must be defined"
#endif

#ifndef UE_SERVER
#error "WITH_EDITORONLY_DATA must be defined"
#endif

typedef FMacPlatformProperties<WITH_EDITORONLY_DATA, UE_SERVER> FPlatformProperties;

