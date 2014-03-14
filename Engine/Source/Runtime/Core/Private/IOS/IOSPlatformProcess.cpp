// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSPlatformProcess.mm: iOS implementations of Process functions
=============================================================================*/

#include "CorePrivate.h"
#include "ApplePlatformRunnableThread.h"
#include <mach-o/dyld.h>

const TCHAR* FIOSPlatformProcess::ComputerName()
{
	static TCHAR Result[256]=TEXT("");

	if( !Result[0] )
	{
		ANSICHAR AnsiResult[ARRAY_COUNT(Result)];
		gethostname(AnsiResult, ARRAY_COUNT(Result));
		FCString::Strcpy(Result, ANSI_TO_TCHAR(AnsiResult));
	}
	return Result;
}

const TCHAR* FIOSPlatformProcess::BaseDir()
{
	return TEXT("");
}

FRunnableThread* FIOSPlatformProcess::CreateRunnableThread()
{
	return new FRunnableThreadApple();
}

void FIOSPlatformProcess::LaunchURL( const TCHAR* URL, const TCHAR* Parms, FString* Error )
{
	UE_LOG(LogIOS, Log,  TEXT("LaunchURL %s %s"), URL, Parms?Parms:TEXT("") );
	CFStringRef CFUrl = FPlatformString::TCHARToCFString( URL );
	CFUrl = CFURLCreateStringByAddingPercentEscapes(NULL, CFUrl, NULL, NULL, kCFStringEncodingUTF8);
	[[UIApplication sharedApplication] openURL: [NSURL URLWithString:( NSString *)CFUrl]];
	CFRelease( CFUrl );

	if (Error != nullptr)
	{
		*Error = TEXT("");
	}
}

void FIOSPlatformProcess::SetRealTimeMode()
{
    mach_timebase_info_data_t TimeBaseInfo;
    mach_timebase_info( &TimeBaseInfo );
    double MsToAbs = ((double)TimeBaseInfo.denom / (double)TimeBaseInfo.numer) * 1000000.0;
    uint32 NormalProcessingTimeMs = 20;
    uint32 ConstraintProcessingTimeMs = 60;
    
    thread_time_constraint_policy_data_t Policy;
    Policy.period      = 0;
    Policy.computation = (uint32_t)(NormalProcessingTimeMs * MsToAbs);
    Policy.constraint  = (uint32_t)(ConstraintProcessingTimeMs * MsToAbs);
    Policy.preemptible = true;
    
    thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&Policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
}