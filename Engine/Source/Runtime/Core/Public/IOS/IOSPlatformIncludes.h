// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	IOSPlatformIncludes.h: Includes the platform specific headers for iOS
==================================================================================*/

#pragma once

// Set up compiler pragmas, etc
#include "IOS/IOSPlatformCompilerSetup.h"

#define FVector FVectorWorkaround
#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>
#else //@todo - zombie - iOS should always have Obj-C to function. This definition guard may be unnecessary. -astarkey
#error "Objective C is undefined."
#endif
#undef check
#undef verify
#undef FVector

#undef TYPE_BOOL

#include <CoreFoundation/CoreFoundation.h>

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
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/sysctl.h>

// SIMD intrinsics
#if WITH_SIMULATOR
#include <xmmintrin.h>
#else
#include <arm_neon.h>
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

/*----------------------------------------------------------------------------
	Memory. On Mac OS X malloc allocates memory aligned to 16 bytes.
----------------------------------------------------------------------------*/
#define _aligned_malloc(Size,Align) malloc(Size)
#define _aligned_realloc(Ptr,Size,Align) realloc(Ptr,Size)
#define _aligned_free(Ptr) free(Ptr)

// include platform implementations
#include "IOS/IOSPlatformMemory.h"
#include "Apple/ApplePlatformString.h"
#include "IOS/IOSPlatformMisc.h"
#include "Apple/ApplePlatformStackWalk.h"
#include "IOS/IOSPlatformMath.h"
#include "Apple/ApplePlatformTime.h"
#include "IOS/IOSPlatformProcess.h"
#include "IOS/IOSPlatformOutputDevices.h"
#include "Apple/ApplePlatformAtomics.h"
#include "Apple/ApplePlatformTLS.h"
#include "IOS/IOSPlatformSplash.h"
#include "Apple/ApplePlatformFile.h"
#include "IOS/IOSPlatformSurvey.h"
#include "IOSAsyncTask.h"
#include "IOS/IOSPlatformHttp.h"

// include platform properties and typedef it for the runtime

#include "IOS/IOSPlatformProperties.h"

typedef FIOSPlatformProperties FPlatformProperties;
