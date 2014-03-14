// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	LinuxPlatformIncludes.h: Includes the platform specific headers for Linux
==================================================================================*/

#pragma once

// Set up compiler pragmas, etc
#include "Linux/LinuxPlatformCompilerSetup.h"

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
#include <wchar.h>
#include <wctype.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>
#include <utime.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/sysctl.h>
#ifdef __x86_64__
	#include <xmmintrin.h>
#endif // PLATFORM_RASPBERRY
#include <sys/utsname.h>
#include <libgen.h>

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
#include "Linux/LinuxPlatformMemory.h"
#include "Linux/LinuxPlatformString.h"
#include "Linux/LinuxPlatformMisc.h"
#include "Linux/LinuxPlatformStackWalk.h"
#include "Linux/LinuxPlatformMath.h"
#include "Linux/LinuxPlatformTime.h"
#include "Linux/LinuxPlatformProcess.h"
#include "Linux/LinuxPlatformOutputDevices.h"
#include "Linux/LinuxPlatformAtomics.h"
#include "Linux/LinuxPlatformTLS.h"
#include "Linux/LinuxPlatformSplash.h"
#include "Linux/LinuxPlatformFile.h"
#include "Linux/LinuxPlatformSurvey.h"
#include "Linux/LinuxPlatformHttp.h"


// include platform properties and typedef it for the runtime
#include "Linux/LinuxPlatformProperties.h"
typedef FLinuxPlatformProperties<UE_SERVER> FPlatformProperties;

