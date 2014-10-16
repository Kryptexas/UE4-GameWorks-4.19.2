// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	HTML5StackWalk.h: HTML5 platform stack walk functions
==============================================================================================*/

#pragma once

/**
* Android platform stack walking
*/
struct CORE_API FHTML5PlatformStackWalk : public FGenericPlatformStackWalk
{
	typedef FGenericPlatformStackWalk Parent;

	static void ProgramCounterToSymbolInfo(uint64 ProgramCounter,FProgramCounterSymbolInfo& out_SymbolInfo);
	static void CaptureStackBackTrace(uint64* BackTrace,uint32 MaxDepth,void* Context = NULL);
};

typedef FHTML5PlatformStackWalk FPlatformStackWalk;
