// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define CRASH_REPORT_UNATTENDED_ONLY			PLATFORM_LINUX

// Pre-compiled header includes
#include "Core.h"
#if !CRASH_REPORT_UNATTENDED_ONLY
	#include "Slate.h"
	#include "StandaloneRenderer.h"
#endif // CRASH_REPORT_UNATTENDED_ONLY

DECLARE_LOG_CATEGORY_EXTERN(CrashReportClientLog, Log, All)

/** IP of server to send report to */
extern const TCHAR* GServerIP;

/** Filename to use when saving diagnostics report (if generated locally) */
extern const TCHAR* GDiagnosticsFilename;

/**
 * Run the crash report client app
 */
void RunCrashReportClient(const TCHAR* Commandline);
