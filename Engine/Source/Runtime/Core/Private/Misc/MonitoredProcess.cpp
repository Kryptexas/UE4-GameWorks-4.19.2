// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MonitoredProcess.cpp: Implements the FMonitoredProcess class
==============================================================================================*/

#include "CorePrivate.h"


/* FMonitoredProcess structors
 *****************************************************************************/

FMonitoredProcess::FMonitoredProcess( const FString& InURL, const FString& InParams, bool InHidden )
	: Canceling(false)
	, EndTime(0)
	, Hidden(InHidden)
	, KillTree(false)
	, Params(InParams)
	, ReadPipe(NULL)
	, StartTime(0)
	, Thread(NULL)
	, URL(InURL)
	, WritePipe(NULL)
{ }


FMonitoredProcess::~FMonitoredProcess( )
{
	if (IsRunning())
	{
		Cancel(true);
		Thread->WaitForCompletion();
		delete Thread;
	}
}


/* FMonitoredProcess interface
 *****************************************************************************/

FTimespan FMonitoredProcess::GetDuration( ) const
{
	if (IsRunning())
	{
		return (FDateTime::UtcNow() - StartTime);
	}

	return (EndTime - StartTime);
}


bool FMonitoredProcess::Launch( )
{
	if (IsRunning())
	{
		return false;
	}

	if (!FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
	{
		return false;
	}

	ProcessHandle = FPlatformProcess::CreateProc(*URL, *Params, false, Hidden, Hidden, NULL, 0, *FPaths::RootDir(), WritePipe);

	if (!ProcessHandle.IsValid())
	{
		return false;
	}

	Thread = FRunnableThread::Create(this, TEXT("FMonitoredProcess"), false, false, 128 * 1024, TPri_AboveNormal);

	return true;
}


/* FMonitoredProcess implementation
 *****************************************************************************/

void FMonitoredProcess::ProcessOutput( const FString& Output )
{
	TArray<FString> LogLines;

	Output.ParseIntoArray(&LogLines, TEXT("\n"), false);

	for (int32 LogIndex = 0; LogIndex < LogLines.Num(); ++LogIndex)
	{
		OutputDelegate.ExecuteIfBound(LogLines[LogIndex]);
	}
}


/* FRunnable interface
 *****************************************************************************/

uint32 FMonitoredProcess::Run( )
{
	// monitor the process
	StartTime = FDateTime::UtcNow();
	{
		do
		{
			FPlatformProcess::Sleep(0.0);

			ProcessOutput(FPlatformProcess::ReadPipe(ReadPipe));

			if (Canceling)
			{
				FPlatformProcess::TerminateProc(ProcessHandle, KillTree);
				CanceledDelegate.ExecuteIfBound();
				Thread = NULL;

				return 0;
			}
		}
		while (FPlatformProcess::IsProcRunning(ProcessHandle));
	}
	EndTime = FDateTime::UtcNow();

	// close output pipes
	FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
	ReadPipe = WritePipe = NULL;

	// get completion status
	int32 ReturnCode;
	
	if (!FPlatformProcess::GetProcReturnCode(ProcessHandle, &ReturnCode))
	{
		ReturnCode = -1;
	}

	CompletedDelegate.ExecuteIfBound(ReturnCode);
	Thread = NULL;

	return 0;
}
