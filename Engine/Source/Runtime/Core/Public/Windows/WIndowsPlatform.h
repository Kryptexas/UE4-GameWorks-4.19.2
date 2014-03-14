// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	WindowsPlatform.h: Setup for the windows platform
==================================================================================*/

#pragma once

// Setup Clang compilation support on Windows
#define PLATFORM_WINDOWS_CLANG defined(__clang__)

/**
* Windows specific types
**/
struct FWindowsPlatformTypes : public FGenericPlatformTypes
{
#ifdef _WIN64
	typedef unsigned __int64	SIZE_T;
#else
	typedef unsigned long		SIZE_T;
#endif
};

typedef FWindowsPlatformTypes FPlatformTypes;

// Base defines, must define these for the platform, there are no defaults
#define PLATFORM_DESKTOP					1
#if defined( _WIN64 )
	#define PLATFORM_64BITS					1
#else
	#define PLATFORM_64BITS					0
#endif
#if defined( _MANAGED ) || defined( _M_CEE )
	#define PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION	1
#endif
#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA	1

// Base defines, defaults are commented out

#define PLATFORM_LITTLE_ENDIAN						1
#if PLATFORM_WINDOWS_CLANG
	// @todo Clang Clang emits a compile error on Windows when __try/__except blocks are used (missing functionality in Clang.)
	//             After Clang adds proper support for SEH exceptions, we should delete this definition and all use cases of it!
	#define PLATFORM_SEH_EXCEPTIONS_DISABLED			1

	// @todo clang: This is needed because of a bug in Clang with RTTI and exceptions (even when RTTI is disabled!)  This manifests as a 
	// compile error whenever try/catch/throw is used.  See http://llvm.org/bugs/show_bug.cgi?id=17403.  This code should be removed 
	// after the bug is fixed in Clang!
	#define PLATFORM_EXCEPTIONS_DISABLED				1
#endif

#define PLATFORM_SUPPORTS_PRAGMA_PACK				1
#if PLATFORM_WINDOWS_CLANG
	#define PLATFORM_ENABLE_VECTORINTRINSICS		0
#else
	#define PLATFORM_ENABLE_VECTORINTRINSICS		1
#endif
//#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR			1
//#define PLATFORM_USE_SYSTEM_VSWPRINTF				1
//#define PLATFORM_TCHAR_IS_4_BYTES					0
#define PLATFORM_HAS_vsnprintf						0
#define PLATFORM_HAS_BSD_TIME						0
#define PLATFORM_USE_PTHREADS						0
#define PLATFORM_MAX_FILEPATH_LENGTH				MAX_PATH
#define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS	1
#define PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS		1
#define PLATFORM_SUPPORTS_TBB						1
#define PLATFORM_SUPPORTS_NAMED_PIPES				1
#define PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS	0
#define PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES	0
#define PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS	0

#define PLATFORM_HAS_128BIT_ATOMICS					PLATFORM_64BITS

#if _MSC_VER < 1700
	#define PLATFORM_COMPILER_HAS_RANGED_FOR_LOOP 0
#endif


// Function type macros.
#define VARARGS     __cdecl					/* Functions with variable arguments */
#define CDECL	    __cdecl					/* Standard C function */
#define STDCALL		__stdcall				/* Standard calling convention */
#define FORCEINLINE __forceinline			/* Force code to be inline */
#define FORCENOINLINE __declspec(noinline)	/* Force code to NOT be inline */

// Hints compiler that expression is true; generally restricted to comparisons against constants
#if !PLATFORM_WINDOWS_CLANG		// Clang doesn't support __assume (Microsoft specific)
	#define ASSUME(expr) __assume(expr)
#endif

#define DECLARE_UINT64(x)	x

// Optimization macros (uses __pragma to enable inside a #define).
#if !PLATFORM_WINDOWS_CLANG		// @todo clang: Clang doesn't appear to support optimization pragmas yet
	#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL __pragma(optimize("",off))
	#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL  __pragma(optimize("",on))
#endif

// Backwater of the spec. All compilers support this except microsoft, and they will soon
#if !PLATFORM_WINDOWS_CLANG		// Clang expects typename outside template
	#define TYPENAME_OUTSIDE_TEMPLATE
#endif

#pragma warning(disable : 4481) // nonstandard extension used: override specifier 'override'
#define OVERRIDE override
#if PLATFORM_WINDOWS_CLANG
	#define CONSTEXPR constexpr
#else
	#define FINAL sealed
#endif
#define ABSTRACT abstract

// Strings.
#define LINE_TERMINATOR TEXT("\r\n")
#define LINE_TERMINATOR_ANSI "\r\n"

// Alignment.
#if PLATFORM_WINDOWS_CLANG
	#define GCC_PACK(n) __attribute__((packed,aligned(n)))
	#define GCC_ALIGN(n) __attribute__((aligned(n)))
#else
	#define MS_ALIGN(n) __declspec(align(n))
#endif

// Pragmas
#define MSVC_PRAGMA(Pragma) __pragma(Pragma)

// Prefetch
#define CACHE_LINE_SIZE	128

// DLL export and import definitions
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

// disable this now as it is annoying for generic platform implementations
#pragma warning(disable : 4100) // unreferenced formal parameter


// Include code analysis features
#include "WindowsPlatformCodeAnalysis.h"
