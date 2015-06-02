// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"
#include "CrashReportClientUnattended.h"

FCrashReportClientUnattended::FCrashReportClientUnattended(const FPlatformErrorReport& ErrorReport)
	: Uploader( FCrashReportClientConfig::Get().GetReceiverAddress() )
{
	// Process the report synchronously
	ErrorReport.DiagnoseReport();
	auto DiagnosticsFilePath = ErrorReport.GetReportDirectory() / FCrashReportClientConfig::Get().GetDiagnosticsFilename();
	Uploader.LocalDiagnosisComplete( FPaths::FileExists( DiagnosticsFilePath ) ? DiagnosticsFilePath : TEXT( "" ) );
	Uploader.BeginUpload( ErrorReport );
	StartTicker();
}

bool FCrashReportClientUnattended::Tick(float UnusedDeltaTime)
{
	if (Uploader.IsFinished())
	{
		FPlatformMisc::RequestExit(false /* don't force */);
		// No more ticks, thank you
		return false;
	}

	return true;	
}

void FCrashReportClientUnattended::StartTicker()
{
	FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FCrashReportClientUnattended::Tick), 1.f);
}
