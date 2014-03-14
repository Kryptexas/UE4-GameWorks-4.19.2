// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CrashTrackerModule.cpp: Implements the FCrashTrackerModule class.
=============================================================================*/

#include "CrashTrackerPrivatePCH.h"
#include "EngineBuildSettings.h"

#if (PLATFORM_WINDOWS || PLATFORM_MAC) && !UE_BUILD_MINIMAL
	#define CRASH_TRACKER_SUPPORTED 1
#else
	#define CRASH_TRACKER_SUPPORTED 0
#endif


/** Logs keypresses to the crash tracker, as well as triggering it's completion upon a crash */
class FCrashTrackerEventLogger : public FStabilityEventLogger
{
public:
	FCrashTrackerEventLogger(TSharedPtr<FCrashVideoCapture> InVideoCapture = NULL);

	virtual void Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget) OVERRIDE;
	
	void OnHandleError();
	void OnHandleEnsure();

private:
	TWeakPtr<FCrashVideoCapture> VideoCapture;
};


FCrashTrackerEventLogger::FCrashTrackerEventLogger(TSharedPtr<FCrashVideoCapture> InVideoCapture)
	: VideoCapture(InVideoCapture)
{
}


void FCrashTrackerEventLogger::Log(EEventLog::Type Event, const FString& AdditionalContent, TSharedPtr<SWidget> Widget)
{
	FStabilityEventLogger::Log(Event, AdditionalContent, Widget);
	
	if (VideoCapture.IsValid())
	{
		if (Event == EEventLog::KeyDown)
		{
			check(AdditionalContent.Len());
			VideoCapture.Pin()->BufferKeyPress(AdditionalContent);
		}
	}
}


void FCrashTrackerEventLogger::OnHandleError()
{
	UE_LOG(LogCrashTracker, Log, TEXT("%s"), *GetLog());

	if (VideoCapture.IsValid())
	{
		// Finish capturing video
		VideoCapture.Pin()->EndCapture(/*bSaveCapture=*/true);
	}
}


void FCrashTrackerEventLogger::OnHandleEnsure()
{
	if (VideoCapture.IsValid())
	{
		// Capture video for the ensure
		VideoCapture.Pin()->SaveCaptureNow(VideoCapture.Pin()->CaptureVideoPath);
	}
}


class FCrashTrackerModule
	: public ICrashTrackerModule
{
public:
	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface interface


	// Begin ICrashTracker interface
	virtual void Update(float DeltaSeconds) OVERRIDE;
	virtual void ForceCompleteCapture() OVERRIDE;
	virtual void SetCrashTrackingEnabled(bool bEnabled) OVERRIDE;
	virtual bool IsVideoCaptureAvailable() const OVERRIDE;
	virtual bool IsCurrentlyCapturing() const OVERRIDE;
	virtual void InvalidateCrashTrackerFrame() OVERRIDE;
	virtual EWriteUserCaptureVideoError::Type WriteUserVideoNow( FString& OutFinalSaveName ) OVERRIDE;
	// End ICrashTracker interface

private:
	// The video capture instance
	TSharedPtr<FCrashVideoCapture> VideoCapture;
	// The event logger instance
	TSharedPtr<FCrashTrackerEventLogger> EventLogger;
};

void FCrashTrackerModule::StartupModule()
{
	FCrashVideoCapture::CaptureVideoPath = FPaths::GameLogDir() + TEXT("CrashVideo.avi");

	// kill it just in case it's hanging around from last session's capture
	IFileManager::Get().Delete(*FCrashVideoCapture::CaptureVideoPath);
	
	VideoCapture.Reset();
#if CRASH_TRACKER_SUPPORTED
	if (GIsEditor &&
		(GRHIShaderPlatform == SP_PCD3D_SM4 || GRHIShaderPlatform == SP_PCD3D_SM5 || GRHIShaderPlatform == SP_OPENGL_SM4 ))
	{
		bool bCrashTrackerShouldBeEnabled = false;
#if UE_BUILD_DEVELOPMENT
		bCrashTrackerShouldBeEnabled =
			false && // Disable crash tracker by default for now unless someone enables it.
			!GIsDemoMode &&
			!FParse::Param( FCommandLine::Get(), TEXT("disablecrashtracker") ) &&
			!GIsBenchmarking &&
			!FPlatformMisc::IsDebuggerPresent();
#endif
		bool bForceEnableCrashTracker = FParse::Param( FCommandLine::Get(), TEXT("forceenablecrashtracker") );

		if (bCrashTrackerShouldBeEnabled || bForceEnableCrashTracker)
		{
			VideoCapture = MakeShareable(new FCrashVideoCapture);
		}
	}
#endif //CRASH_TRACKER_SUPPORTED
	
	EventLogger = MakeShareable( new FCrashTrackerEventLogger(VideoCapture) );
	FSlateApplication::Get().SetSlateUILogger(EventLogger);
	// hook up event logger to global assert/ensure hook
	FCoreDelegates::OnHandleSystemEnsure.Add(FCoreDelegates::FOnHandleSystemEnsure::FDelegate::CreateRaw(EventLogger.Get(), &FCrashTrackerEventLogger::OnHandleEnsure));
	FCoreDelegates::OnHandleSystemError.Add(FCoreDelegates::FOnHandleSystemError::FDelegate::CreateRaw(EventLogger.Get(), &FCrashTrackerEventLogger::OnHandleError));
	
	if (VideoCapture.IsValid())
	{
		UE_LOG(LogCrashTracker, Log, TEXT("Crashtracker beginning capture."));
		VideoCapture->BeginCapture(FSlateApplication::Get().GetRenderer().Get());
	}
	else
	{
		UE_LOG(LogCrashTracker, Log, TEXT("Crashtracker disabled due to settings."));
	}
}


void FCrashTrackerModule::ShutdownModule()
{
	// Make sure we remove the hook to prevent calling this when it's garbage
	FCoreDelegates::OnHandleSystemEnsure.Remove(FCoreDelegates::FOnHandleSystemEnsure::FDelegate::CreateRaw(EventLogger.Get(), &FCrashTrackerEventLogger::OnHandleEnsure));
	FCoreDelegates::OnHandleSystemError.Remove(FCoreDelegates::FOnHandleSystemError::FDelegate::CreateRaw(EventLogger.Get(), &FCrashTrackerEventLogger::OnHandleError));

	VideoCapture.Reset();
}


void FCrashTrackerModule::Update(float DeltaSeconds)
{
	if (VideoCapture.IsValid())
	{
		VideoCapture->Update(DeltaSeconds);
	}
}


void FCrashTrackerModule::ForceCompleteCapture()
{
	if (VideoCapture.IsValid())
	{
		VideoCapture->EndCapture(/*bSaveCapture=*/true);
		check(0);
	}
}


void FCrashTrackerModule::SetCrashTrackingEnabled(bool bEnabled)
{
	if (VideoCapture.IsValid())
	{
		VideoCapture->SetCrashTrackingEnabled(bEnabled);
	}
}


bool FCrashTrackerModule::IsVideoCaptureAvailable() const
{
	return VideoCapture.IsValid();
}


bool FCrashTrackerModule::IsCurrentlyCapturing() const
{
	return VideoCapture.IsValid() && VideoCapture->IsCapturing();
}


void FCrashTrackerModule::InvalidateCrashTrackerFrame()
{
	if (VideoCapture.IsValid())
	{
		VideoCapture->InvalidateFrame();
	}
}


EWriteUserCaptureVideoError::Type FCrashTrackerModule::WriteUserVideoNow( FString& OutFinalSaveName )
{
	EWriteUserCaptureVideoError::Type WriteResult = EWriteUserCaptureVideoError::None;
	if(VideoCapture.IsValid())
	{
		// Check if the capture is running and try to create the video capture folder.
		if( !VideoCapture->IsCapturing() )
		{
			WriteResult = EWriteUserCaptureVideoError::CaptureNotRunning;		
		}		
		else if( !IFileManager::Get().MakeDirectory( *FPaths::VideoCaptureDir(), true )  )
		{			
			WriteResult = EWriteUserCaptureVideoError::FailedToCreateDirectory;
		}
	}
	else
	{
		WriteResult = EWriteUserCaptureVideoError::VideoCaptureInvalid;		
	}

	// If there was no error find a suitable filename and write the file.
	if( WriteResult == EWriteUserCaptureVideoError::None )
	{
		// The final save name
		OutFinalSaveName = "";
		// have we got a name
		bool bGotName = false;
		// Counter used to build filename
		int32 SaveNameCount = 1;
		while( bGotName == false )
		{
			// Build a name from the video capture path and an incrementing count
			OutFinalSaveName = FString::Printf(TEXT("%s_%02d.avi"), *(FPaths::VideoCaptureDir() + TEXT("CaptureVideo" ) ) , SaveNameCount );
			// FileSize returns INDEX_NONE if the file doesn't exist
			if( IFileManager::Get().FileSize( *OutFinalSaveName ) == INDEX_NONE )
			{
				bGotName = true;
			}
			// Try another filename
			SaveNameCount++;
		}
		// We now have a filename - write the video. This can only return false if the capture is not running - and we already checked for that
		VideoCapture->SaveCaptureNow( OutFinalSaveName );		
	}	
	return WriteResult;
}


IMPLEMENT_MODULE(FCrashTrackerModule, CrashTracker);
