// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"
#if !CRASH_REPORT_UNATTENDED_ONLY
	#include "SCrashReportClient.h"
	#include "CrashReportClient.h"
	#include "CrashReportClientStyle.h"
#endif // !CRASH_REPORT_UNATTENDED_ONLY
#include "CrashReportClientUnattended.h"
#include "RequiredProgramMainCPPInclude.h"
#include "AsyncWork.h"

#include "MainLoopTiming.h"

/** Default main window size */
const FVector2D InitialWindowDimensions(640, 560);

/** Average tick rate the app aims for */
const float IdealTickRate = 30.f;

/** Set this to true in the code to open the widget reflector to debug the UI */
const bool RunWidgetReflector = false;

IMPLEMENT_APPLICATION(CrashReportClient, "CrashReportClient");

DEFINE_LOG_CATEGORY(CrashReportClientLog);

/**
 * Upload the crash report with no user interaction
 */
void RunCrashReportClientUnattended(FMainLoopTiming& MainLoop, const FString& ReportDirectory)
{
	FCrashReportClientUnattended CrashReportClient(ReportDirectory);

	// loop until the app is ready to quit
	while (!GIsRequestingExit)
	{
		MainLoop.Tick();
	}
}

/**
 * Look for the report to upload, either in the command line or in the platform's report queue
 */
FString FindCrashReport(const TCHAR* CommandLine)
{
	const TCHAR* CommandLineAfterExe = FCommandLine::RemoveExeName(CommandLine);

	// Use the first argument if present and it's not a flag
	if (*CommandLineAfterExe && *CommandLineAfterExe != '-')
	{
		return FParse::Token(CommandLineAfterExe, true /* handle escaped quotes */);
	}
	else
	{
		return FPlatformErrorReport::FindMostRecentErrorReport();
	}
}

void RunCrashReportClient(const TCHAR* CommandLine)
{
	// Set up the main loop
	GEngineLoop.PreInit(CommandLine);

	bool bUnattended = 
#if CRASH_REPORT_UNATTENDED_ONLY
		true;
#else
		FApp::IsUnattended();
#endif // CRASH_REPORT_UNATTENDED_ONLY

	// Set up the main ticker
	FMainLoopTiming MainLoop(IdealTickRate, bUnattended ? EMainLoopOptions::CoreTickerOnly : EMainLoopOptions::UsingSlate);

	// Find the report to upload in the command line arguments
	auto CommandLineAfterExe = FCommandLine::RemoveExeName(CommandLine);
	FString ReportDirectory = FindCrashReport(CommandLine);

	if (ReportDirectory.IsEmpty())
	{
		UE_LOG(CrashReportClientLog, Warning, TEXT("No error report found"));
	}

	FParse::Token(CommandLineAfterExe, true /* handle escaped quotes */);

	if (bUnattended)
	{
		RunCrashReportClientUnattended(MainLoop, ReportDirectory);
	}
	else
	{
#if !CRASH_REPORT_UNATTENDED_ONLY
		// crank up a normal Slate application using the platform's standalone renderer
		FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer());

		// Prepare the custom Slate styles
		FCrashReportClientStyle::Initialize();

		// Create the main implementation object
		TSharedRef<FCrashReportClient> CrashReportClient = MakeShareable(new FCrashReportClient(ReportDirectory));

		// open up the app window	
		TSharedRef<SCrashReportClient> ClientControl = SNew(SCrashReportClient, CrashReportClient);

		auto Window = FSlateApplication::Get().AddWindow(
			SNew(SWindow)
			.Title(NSLOCTEXT("CrashReportClient", "CrashReportClientAppName", "Unreal Engine 4 Crash Reporter"))
			.ClientSize(InitialWindowDimensions)
			[
				ClientControl
			]);

		Window->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateSP(CrashReportClient, &FCrashReportClient::RequestCloseWindow));

		// Setting focus seems to have to happen after the Window has been added
		ClientControl->SetDefaultFocus();

		// Debugging code
		if (RunWidgetReflector)
		{
			FSlateApplication::Get().AddWindow(
				SNew(SWindow)
				.ClientSize(FVector2D(800, 600))
				[
					FSlateApplication::Get().GetWidgetReflector()
				]);
		}

		// loop until the app is ready to quit
		while (!GIsRequestingExit)
		{
			MainLoop.Tick();

			if (CrashReportClient->ShouldWindowBeHidden())
			{
				Window->HideWindow();
			}
		}

		// Clean up the custom styles
		FCrashReportClientStyle::Shutdown();

		// Close down the Slate application
		FSlateApplication::Shutdown();
#endif // !CRASH_REPORT_UNATTENDED_ONLY
	}

	GEngineLoop.AppExit();
}
