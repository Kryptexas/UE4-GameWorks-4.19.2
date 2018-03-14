// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AndroidErrorOutputDevice.h"

#include "CoreGlobals.h"
#include "OutputDeviceHelper.h"
#include "HAL/PlatformMisc.h"
#include "Misc/OutputDeviceRedirector.h"
#include "CoreGlobals.h"

FAndroidErrorOutputDevice::FAndroidErrorOutputDevice()
{
}

void FAndroidErrorOutputDevice::Serialize( const TCHAR* Msg, ELogVerbosity::Type Verbosity, const class FName& Category )
{
	FPlatformMisc::LowLevelOutputDebugString(*FOutputDeviceHelper::FormatLogLine(Verbosity, Category, Msg, GPrintLogTimes));
	if (GIsGuarded)
	{
		UE_DEBUG_BREAK();
	}
	else
	{
		HandleError();
		FPlatformMisc::RequestExit(true);
	}
}

void FAndroidErrorOutputDevice::HandleError()
{
	static int32 CallCount = 0;
	int32 NewCallCount = FPlatformAtomics::InterlockedIncrement(&CallCount);

	if (NewCallCount != 1)
	{
		UE_LOG(LogAndroid, Error, TEXT("HandleError re-entered."));
		return;
	}
	
	GIsGuarded = 0;
	GIsRunning = 0;
	GIsCriticalError = 1;
	GLogConsole = NULL;
	GErrorHist[ARRAY_COUNT(GErrorHist) - 1] = 0;

	// Dump the error and flush the log.
#if !NO_LOGGING
	FDebug::LogFormattedMessageWithCallstack(LogAndroid.GetCategoryName(), __FILE__, __LINE__, TEXT("=== Critical error: ==="), GErrorHist, ELogVerbosity::Error);
#endif
	GLog->PanicFlushThreadedLogs();
}
