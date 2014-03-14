// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*================================================================================
	Platform.h: Setup the includes for the platform
==================================================================================*/
#pragma once

#if PLATFORM_APPLE
	#include <stddef.h> // needed for size_t
#endif

#include "GenericPlatform/GenericPlatform.h"

//---------------------------------------------------------
// Identify the current platform and include that header
//---------------------------------------------------------

//@port Identify the platform here and include the platform header to setup the platform types, etc
#if PLATFORM_WINDOWS
	#include "Windows/WindowsPlatform.h"
#elif PLATFORM_PS4
	#include "PS4/PS4Platform.h"
#elif PLATFORM_XBOXONE
	#include "XboxOne/XboxOnePlatform.h"
#elif PLATFORM_MAC
	#include "Mac/MacPlatform.h"
#elif PLATFORM_IOS
	#include "IOS/IOSPlatform.h"
#elif PLATFORM_ANDROID
	#include "Android/AndroidPlatform.h"
#elif PLATFORM_WINRT_ARM
	#include "WinRT/WinRTARMPlatform.h"
#elif PLATFORM_WINRT
	#include "WinRT/WinRTPlatform.h"
#elif PLATFORM_HTML5
	#include "HTML5/HTML5Platform.h"
#elif PLATFORM_LINUX
	#include "Linux/LinuxPlatform.h"
#else
	#error Unknown Compiler
#endif

// define all other platforms to be zero
//@port Define the platform here to be zero when compiling for other platforms
#if !defined(PLATFORM_WINDOWS)
	#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_XBOXONE)
	#define PLATFORM_XBOXONE 0
#endif
#if !defined(PLATFORM_MAC)
	#define PLATFORM_MAC 0
#endif
#if !defined(PLATFORM_PS4)
	#define PLATFORM_PS4 0
#endif
#if !defined(PLATFORM_IOS)
	#define PLATFORM_IOS 0
#endif
#if !defined(PLATFORM_ANDROID)
	#define PLATFORM_ANDROID 0
#endif
#if !defined(PLATFORM_WINRT)
	#define PLATFORM_WINRT 0
#endif
#if !defined(PLATFORM_WINRT_ARM)
	#define PLATFORM_WINRT_ARM	0
#endif
#if !defined(PLATFORM_APPLE)
	#define PLATFORM_APPLE 0
#endif
#if !defined(PLATFORM_HTML5)
	#define PLATFORM_HTML5 0
#endif
#if !defined(PLATFORM_LINUX)
	#define PLATFORM_LINUX 0
#endif

//------------------------------------------------------------------
// Finalize define setup
//------------------------------------------------------------------

// Base defines, must define these for the platform, there are no defaults
#ifndef PLATFORM_DESKTOP
	#error "PLATFORM_DESKTOP must be defined"
#endif
#ifndef PLATFORM_64BITS
	#error "PLATFORM_64BITS must be defined"
#endif

// Base defines, these have defaults
#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN				0
#endif
#ifndef PLATFORM_EXCEPTIONS_DISABLED
	#define PLATFORM_EXCEPTIONS_DISABLED		!PLATFORM_DESKTOP
#endif
#ifndef PLATFORM_SEH_EXCEPTIONS_DISABLED
	#define PLATFORM_SEH_EXCEPTIONS_DISABLED	0
#endif
#ifndef PLATFORM_SUPPORTS_PRAGMA_PACK
	#define PLATFORM_SUPPORTS_PRAGMA_PACK		0
#endif
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
	#define PLATFORM_ENABLE_VECTORINTRINSICS	0
#endif
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS_NEON
	#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON	0
#endif
#ifndef PLATFORM_USE_LS_SPEC_FOR_WIDECHAR
	#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR	1
#endif
#ifndef PLATFORM_USE_SYSTEM_VSWPRINTF
	#define PLATFORM_USE_SYSTEM_VSWPRINTF		1
#endif
#ifndef PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG
	#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG			0
#endif
#ifndef PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#define PLATFORM_COMPILER_HAS_GENERIC_KEYWORD	0
#endif
#ifndef PLATFORM_COMPILER_HAS_RVALUE_REFERENCES
	#define PLATFORM_COMPILER_HAS_RVALUE_REFERENCES	1
#endif
#ifndef PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	#define PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS	1
#endif
#ifndef PLATFORM_COMPILER_HAS_RANGED_FOR_LOOP
	#define PLATFORM_COMPILER_HAS_RANGED_FOR_LOOP	1
#endif
#ifndef PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
	#define PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES	1
#endif
#ifndef PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS
	#define PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS	1
#endif
#ifndef PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION
	#define PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION 0
#endif
#ifndef PLATFORM_TCHAR_IS_4_BYTES
	#define PLATFORM_TCHAR_IS_4_BYTES			0
#endif
#ifndef PLATFORM_HAS_vsnprintf
	#define PLATFORM_HAS_vsnprintf				1
#endif
#ifndef PLATFORM_HAS_BSD_TIME
	#define PLATFORM_HAS_BSD_TIME				1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKETS
	#define PLATFORM_HAS_BSD_SOCKETS			1
#endif
#ifndef PLATFORM_USE_PTHREADS
	#define PLATFORM_USE_PTHREADS				1
#endif
#ifndef PLATFORM_MAX_FILEPATH_LENGTH
	#define PLATFORM_MAX_FILEPATH_LENGTH		128
#endif
#ifndef PLATFORM_SUPPORTS_TEXTURE_STREAMING
	#define PLATFORM_SUPPORTS_TEXTURE_STREAMING	1
#endif
#ifndef PLATFORM_SUPPORTS_VIRTUAL_TEXTURES
#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURES		0
#endif
#ifndef PLATFORM_USES_DYNAMIC_RHI
	#define PLATFORM_USES_DYNAMIC_RHI			PLATFORM_DESKTOP
#endif
#ifndef PLATFORM_REQUIRES_FILESERVER
	#define PLATFORM_REQUIRES_FILESERVER		0
#endif
#ifndef PLATFORM_SUPPORTS_MULTITHREADED_GC
	#define PLATFORM_SUPPORTS_MULTITHREADED_GC	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME	1
#endif
#ifndef PLATFORM_HAS_NO_EPROCLIM
	#define PLATFORM_HAS_NO_EPROCLIM			0
#endif
#ifndef PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS
	#define PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS 0
#endif

#ifndef PLATFORM_SUPPORTS_DRAW_MESH_EVENTS
	#define PLATFORM_SUPPORTS_DRAW_MESH_EVENTS	1
#endif

#ifndef PLATFORM_SUPPORTS_CanConvertPointerFromTo
	#define PLATFORM_SUPPORTS_CanConvertPointerFromTo		1
#endif

#ifndef PLATFORM_USES_ES2
	#define PLATFORM_USES_ES2					0
#endif

#ifndef PLATFORM_SUPPORTS_TBB
	#define PLATFORM_SUPPORTS_TBB 0
#endif

#ifndef PLATFORM_SUPPORTS_JEMALLOC
	#define PLATFORM_SUPPORTS_JEMALLOC 0
#endif

#ifndef PLATFORM_CAN_SUPPORT_EDITORONLY_DATA
	#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA 0
#endif

#ifndef PLATFORM_SUPPORTS_NAMED_PIPES
	#define PLATFORM_SUPPORTS_NAMED_PIPES		0
#endif

#ifndef PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS
	#define PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS	1
#endif

#ifndef PLATFORM_HAS_TOUCH_MAIN_SCREEN
	#define PLATFORM_HAS_TOUCH_MAIN_SCREEN		0
#endif

#ifndef PLATFORM_HAS_128BIT_ATOMICS
	#define PLATFORM_HAS_128BIT_ATOMICS 0
#endif

//These are deprecated old defines that we want to make sure are not used
#define CONSOLE (#)
#define MOBILE (#)
#define PLATFORM_CONSOLE (#)

// These is computed, not predefined
#define PLATFORM_32BITS					(!PLATFORM_64BITS)

// not supported by the platform system yet or maybe ever
#define PLATFORM_VTABLE_AT_END_OF_CLASS 0 

#ifndef VARARGS
	#define VARARGS					/* Functions with variable arguments */
#endif
#ifndef CDECL
	#define CDECL	    			/* Standard C function */
#endif
#ifndef STDCALL
	#define STDCALL					/* Standard calling convention */
#endif
#ifndef FORCEINLINE
	#define FORCEINLINE 			/* Force code to be inline */
#endif
#ifndef FORCENOINLINE
	#define FORCENOINLINE 			/* Force code to NOT be inline */
#endif
#ifndef RESTRICT
	#define RESTRICT __restrict		/* no alias hint */
#endif

#ifndef ASSUME						/* Hints compiler that expression is true; generally restricted to comparisons against constants */
	#define ASSUME(...) 
#endif

// Optimization macros (uses __pragma to enable inside a #define).
#ifndef PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL
#endif

#ifndef FORCEINLINE_DEBUGGABLE_ACTUAL
	#define FORCEINLINE_DEBUGGABLE_ACTUAL inline
#endif

#ifndef DECLARE_UINT64						
	#define DECLARE_UINT64(x) x##ULL	/* Define a 64 bit immediate int **/
#endif

// Backwater of the spec. All compilers support this except microsoft, and they will soon
#ifndef TYPENAME_OUTSIDE_TEMPLATE
	#define TYPENAME_OUTSIDE_TEMPLATE	typename
#endif

// Method modifiers
#ifndef OVERRIDE						
	#define OVERRIDE
#endif
#ifndef FINAL						
	#define FINAL
#endif
#ifndef ABSTRACT						
	#define ABSTRACT
#endif
#ifndef CONSTEXPR
	#define CONSTEXPR
#endif

// String constants
#ifndef LINE_TERMINATOR						
	#define LINE_TERMINATOR TEXT("\n")
#endif
#ifndef LINE_TERMINATOR_ANSI
	#define LINE_TERMINATOR_ANSI "\n"
#endif

// Alignment.
#ifndef GCC_PACK						
	#define GCC_PACK(n)
#endif
#ifndef GCC_ALIGN						
	#define GCC_ALIGN(n)
#endif
#ifndef MS_ALIGN						
	#define MS_ALIGN(n)
#endif

// MSVC pragmas - used so other platforms can remove them easily (by not defining this)
#ifndef MSVC_PRAGMA
	#define MSVC_PRAGMA(...)
#endif


// Inlining
#ifndef PRAGMA_DISABLE_INLINING
	#define PRAGMA_DISABLE_INLINING	
	#define PRAGMA_ENABLE_INLINING	
#endif

// Cache control
#ifndef FLUSH_CACHE_LINE
	#define FLUSH_CACHE_LINE(x)
#endif

// Prefetch
#ifndef CACHE_LINE_SIZE
	#define CACHE_LINE_SIZE	128
#endif

// These have to be forced inline on some OSes so the dynamic loader will not 
// resolve to our allocators for the system libraries.
#ifndef OPERATOR_NEW_INLINE
	#define OPERATOR_NEW_INLINE FORCEINLINE
#endif

#ifndef OPERATOR_NEW_THROW_SPEC
	#define OPERATOR_NEW_THROW_SPEC
#endif
#ifndef OPERATOR_DELETE_THROW_SPEC
	#define OPERATOR_DELETE_THROW_SPEC
#endif

#ifndef checkAtCompileTime
	#define checkAtCompileTime(expr, msg)  static_assert(expr, #msg)
#endif

// DLL export and import definitions
#ifndef DLLEXPORT
	#define DLLEXPORT
	#define DLLIMPORT
#endif

// explicit bool support
namespace FHasOperatorImpl
{
	struct FNotSpecified {};

	template <typename T>
	struct FReturnValueCheck
	{
		static char (&Func())[2];
	};

	template <>
	struct FReturnValueCheck<FNotSpecified>
	{
		static char (&Func())[1];
	};

	template <typename T>
	FNotSpecified operator==(const T&, const T&);

	template <typename T>
	FNotSpecified operator!=(const T&, const T&);

	template <typename T>
	const T& Make();

	template <typename T>
	struct Equals
	{
		enum { Value = sizeof(FReturnValueCheck<decltype(Make<T>() == Make<T>())>::Func()) == sizeof(char[2]) };
	};

	template <typename T>
	struct NotEquals
	{
		enum { Value = sizeof(FReturnValueCheck<decltype(Make<T>() != Make<T>())>::Func()) == sizeof(char[2]) };
	};
}

template <typename T>
struct THasOperatorEquals
{
	enum { Value = FHasOperatorImpl::Equals<T>::Value };
};

template <typename T>
struct THasOperatorNotEquals
{
	enum { Value = FHasOperatorImpl::NotEquals<T>::Value };
};

#if PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL FORCEINLINE explicit operator bool
	#define SAFE_BOOL_OPERATORS(...)		// not needed when compiler supports explicit operator bool()
#elif PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION
	// unsafe version
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL FORCEINLINE operator bool
	#define SAFE_BOOL_OPERATORS(...)
#else
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL \
					private: \
						template <typename TheClassType> \
						static TheClassType UE_ClassTypeHelper(const TheClassType&); \
						struct UE_PrivateBoolType \
						{ \
							int x; \
						}; \
					public: \
						FORCEINLINE operator int UE_PrivateBoolType::*() const \
						{ \
							checkAtCompileTime(THasOperatorEquals   <decltype(UE_ClassTypeHelper(*this))>::Value, "Class_needs_operator_equals"); \
							checkAtCompileTime(THasOperatorNotEquals<decltype(UE_ClassTypeHelper(*this))>::Value, "Class_needs_operator_not_equals"); \
							return UE_OperatorBool() ? &UE_PrivateBoolType::x : 0; \
						} \
						FORCEINLINE bool UE_OperatorBool

	// these are left unimplemented to cause link errors if used
	#define SAFE_BOOL_OPERATORS(...)	\
			void operator ==( const __VA_ARGS__ & RHS ) const; \
			void operator !=( const __VA_ARGS__ & RHS ) const;
#endif

//--------------------------------------------------------------------------------------------
// POD types refactor for porting old code:
//
// Replace 'FLOAT' with 'float'
// Replace 'DOUBLE' with 'double'
// Replace 'INT' with 'int32' (make sure not to replace INT text in localization strings!)
// Replace 'UINT' with 'uint32'
// Replace 'DWORD' with 'uint32' (DWORD is still used in Windows platform source files)
// Replace 'WORD' with 'uint16'
// Replace 'USHORT' with 'uint16'
// Replace 'SWORD' with 'int16'
// Replace 'QWORD' with 'uint64'
// Replace 'SQWORD' with 'int64'
// Replace 'BYTE' with 'uint8'
// Replace 'SBYTE' with 'int8'
// Replace 'BITFIELD' with 'uint32'
// Replace 'UBOOL' with 'bool'
// Replace 'FALSE' with 'false' and 'TRUE' with true.
// Make sure any platform API uses its own bool types. 
// For example WinAPI uses BOOL, FALSE and TRUE otherwise sometimes it doesn't work properly
// or it doesn't compile.
// Replace 'ExtractUBOOLFromBitfield' with 'ExtractBoolFromBitfield'
// Replace 'SetUBOOLInBitField' with 'SetBoolInBitField'
// Replace 'ParseUBOOL' with 'FParse::Bool'
// Replace 'MINBYTE', with 'MIN_uint8'
// Replace 'MINWORD', with 'MIN_uint16'
// Replace 'MINDWORD', with 'MIN_uint32'
// Replace 'MINSBYTE', with 'MIN_int8'
// Replace 'MINSWORD', with 'MIN_int16'
// Replace 'MAXINT', with 'MAX_int32'
// Replace 'MAXBYTE', with 'MAX_uint8'
// Replace 'MAXWORD', with 'MAX_uint16'
// Replace 'MAXDWORD', with 'MAX_uint32'
// Replace 'MAXSBYTE', with 'MAX_int8'
// Replace 'MAXSWORD', with 'MAX_int16'
// Replace 'MAXINT', with 'MAX_int32'
//--------------------------------------------------------------------------------------------

//------------------------------------------------------------------
// Transfer the platform types to global types
//------------------------------------------------------------------

// Unsigned base types.
typedef FPlatformTypes::uint8		uint8;		///< An 8-bit unsigned integer.
typedef FPlatformTypes::uint16		uint16;		///< A 16-bit unsigned integer.
typedef FPlatformTypes::uint32		uint32;		///< A 32-bit unsigned integer.
typedef FPlatformTypes::uint64		uint64;		///< A 64-bit unsigned integer.

// Signed base types.
typedef	FPlatformTypes::int8		int8;		///< An 8-bit signed integer.
typedef FPlatformTypes::int16		int16;		///< A 16-bit signed integer.
typedef FPlatformTypes::int32		int32;		///< A 32-bit signed integer.
typedef FPlatformTypes::int64		int64;		///< A 64-bit signed integer.

// Character types.
typedef FPlatformTypes::ANSICHAR	ANSICHAR;	///< An ANSI character. Normally a signed type.
typedef FPlatformTypes::WIDECHAR	WIDECHAR;	///< A wide character. Normally a signed type.
typedef FPlatformTypes::TCHAR		TCHAR;		///< Either ANSICHAR or WIDECHAR, depending on whether the platform supports wide characters or the requirements of the licensee.
typedef FPlatformTypes::CHAR8		UTF8CHAR;	///< An 8-bit character containing a UTF8 (Unicode, 8-bit, variable-width) code unit.
typedef FPlatformTypes::CHAR16		UCS2CHAR;	///< A 16-bit character containing a UCS2 (Unicode, 16-bit, fixed-width) code unit, used for compatibility with 'Windows TCHAR' across multiple platforms.
typedef FPlatformTypes::CHAR16		UTF16CHAR;	///< A 16-bit character containing a UTF16 (Unicode, 16-bit, variable-width) code unit.
typedef FPlatformTypes::CHAR32		UTF32CHAR;	///< A 32-bit character containing a UTF32 (Unicode, 32-bit, fixed-width) code unit.

typedef FPlatformTypes::UPTRINT UPTRINT;	///< An unsigned integer the same size as a pointer
typedef FPlatformTypes::PTRINT PTRINT;		///< A signed integer the same size as a pointer
typedef FPlatformTypes::SIZE_T SIZE_T;		///< A signed integer the same size as a pointer

typedef FPlatformTypes::TYPE_OF_NULL	TYPE_OF_NULL; ///< The type of the NULL constant.

//------------------------------------------------------------------
// Test the global types
//------------------------------------------------------------------
namespace TypeTests
{
	template <typename A, typename B>
	struct TAreTypesEqual
	{
		enum { Value = false };
	};

	template <typename T>
	struct TAreTypesEqual<T, T>
	{
		enum { Value = true };
	};

	checkAtCompileTime(!PLATFORM_TCHAR_IS_4_BYTES || sizeof(TCHAR) == 4,TypeTests_TCHAR_size);
	checkAtCompileTime(PLATFORM_TCHAR_IS_4_BYTES || sizeof(TCHAR) == 2,TypeTests_TCHAR_size);

	checkAtCompileTime(PLATFORM_32BITS || PLATFORM_64BITS,TypeTests_pointer_size);
	checkAtCompileTime(PLATFORM_32BITS != PLATFORM_64BITS,TypeTests_pointer_exclusive);
	checkAtCompileTime(!PLATFORM_64BITS || sizeof(void*) == 8,TypeTests_pointer_size_64bit_but_short_pointers);
	checkAtCompileTime(PLATFORM_64BITS || sizeof(void*) == 4,TypeTests_pointer_size_32bit_but_long_pointers);

	checkAtCompileTime(char(-1) < char(0),TypeTests_char_unsigned);

	checkAtCompileTime((!TAreTypesEqual<ANSICHAR, WIDECHAR>::Value),TypeTests_ANSICHAR_and_WIDECHAR_should_be_different_types);
	checkAtCompileTime((!TAreTypesEqual<ANSICHAR, UCS2CHAR>::Value),TypeTests_ANSICHAR_and_CHAR16_should_be_different_types);
	checkAtCompileTime((!TAreTypesEqual<WIDECHAR, UCS2CHAR>::Value),TypeTests_WIDECHAR_and_CHAR16_should_be_different_types);
	checkAtCompileTime((TAreTypesEqual<TCHAR, ANSICHAR>::Value || TAreTypesEqual<TCHAR, WIDECHAR>::Value),TypeTests_TCHAR_should_either_be_ANSICHAR_or_WIDECHAR);

	checkAtCompileTime(sizeof(uint8) == 1,TypeTests_BYTE_size);
	checkAtCompileTime(int32(uint8(-1)) == 0xFF,TypeTests_BYTE_sign);

	checkAtCompileTime(sizeof(uint16) == 2,TypeTests_WORD_size);
	checkAtCompileTime(int32(uint16(-1)) == 0xFFFF,TypeTests_WORD_sign);

	checkAtCompileTime(sizeof(uint32) == 4,TypeTests_DWORD_size);
	checkAtCompileTime(int64(uint32(-1)) == int64(0xFFFFFFFF),TypeTests_DWORD_sign);

	checkAtCompileTime(sizeof(uint64) == 8,TypeTests_QWORD_size);
	checkAtCompileTime(uint64(-1) > uint64(0),TypeTests_QWORD_sign);


	checkAtCompileTime(sizeof(int8) == 1,TypeTests_SBYTE_size);
	checkAtCompileTime(int32(int8(-1)) == -1,TypeTests_SBYTE_sign);

	checkAtCompileTime(sizeof(int16) == 2,TypeTests_SWORD_size);
	checkAtCompileTime(int32(int16(-1)) == -1,TypeTests_SWORD_sign);

	checkAtCompileTime(sizeof(int32) == 4,TypeTests_INT_size);
	checkAtCompileTime(int64(int32(-1)) == int64(-1),TypeTests_INT_sign);

	checkAtCompileTime(sizeof(int64) == 8,TypeTests_SQWORD_size);
	checkAtCompileTime(int64(-1) < int64(0),TypeTests_SQWORD_sign);

	checkAtCompileTime(sizeof(ANSICHAR) == 1,TypeTests_ANSICHAR_size);
	checkAtCompileTime(int32(ANSICHAR(-1)) == -1,TypeTests_ANSICHAR_sign);

	checkAtCompileTime(sizeof(WIDECHAR) == 2 || sizeof(WIDECHAR) == 4,TypeTests_WIDECHAR_size);

	checkAtCompileTime(sizeof(UCS2CHAR) == 2,TypeTests_UCS2CHAR_size);

	checkAtCompileTime(sizeof(uint32) == 4,TypeTests_BITFIELD_size);
	checkAtCompileTime(int64(uint32(-1)) == int64(0xFFFFFFFF),TypeTests_BITFIELD_sign);

	checkAtCompileTime(sizeof(PTRINT) == sizeof(void *),TypeTests_PTRINT_size);
	checkAtCompileTime(PTRINT(-1) < PTRINT(0),TypeTests_PTRINT_sign);

	checkAtCompileTime(sizeof(UPTRINT) == sizeof(void *),TypeTests_UPTRINT_size);
	checkAtCompileTime(UPTRINT(-1) > UPTRINT(0),TypeTests_UPTRINT_sign);

	checkAtCompileTime(sizeof(SIZE_T) == sizeof(void *),TypeTests_SIZE_T_size);
	checkAtCompileTime(SIZE_T(-1) > SIZE_T(0),TypeTests_SIZE_T_sign);
}
