// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "CrashReportClient.h"
#include "UniquePtr.h"

#define LOCTEXT_NAMESPACE "CrashReportClient"

const TCHAR* GServerIP = TEXT("http://crashreporter.epicgames.com:57005");
// Must match filename specified in RunMinidumpDiagnostics
const TCHAR* GDiagnosticsFilename = TEXT("Diagnostics.txt");

#if !CRASH_REPORT_UNATTENDED_ONLY

FCrashReportClient::FCrashReportClient(const FString& Directory)
	: ReportDirectory(Directory)
	, AppState(EApplicationState::Ready)
	, SubmittedCountdown(-1)
	, bDiagnosticFileSent(false)
	, ErrorReportFiles(Directory)
	, Uploader(GServerIP)
	, CancelButtonText(LOCTEXT("Cancel", "Don't Send"))
{
	CrashedAppName = ErrorReportFiles.FindCrashedAppName();

	// Rocket builds will not try to analyze the report locally. Reports will be sent to the crash server and analyzed there.
	if (FRocketSupport::IsRocket())
	{
		DiagnosticText = LOCTEXT("NoDiagnoseReportRocket", "We apologize for the inconvenience.\nPlease send this crash report to help improve our software.");
	}
	else
	{
		if (!ErrorReportFiles.TryReadDiagnosticsFile(DiagnosticText) && !FParse::Param(FCommandLine::Get(), TEXT("no-local-diagnosis")))
		{
			auto& Worker = DiagnoseReportTask.GetTask();
			Worker.DiagnosticText = &DiagnosticText;
			Worker.ErrorReportFiles = &ErrorReportFiles;
			DiagnoseReportTask.StartBackgroundTask();
		}
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
	return DiagnoseReportTask.IsWorkDone() ? DiagnosticText : ProcessingReportText;
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
	ErrorReportFiles.SetUserComment(UserComment.IsEmpty() ? LOCTEXT("NoComment", "No comment provided") : UserComment);

	// Using GetCleanFilename to actually get directory leaf name
	Uploader.BeginUpload(FPaths::GetCleanFilename(ReportDirectory), ErrorReportFiles);

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
		check(false);
		return false;
	}

	static const FText CountdownTextFormat = LOCTEXT("CloseApplication", "Close ({0})");

	if (!bDiagnosticFileSent && DiagnoseReportTask.IsWorkDone())
	{
		auto DiagnosticsFilePath = ReportDirectory / GDiagnosticsFilename;

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

	// IsWorkDone will always return true here (since uploader can't finish until
	// the diagnosis has been sent), but it has the side effect of joining the
	// worker thread.
	if (!Uploader.IsFinished() || !DiagnoseReportTask.IsDone())
	{
		// More ticks, please
		return true;
	}

	FPlatformMisc::RequestExit(false /* don't force */);
	// No more ticks, thank you
	return false;
}

#endif // !CRASH_REPORT_UNATTENDED_ONLY

void FDiagnoseReportWorker::DoWork()
{
	*DiagnosticText = ErrorReportFiles->DiagnoseReport();
}

const TCHAR* FDiagnoseReportWorker::Name()
{
	return TEXT("FDiagnoseCrashWorker");
}

#undef LOCTEXT_NAMESPACE
