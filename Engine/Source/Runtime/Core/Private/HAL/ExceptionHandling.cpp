// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExceptionHandling.cpp: Exception handling for functions that want to create crash dumps.
=============================================================================*/

#include "HAL/ExceptionHandling.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"
#include "CoreGlobals.h"
#include "Misc/CoreDelegates.h"
#include "Misc/OutputDeviceRedirector.h"

#ifndef UE_ASSERT_ON_BUILD_INTEGRITY_COMPROMISED
#define UE_ASSERT_ON_BUILD_INTEGRITY_COMPROMISED 0
#endif

/** Whether we should generate crash reports even if the debugger is attached. */
CORE_API bool GAlwaysReportCrash = false;

/** Whether to use ClientReportClient rather than the old AutoReporter. */
CORE_API bool GUseCrashReportClient = true;

/** Whether we should ignore the attached debugger. */
CORE_API bool GIgnoreDebugger = false;

CORE_API TCHAR MiniDumpFilenameW[1024] = TEXT("");


volatile int32 GCrashType = 0;
bool GEnsureShowsCRC = false;

void CheckImageIntegrity()
{
	FPlatformMisc::MemoryBarrier();
}

void CheckImageIntegrityAtRuntime()
{
	FPlatformMisc::MemoryBarrier();
}

void SetCrashType(ECrashType InCrashType)
{
	GCrashType = (int32)InCrashType;
}

int32 GetCrashType()
{
	return GCrashType;
}

void ReportInteractiveEnsure(const TCHAR* InMessage)
{
	GEnsureShowsCRC = true;

#if PLATFORM_DESKTOP
	GLog->PanicFlushThreadedLogs();
	// GErrorMessage here is very unfortunate but it's used internally by the crash context code.
	FCString::Strcpy(GErrorMessage, ARRAY_COUNT(GErrorMessage), InMessage);
	// Skip macros and FDebug, we always want this to fire
	NewReportEnsure(InMessage);
	GErrorMessage[0] = '\0';
#endif

	GEnsureShowsCRC = false;
}

bool IsInteractiveEnsureMode()
{
	return GEnsureShowsCRC;
}