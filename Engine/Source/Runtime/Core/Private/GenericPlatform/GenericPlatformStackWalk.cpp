// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformStackWalk.cpp: Generic, do nothing stack walk implementation
=============================================================================*/

#include "CorePrivate.h"

bool FGenericPlatformStackWalk::ProgramCounterToHumanReadableString( int32 CurrentCallDepth, uint64 ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, EVerbosityFlags VerbosityFlags, FGenericCrashContext* Context )
{
	if (HumanReadableString && HumanReadableStringSize > 0)
	{
		ANSICHAR TempArray[MAX_SPRINTF];
		if (PLATFORM_64BITS)
		{
			FCStringAnsi::Sprintf(TempArray, "[Callstack] %p", (void*) ProgramCounter);
		}
		else
		{
			FCStringAnsi::Sprintf(TempArray, "[Callstack] 0x%.8x", (uint32) ProgramCounter);
		}
		FCStringAnsi::Strcat( HumanReadableString, HumanReadableStringSize, TempArray );

		if( VerbosityFlags & VF_DISPLAY_FILENAME )
		{
			//Append the filename to the string here
		}
		return true;
	}
	return true;
}

void FGenericPlatformStackWalk::CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context )
{
	// nothing we can do generically
	for( uint32 i=0; i<MaxDepth; i++ )
	{
		BackTrace[i] = 0;
	}
}

void FGenericPlatformStackWalk::StackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, int32 IgnoreCount, void* Context )
{
	// Temporary memory holding the stack trace.
	static const int MAX_DEPTH = 100;
	uint64 StackTrace[MAX_DEPTH];
	memset( StackTrace, 0, sizeof(StackTrace) );

	// Capture stack backtrace.
	FPlatformStackWalk::CaptureStackBackTrace( StackTrace, MAX_DEPTH, Context );

	// Skip the first two entries as they are inside the stack walking code.
	int32 CurrentDepth = IgnoreCount;
	// Allow the first entry to be NULL as the crash could have been caused by a call to a NULL function pointer,
	// which would mean the top of the callstack is NULL.
	while( StackTrace[CurrentDepth] || ( CurrentDepth == IgnoreCount ) )
	{
		FPlatformStackWalk::ProgramCounterToHumanReadableString( CurrentDepth, StackTrace[CurrentDepth], HumanReadableString, HumanReadableStringSize, 
			VF_DISPLAY_ALL, reinterpret_cast< FGenericCrashContext* >( Context ) );
		FCStringAnsi::Strcat( HumanReadableString, HumanReadableStringSize, LINE_TERMINATOR_ANSI );
		CurrentDepth++;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CrashForUAT")) && FParse::Param(FCommandLine::Get(), TEXT("stdout")))
	{
		FPlatformMisc::LowLevelOutputDebugString(ANSI_TO_TCHAR(HumanReadableString));
		wprintf(TEXT("\nbegin: stack for UAT"));
		wprintf(TEXT("\n%s"), ANSI_TO_TCHAR(HumanReadableString));
		wprintf(TEXT("\nend: stack for UAT"));
		fflush(stdout);
	}
}

