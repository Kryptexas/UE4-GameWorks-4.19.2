// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	LinuxPlatform.h: Setup for the linux platform
==================================================================================*/

#pragma once

/**
* Linux specific types
**/
struct FLinuxPlatformTypes : public FGenericPlatformTypes
{
	typedef unsigned int		DWORD;
	typedef __SIZE_TYPE__		SIZE_T;
	typedef decltype(__null)	TYPE_OF_NULL;
};

typedef FLinuxPlatformTypes FPlatformTypes;

#define MAX_PATH				PATH_MAX

// Base defines, must define these for the platform, there are no defaults
#define PLATFORM_DESKTOP						1
#ifdef _LINUX64
	#define PLATFORM_64BITS						1
#else
	#define PLATFORM_64BITS						0
#endif
#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA	0

// Base defines, defaults are commented out

#define PLATFORM_LITTLE_ENDIAN						1
#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG 1
#define PLATFORM_SUPPORTS_PRAGMA_PACK				1
#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR			1
#define PLATFORM_TCHAR_IS_4_BYTES					1
#define PLATFORM_HAS_vsnprintf						0
#define PLATFORM_HAS_BSD_TIME						1
#define PLATFORM_USE_PTHREADS						1
#define PLATFORM_MAX_FILEPATH_LENGTH				MAX_PATH /* @todo linux: avoid using PATH_MAX as it is known to be broken */
#define PLATFORM_HAS_NO_EPROCLIM					1
#define PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL		1
#define PLATFORM_SUPPORTS_JEMALLOC					1
#define PLATFORM_EXCEPTIONS_DISABLED				1

// Function type macros.
#define VARARGS													/* Functions with variable arguments */
#define CDECL													/* Standard C function */
#define STDCALL													/* Standard calling convention */
#define FORCEINLINE inline __attribute__ ((always_inline))		/* Force code to be inline */
#define FORCENOINLINE __attribute__((noinline))					/* Force code to NOT be inline */

#define TEXT_HELPER(a,b)	a ## b
#define TEXT(s)				TEXT_HELPER(L, s)

// Optimization macros (uses _Pragma to enable inside a #define).
// @todo linux: do these actually work with clang?  (no, they do not, but keeping in in case we switch to gcc)
#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL _Pragma("optimize(\"\",off)")
#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL  _Pragma("optimize(\"\",on)")

#define OVERRIDE override
#define FINAL	 final
#define ABSTRACT abstract

// Alignment.
#define GCC_PACK(n)			__attribute__((packed,aligned(n)))
#define GCC_ALIGN(n)		__attribute__((aligned(n)))

// Operator new/delete handling.
#define OPERATOR_NEW_THROW_SPEC throw (std::bad_alloc)
#define OPERATOR_DELETE_THROW_SPEC throw()
