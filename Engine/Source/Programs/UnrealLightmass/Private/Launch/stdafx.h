// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Scene.cpp: PCH master include file.
=============================================================================*/

#pragma once

// basic stuff needed by everybody
#pragma warning( disable: 4799 )		// function '...' has no EMMS instruction)

#if PLATFORM_MAC
inline unsigned long long __rdtsc()
{
	unsigned long long Low, High;
	__asm__ __volatile__ ("rdtsc" : "=a" (Low), "=d" (High));
	return (High << 32) | Low;
}
#endif

#include "CoreUObject.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLightmass, Log, All);

#include "ChunkedArray.h"
#include "Lighting.h"
#include "3DVisualizer.h"
