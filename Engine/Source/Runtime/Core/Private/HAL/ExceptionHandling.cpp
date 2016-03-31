// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExceptionHandling.cpp: Exception handling for functions that want to create crash dumps.
=============================================================================*/

#include "CorePrivatePCH.h"
#include "ExceptionHandling.h"

/** Whether we should generate crash reports even if the debugger is attached. */
CORE_API bool GAlwaysReportCrash = false;

/** Whether to use ClientReportClient rather than the old AutoReporter. */
CORE_API bool GUseCrashReportClient = true;

/** Whether we should ignore the attached debugger. */
CORE_API bool GIgnoreDebugger = false;

CORE_API TCHAR MiniDumpFilenameW[1024] = TEXT("");


volatile int32 GImageIntegrityCompromised = 0;
void CheckImageIntegrity()
{
	FPlatformMisc::MemoryBarrier();
	UE_CLOG(GImageIntegrityCompromised > 0, LogCore, Fatal, TEXT("Image integrity compromised (%d)"), GImageIntegrityCompromised);
}

void CheckImageIntegrityAtRuntime()
{
	FPlatformMisc::MemoryBarrier();
	UE_CLOG(GImageIntegrityCompromised > 0, LogCore, Fatal, TEXT("Image integrity compromised at runtime (%d)"), GImageIntegrityCompromised);
}

void SetImageIntegrtiryStatus(int32 Status)
{
	GImageIntegrityCompromised = Status;
}

int32 GetImageIntegrityStatus()
{
	return GImageIntegrityCompromised;
}