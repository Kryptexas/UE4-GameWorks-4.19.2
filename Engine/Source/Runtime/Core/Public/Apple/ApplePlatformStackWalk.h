// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	ApplePlatformStackWalk.h: Apple platform stack walk functions
==============================================================================================*/

#pragma once

#undef PLATFORM_SUPPORTS_STACK_SYMBOLS
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1

/**
 * Symbol information associated with a program counter.
 */
struct FProgramCounterSymbolInfo
{
	/** Module name.					*/
	ANSICHAR	ModuleName[1024];
	/** Function name.					*/
	ANSICHAR	FunctionName[1024];
	/** Filename.						*/
	ANSICHAR	Filename[1024];
	/** Line number in file.			*/
	int32			LineNumber;
	/** Symbol displacement of address.	*/
	int32			SymbolDisplacement;
};



/**
 * Apple platform implementation of the misc OS functions
 */
struct CORE_API FApplePlatformStackWalk : public FGenericPlatformStackWalk
{
	static bool ProgramCounterToHumanReadableString( int32 CurrentCallDepth, uint64 ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, EVerbosityFlags VerbosityFlags = VF_DISPLAY_ALL, FGenericCrashContext* Context = NULL );
	static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo&  OutProgramCounterSymbolInfo);
	static void CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context = NULL );
	static int32 GetProcessModuleCount();
	static int32 GetProcessModuleSignatures(FStackWalkModuleInfo *ModuleSignatures, const int32 ModuleSignaturesSize);
};

typedef FApplePlatformStackWalk FPlatformStackWalk;
