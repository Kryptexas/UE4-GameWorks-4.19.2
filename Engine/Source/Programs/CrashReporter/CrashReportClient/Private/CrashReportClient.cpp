// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "CrashReportClient.h"
#include "UniquePtr.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "CrashReportClient"

#if	DO_LOCAL_TESTING
	const TCHAR* GServerIP = TEXT( "http://localhost:57005" );
#else
	const TCHAR* GServerIP = TEXT( "http://crashreporter.epicgames.com:57005" );
#endif // DO_LOCAL_TESTING

// Must match filename specified in RunMinidumpDiagnostics
const TCHAR* GDiagnosticsFilename = TEXT("Diagnostics.txt");
FString GCrashUserId;

#if !CRASH_REPORT_UNATTENDED_ONLY

FCrashReportClient::FCrashReportClient(const FPlatformErrorReport& InErrorReport, const FString& AppName)
	: AppState(EApplicationState::Ready)
	, SubmittedCountdown(-1)
	, bDiagnosticFileSent(false)
	, ErrorReport( InErrorReport )
	, Uploader(GServerIP)
	, CrashedAppName(AppName)
	, CancelButtonText(LOCTEXT("Cancel", "Don't Send"))
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const FString MachineId = DesktopPlatform->GetMachineId().ToString( EGuidFormats::Digits );

	// The Epic ID can be looked up from this ID.
	const FString EpicAccountId = DesktopPlatform->GetEpicAccountId();

	// Remove periods from internal user names to match AutoReporter user names
	// The name prefix is read by CrashRepository.AddNewCrash in the website code
	const FString UserNameNoDot = FString( FPlatformProcess::UserName() ).Replace( TEXT( "." ), TEXT( "" ) );

	// Set global user name ID: will be added to the report
	if (FRocketSupport::IsRocket())
	{
		GCrashUserId = FString::Printf( TEXT( "!MachineId:%s!EpicAccountId:%s" ), *MachineId, *EpicAccountId );
	}
	else
	{
		GCrashUserId = FString::Printf( TEXT( "!MachineId:%s!Name:%s" ), *MachineId, *UserNameNoDot );
	}

	if (!ErrorReport.TryReadDiagnosticsFile(DiagnosticText) && !FParse::Param(FCommandLine::Get(), TEXT("no-local-diagnosis")))
	{
		FDiagnoseReportWorker& Worker = DiagnoseReportTask.GetTask( &DiagnosticText, MachineId, EpicAccountId, UserNameNoDot, &ErrorReport );
		DiagnoseReportTask.StartBackgroundTask();
	}
	else if( !DiagnosticText.IsEmpty() )
	{
		DiagnosticText = FCrashReportClient::FormatDiagnosticText( DiagnosticText, MachineId, EpicAccountId, UserNameNoDot );
	}
}

FReply FCrashReportClient::Submit()
{
	if (AppState == EApplicationState::Ready)
	{
		StoreCommentAndUpload();
	}

	return FReply::Handled();
}

FReply FCrashReportClient::Cancel()
{
	// If the AppState is Ready, this performs a Cancel; otherwise the Cancel
	// button has become the Close button and the ticker has already started.
	if (AppState == EApplicationState::Ready)
	{
		Uploader.Cancel();
		StartUIWillCloseTicker();
	}
	AppState = EApplicationState::Closing;

	return FReply::Handled();
}

FReply FCrashReportClient::CopyCallstack()
{
	FPlatformMisc::ClipboardCopy(*DiagnosticText.ToString());
	return FReply::Handled();
}

FText FCrashReportClient::GetStatusText() const
{
	static const FText ClosingText = LOCTEXT("Closing", "Thank you for reporting this issue - closing automatically");
	return AppState == EApplicationState::CountingDown ? ClosingText : Uploader.GetStatusText();
}

FText FCrashReportClient::GetCancelButtonText() const
{
	return CancelButtonText;
}

FText FCrashReportClient::GetDiagnosticText() const
{
	static const FText ProcessingReportText = LOCTEXT("ProcessingReport", "Processing crash report ...");
	return DiagnoseReportTask.IsDone() ? DiagnosticText : ProcessingReportText;
}

EVisibility FCrashReportClient::SubmitButtonVisibility() const
{
	return AppState == EApplicationState::Ready ? EVisibility::Visible : EVisibility::Hidden;
}

FString FCrashReportClient::GetCrashedAppName() const
{
	return CrashedAppName;
}

void FCrashReportClient::UserCommentChanged(const FText& Comment, ETextCommit::Type CommitType)
{
	UserComment = Comment;

	// Implement Shift+Enter to commit shortcut
	if (CommitType == ETextCommit::OnEnter &&
		AppState == EApplicationState::Ready &&
		FSlateApplication::Get().GetModifierKeys().IsShiftDown())
	{
		Submit();
	}
}

bool FCrashReportClient::ShouldWindowBeHidden() const
{
	return AppState == EApplicationState::Closing;
}

void FCrashReportClient::RequestCloseWindow(const TSharedRef<SWindow>& Window)
{
	Cancel();
}

void FCrashReportClient::StartUIWillCloseTicker()
{
	FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FCrashReportClient::UIWillCloseTick), 1.f);
}

void FCrashReportClient::StoreCommentAndUpload()
{
	// Call upload even if the report is empty: pending reports will be sent if any
	ErrorReport.SetUserComment(UserComment.IsEmpty() ? LOCTEXT("NoComment", "No comment provided") : UserComment);
	Uploader.BeginUpload(ErrorReport);

	SubmittedCountdown = 5;
	AppState = EApplicationState::CountingDown;
	// Change the submit button text immediately (also sends diagnostics file if complete)
	UIWillCloseTick(0);
	StartUIWillCloseTicker();
}

bool FCrashReportClient::UIWillCloseTick(float UnusedDeltaTime)
{
	bool bCountingDown = AppState == EApplicationState::CountingDown;
	if (!bCountingDown && AppState != EApplicationState::Closing)
	{
		CRASHREPORTCLIENT_CHECK(false);
		return false;
	}

	static const FText CountdownTextFormat = LOCTEXT("CloseApplication", "Close ({0})");

	if( !bDiagnosticFileSent && DiagnoseReportTask.IsDone() )
	{
		auto DiagnosticsFilePath = ErrorReport.GetReportDirectory() / GDiagnosticsFilename;

		Uploader.LocalDiagnosisComplete(FPaths::FileExists(DiagnosticsFilePath) ? DiagnosticsFilePath : TEXT(""));
		bDiagnosticFileSent = true;	
	}

	if (bCountingDown)
	{
		CancelButtonText = FText::Format(CountdownTextFormat, FText::AsNumber(SubmittedCountdown));
		if (SubmittedCountdown-- == 0)
		{
			AppState = EApplicationState::Closing;
		}
		else
		{
			// More ticks, please
			return true;
		}
	}

	// IsWorkDone will always return true here (since uploader can't finish until the diagnosis has been sent), but it
	//  has the side effect of joining the worker thread.
	if( !Uploader.IsFinished() || !DiagnoseReportTask.IsDone() )
	{
		// More ticks, please
		return true;
	}

	FPlatformMisc::RequestExit(false /* don't force */);
	// No more ticks, thank you
	return false;
}

FText FCrashReportClient::FormatDiagnosticText( const FText& DiagnosticText, const FString MachineId, const FString EpicAccountId, const FString UserNameNoDot )
{
	if( FRocketSupport::IsRocket() )
	{
		return FText::Format( LOCTEXT( "CrashReportClientCallstackPattern", "MachineId:{0}\nEpicAccountId:{1}\n\n{2}" ), FText::FromString( MachineId ), FText::FromString( EpicAccountId ), DiagnosticText );
	}
	else
	{
		return FText::Format( LOCTEXT( "CrashReportClientCallstackPattern", "MachineId:{0}\nUserName:{1}\n\n{2}" ), FText::FromString( MachineId ), FText::FromString( UserNameNoDot ), DiagnosticText );
	}

}

#endif // !CRASH_REPORT_UNATTENDED_ONLY

void FDiagnoseReportWorker::DoWork()
{
	const FText ReportText = ErrorReport.DiagnoseReport();
	DiagnosticText = FCrashReportClient::FormatDiagnosticText( ReportText, MachineId, EpicAccountId, UserNameNoDot );
}

#undef LOCTEXT_NAMESPACE
