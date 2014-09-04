// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformSymbolication.h: Apple platform implementation of symbolication
==============================================================================================*/

#pragma once

#include "ApplePlatformStackWalk.h"

/**
 * Apple platform implementation of symbolication - not async. handler safe, so don't call during crash reporting!
 */
struct CORE_API FApplePlatformSymbolication
{
	static bool SymbolInfoForAddress(uint64 ProgramCounter, FProgramCounterSymbolInfo& Info);
	static bool SymbolInfoForFunctionFromModule(ANSICHAR const* MangledName, ANSICHAR const* ModuleName, FProgramCounterSymbolInfo& Info);
	static bool SymbolInfoForStrippedSymbol(uint64 ProgramCounter, ANSICHAR const* ModulePath, ANSICHAR const* ModuleUUID, FProgramCounterSymbolInfo& Info);
};
