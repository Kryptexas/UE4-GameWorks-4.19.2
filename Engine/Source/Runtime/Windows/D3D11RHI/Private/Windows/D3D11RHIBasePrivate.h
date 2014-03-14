// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11BaseRHIPrivate.h: Private D3D RHI definitions for Windows.
=============================================================================*/

#pragma once

// Assume D3DX is available
#ifndef WITH_D3DX_LIBS
#define WITH_D3DX_LIBS	1
#endif

// Disable macro redefinition warning for compatibility with Windows SDK 8+
#pragma warning(push)
#if _MSC_VER >= 1700
#pragma warning(disable : 4005)	// macro redefinition
#endif

// D3D headers.
#if PLATFORM_64BITS
#pragma pack(push,16)
#else
#pragma pack(push,8)
#endif
#define D3D_OVERLOADS 1
#include "AllowWindowsPlatformTypes.h"

#include <D3D11.h>
#if WITH_D3DX_LIBS
#include <D3DX11.h>
#endif

#include "HideWindowsPlatformTypes.h"

#undef DrawText

#pragma pack(pop)
#pragma warning(pop)

#if WITH_D3DX_LIBS
	/**
	 * Make sure we are compiling against the DXSDK we are expecting to,
	 * Which is the June 2010 DX SDK.
	 */
	const int32 REQUIRED_D3DX11_SDK_VERSION = 43;
	checkAtCompileTime(D3DX11_SDK_VERSION == REQUIRED_D3DX11_SDK_VERSION, D3DX11_SDK_VERSION_DoesNotMatchRequiredVersion);
#endif	//WITH_D3DX_LIBS

typedef ID3D11DeviceContext FD3D11DeviceContext;
typedef ID3D11Device FD3D11Device;
