// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExceptionHandling.cpp: Exception handling for functions that want to create crash dumps.
=============================================================================*/

#include "CorePrivate.h"
#include "ExceptionHandling.h"

/** Whether we should generate crash reports even if the debugger is attached. */
CORE_API bool GAlwaysReportCrash = false;

/** Whether to use ClientReportClient rather than the old AutoReporter. */
CORE_API bool GUseCrashReportClient = true;

CORE_API TCHAR MiniDumpFilenameW[1024] = TEXT("");
