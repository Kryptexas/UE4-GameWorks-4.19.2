///////////////////////////////////////////////////////////////////////
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2017 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#pragma once
#include <stddef.h>
#include <assert.h>


///////////////////////////////////////////////////////////////////////
// Storage-class specification

#ifndef ST_DLL_LINK
	#if (defined(_WIN32) || defined(_XBOX)) && defined(_USRDLL)
		#define ST_DLL_LINK __declspec(dllexport)
		#define ST_DLL_LINK_STATIC_MEMVAR __declspec(dllexport)
	#elif defined(ST_USE_SDK_AS_DLLS)
		#define ST_DLL_LINK
		#define ST_DLL_LINK_STATIC_MEMVAR __declspec(dllimport)
	#else
		#define ST_DLL_LINK
		#define ST_DLL_LINK_STATIC_MEMVAR
	#endif
#endif

// specify calling convention
#ifndef ST_CALL_CONV
	#if (defined(_WIN32) || defined(_XBOX)) && !defined(ST_CALL_CONV)
		#define ST_CALL_CONV   __cdecl
	#else
		#define ST_CALL_CONV
	#endif
#endif


///////////////////////////////////////////////////////////////////////
// Packing
// certain SpeedTree data structures require particular packing, so we set it
// explicitly. Comment this line out to override, but be sure to set packing to 4 before
// including Core.h or other key header files.

#define ST_SETS_PACKING_INTERNALLY
#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(push, 4)
#endif


///////////////////////////////////////////////////////////////////////
//  Inline macros

#ifndef ST_INLINE
	#define ST_INLINE inline
#endif
#ifndef ST_FORCE_INLINE
	#ifdef NDEBUG
		#ifdef __GNUC__
			#define ST_FORCE_INLINE inline __attribute__ ((always_inline))
		#else
			#define ST_FORCE_INLINE __forceinline
		#endif
	#else
		#define ST_FORCE_INLINE inline
	#endif
#endif


////////////////////////////////////////////////////////////
//  Unreferenced parameters

#define ST_UNREF_PARAM(x) (void)(x)


///////////////////////////////////////////////////////////////////////
//  Code Safety

#define ST_PREVENT_INSTNATIATION(a) private: a();
#define ST_PREVENT_COPY(a) private: a(const a&); a& operator=(const a&);


////////////////////////////////////////////////////////////
//  Compile-time assertion

#define ST_ASSERT_ON_COMPILE(expr) extern char AssertOnCompile[(expr) ? 1 : -1]


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree8
{
	///////////////////////////////////////////////////////////////////////
	//  Types

	typedef bool            st_bool;
	typedef char            st_int8;
	typedef char            st_char;
	typedef short           st_int16;
	typedef int             st_int32;
	typedef long long       st_int64;
	typedef unsigned char   st_uint8;
	typedef unsigned char   st_byte;
	typedef unsigned char   st_uchar;
	typedef unsigned short  st_uint16;
	typedef unsigned int    st_uint32;
	typedef float           st_float32;
	typedef double          st_float64;


	///////////////////////////////////////////////////////////////////////
	//  class st_float16 (half-float)

	class ST_DLL_LINK st_float16
	{
	public:
					st_float16( );
					st_float16(st_float32 fSinglePrecision);
					st_float16(const st_float16& hfCopy);

					operator st_float32(void) const;

	private:
		st_uint16   m_uiValue;
	};


	///////////////////////////////////////////////////////////////////////
	//  Compile time checks on types

	ST_ASSERT_ON_COMPILE(sizeof(st_int8) == 1);
	ST_ASSERT_ON_COMPILE(sizeof(st_int16) == 2);
	ST_ASSERT_ON_COMPILE(sizeof(st_int32) == 4);
	ST_ASSERT_ON_COMPILE(sizeof(st_int64) == 8);
	ST_ASSERT_ON_COMPILE(sizeof(st_uint8) == 1);
	ST_ASSERT_ON_COMPILE(sizeof(st_uint16) == 2);
	ST_ASSERT_ON_COMPILE(sizeof(st_uint32) == 4);
	ST_ASSERT_ON_COMPILE(sizeof(st_float16) == 2);
	ST_ASSERT_ON_COMPILE(sizeof(st_float32) == 4);
	ST_ASSERT_ON_COMPILE(sizeof(st_float64) == 8);


	///////////////////////////////////////////////////////////////////////
	//  Special SpeedTree assertion macro

	#ifndef NDEBUG
		#define st_assert(condition, explanation) assert((condition) && (explanation))
	#else
		#define st_assert(condition, explanation)
	#endif


	// chris says: I'm getting:
	//	error: C2244: 'glm::min' : unable to match function definition to an existing declaration
	//
	// for both st_min() and st_max() because I have them defined elsewhere; I don't see where the new
	// SpeedTreeFormat stuff is using these, so temporarily commenting them out

#if 0
	///////////////////////////////////////////////////////////////////////
	//  Function equivalents of __min and __max macros

	template <class T> inline T st_min(const T& a, const T& b)
	{
		return (a < b) ? a : b;
	}

	template <class T> inline T st_max(const T& a, const T& b)
	{
		return (a > b) ? a : b;
	}
#endif


	///////////////////////////////////////////////////////////////////////
	//  SDK-wide constants

	#if defined(_XBOX)
		static const st_char c_chFolderSeparator = '\\';
		static const st_char* c_szFolderSeparator = "\\";
	#else
		// strange way to specify this because GCC issues warnings about these being unused in Core
		static const st_char* c_szFolderSeparator = "/";
		static const st_char c_chFolderSeparator = c_szFolderSeparator[0];
	#endif


	// include inline functions
	#include "Platform_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

