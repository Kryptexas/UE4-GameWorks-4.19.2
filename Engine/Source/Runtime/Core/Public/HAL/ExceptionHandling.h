// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ExceptionHandling.h: Exception handling for functions that want to create crash dumps.
=============================================================================*/

#pragma once

#include "Core.h"

/** Whether we should generate crash reports even if the debugger is attached. */
extern CORE_API bool GAlwaysReportCrash;

/** Whether to use ClientReportClient rather than AutoReporter. */
extern CORE_API bool GUseCrashReportClient;

extern CORE_API TCHAR MiniDumpFilenameW[1024];

#if PLATFORM_WINDOWS
extern CORE_API int32 NullReportCrash( LPEXCEPTION_POINTERS ExceptionInfo );
extern CORE_API int32 ReportCrash( LPEXCEPTION_POINTERS ExceptionInfo );
extern CORE_API void NewReportEnsure( const TCHAR* ErrorMessage );
#elif PLATFORM_MAC
extern CORE_API int32 ReportCrash( ucontext_t *Context, int32 Signal, struct __siginfo* Info );
extern CORE_API void NewReportEnsure( const TCHAR* ErrorMessage );
#elif PLATFORM_LINUX
extern CORE_API int32 ReportCrash( ucontext_t *Context, int32 Signal, siginfo_t* Info );
extern CORE_API void NewReportEnsure( const TCHAR* ErrorMessage );
#endif
