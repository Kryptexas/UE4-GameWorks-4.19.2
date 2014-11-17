// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "DynamicRHI.h"

#if WANTS_DRAW_MESH_EVENTS && PLATFORM_SUPPORTS_DRAW_MESH_EVENTS

bool FDrawEvent::IsInRenderingThread_Internal()
{
	return IsInRenderingThread();
}

FDrawEvent::~FDrawEvent()
{
	if (bDrawEventHasBeenEmitted)
	{
		GDynamicRHI->PopEvent();
	}
}

void FDrawEvent::Start(const FColor& Color, const TCHAR* Fmt, ...)
{
	check(IsInRenderingThread_Internal());
	va_list ptr;
	va_start(ptr, Fmt);
	TCHAR TempStr[256];
	// Build the string in the temp buffer
	FCString::GetVarArgs(TempStr, ARRAY_COUNT(TempStr), ARRAY_COUNT(TempStr) - 1, Fmt, ptr);
	GDynamicRHI->PushEvent(TempStr);
	bDrawEventHasBeenEmitted = true;
}

#endif // WANTS_DRAW_MESH_EVENTS && PLATFORM_SUPPORTS_DRAW_MESH_EVENTS