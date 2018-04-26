// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2017 NVIDIA Corporation. All rights reserved.


/*===========================================================================
GFSDK_Common.h

GFSDK_Common defines common macros, types and structs used generally by
all NVIDIA TECH Libraries.  You won't find anything particularly interesting
here.

For any issues with this particular header please contact:

devsupport@nvidia.com

For any issues with the specific FX library you are using, see the header
file for that library for contact information.

===========================================================================*/

#include <stddef.h>

#pragma pack(push,8) // Make sure we have consistent structure packings

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
AUTO CONFIG
===========================================================================*/
#if !defined(__GFSDK_COMMON_AUTOCONFIG)
#   define __GFSDK_COMMON_AUTOCONFIG

#   if defined(__GNUC__)
#       define __GFSDK_CC_GNU__
#   else
#       define __GFSDK_CC_MSVC__
#   endif

#endif

/*===========================================================================
FORWARD DECLARATIONS 
===========================================================================*/
#if !defined(__GFSDK_COMMON_FORWARDDECLS)
#   define __GFSDK_COMMON_FORWARDDECLS

#   if defined(__GFSDK_GL__)
    // $ TODO Add common fwd decls for OGL
#   endif
    /*-------------------------------------------------------------------------*/
#   if defined(__GFSDK_DX11__)
    // $ TODO Add common fwd decls for D3D11
#   endif

#endif

/*===========================================================================
TYPES
===========================================================================*/
#if !defined(__GFSDK_COMMON_TYPES)
#   define __GFSDK_COMMON_TYPES

  typedef unsigned char gfsdk_U8;
  typedef unsigned short gfsdk_U16;
  typedef signed int gfsdk_S32;
  typedef signed long long gfsdk_S64;
  typedef unsigned int gfsdk_U32;
  typedef unsigned long long gfsdk_U64;
  typedef float gfsdk_F32;

  typedef struct {
    gfsdk_F32 x;
    gfsdk_F32 y;
  } gfsdk_float2;

  typedef struct {
    gfsdk_F32 x;
    gfsdk_F32 y;
    gfsdk_F32 z;
  } gfsdk_float3;

  typedef struct {
    gfsdk_F32 x;
    gfsdk_F32 y;
    gfsdk_F32 z;
    gfsdk_F32 w;
  } gfsdk_float4;

  // implicit row major
  typedef struct {
    gfsdk_F32 _11, _12, _13, _14;
    gfsdk_F32 _21, _22, _23, _24;
    gfsdk_F32 _31, _32, _33, _34;
    gfsdk_F32 _41, _42, _43, _44;
  } gfsdk_float4x4;

#   if defined(__cplusplus)
	typedef bool                 gfsdk_bool;
#   endif
	typedef char                 gfsdk_char;
	typedef const gfsdk_char*    gfsdk_cstr;
	typedef double               gfsdk_F64;

#endif // !defined(__GFSDK_COMMON_TYPES)

/*===========================================================================
MACROS
===========================================================================*/
#if !defined(__GFSDK_COMMON_MACROS)
#   define __GFSDK_COMMON_MACROS

#   if defined(__GFSDK_CC_GNU__)
#       define __GFSDK_ALIGN__(bytes) __attribute__((aligned (bytes)))
#       define __GFSDK_EXPECT__(exp,tf) __builtin_expect(exp, tf)
#       define __GFSDK_INLINE__ __attribute__((always_inline))
#       define __GFSDK_NOLINE__ __attribute__((noinline))
#       define __GFSDK_RESTRICT__ __restrict
#       define __GFSDK_CDECL__
#       define __GFSDK_IMPORT__
#       define __GFSDK_EXPORT__
#       define __GFSDK_STDCALL__
#		define __GFSDK_THREAD__ __thread
#   elif defined(__GFSDK_CC_MSVC__)
#       define __GFSDK_ALIGN__(bytes) __declspec(align(bytes))
#       define __GFSDK_EXPECT__(exp, tf) (exp)
#       define __GFSDK_INLINE__ __forceinline
#       define __GFSDK_NOINLINE__
#       define __GFSDK_RESTRICT__ __restrict
#       define __GFSDK_CDECL__ __cdecl
#       define __GFSDK_IMPORT__ __declspec(dllimport)
#       define __GFSDK_EXPORT__ __declspec(dllexport)
#       define __GFSDK_STDCALL__ __stdcall
#		define __GFSDK_THREAD__ __declspec(thread)
#   endif

#endif // !defined(__GFSDK_COMMON_MACROS)

/*===========================================================================
RETURN CODES
===========================================================================*/
#if !defined(__GFSDK_COMMON_RETURNCODES)
#   define __GFSDK_COMMON_RETURNCODES

#   define GFSDK_RETURN_OK   0
#   define GFSDK_RETURN_FAIL 1
#   define GFSDK_RETURN_EMULATION 2

#endif // !defined(__GFSDK_COMMON_RETURNCODES)

/*===========================================================================
CUSTOM HEAP
===========================================================================*/
#if !defined(__GFSDK_COMMON_CUSTOMHEAP)
#   define __GFSDK_COMMON_CUSTOMHEAP

  typedef struct {
    void* (*new_)(size_t);
    void (*delete_)(void*);
  } gfsdk_new_delete_t;

#endif // !defined(__GFSDK_COMMON_CUSTOMHEAP)

/*===========================================================================
HANDLE DECLARATION
===========================================================================*/
#if !defined(__GFSDK_COMMON_DECLHANDLE)
#   define __GFSDK_COMMON_DECLHANDLE
#   define GFSDK_DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif // !defined(__GFSDK_COMMON_DECLHANDLE)

#if defined(__cplusplus)
}; //extern "C" {
#endif

#pragma pack(pop)
