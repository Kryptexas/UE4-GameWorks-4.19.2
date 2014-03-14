// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "MainFrame.h"

/** Notification class for asynchronous shader compiling. */
class FShaderCompilingNotificationImpl
	: public FTickableEditorObject
{

public:

	FShaderCompilingNotificationImpl()
	{
		LastEnableTime = 0;
	}

	/** Starts the notification. */
	void ShaderCompileStarted();

	/** Ends the notification. */
	void ShaderCompileFinished();

protected:
	/** FTickableEditorObject interface */
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const
	{
		return true;
	}
	virtual TStatId GetStatId() const OVERRIDE;

private:

	/** Tracks the last time the notification was started, used to avoid spamming. */
	double LastEnableTime;

	/** The source code symbol query in progress message */
	TWeakPtr<SNotificationItem> ShaderCompileNotificationPtr;
};

/** Global notification object. */
FShaderCompilingNotificationImpl GShaderCompilingNotification;

void FShaderCompilingNotificationImpl::ShaderCompileStarted()
{
	LastEnableTime = FPlatformTime::Seconds();

	// Starting a new request! Notify the UI.
	if (ShaderCompileNotificationPtr.IsValid())
	{
		ShaderCompileNotificationPtr.Pin()->ExpireAndFadeout();
	}
	
	FNotificationInfo Info( NSLOCTEXT("ShaderCompile", "ShaderCompileInProgress", "Compiling Shaders") );
	Info.bFireAndForget = false;
	
	// Setting fade out and expire time to 0 as the expire message is currently very obnoxious
	Info.FadeOutDuration = 0.0f;
	Info.ExpireDuration = 0.0f;

	ShaderCompileNotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

	if (ShaderCompileNotificationPtr.IsValid())
	{
		ShaderCompileNotificationPtr.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FShaderCompilingNotificationImpl::ShaderCompileFinished()
{
	// Finished all requests! Notify the UI.
	TSharedPtr<SNotificationItem> NotificationItem = ShaderCompileNotificationPtr.Pin();

	if (NotificationItem.IsValid())
	{
		NotificationItem->SetText( NSLOCTEXT("ShaderCompile", "ShaderCompileComplete", "Shaders finished compiling!") );
		NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
		NotificationItem->ExpireAndFadeout();

		ShaderCompileNotificationPtr.Reset();
	}
}

void FShaderCompilingNotificationImpl::Tick(float DeltaTime)
{
	// Trigger a new notification if we are doing an async shader compile, and we haven't displayed the notification recently
	if (GShaderCompilingManager->ShouldDisplayCompilingNotification() 
		&& !ShaderCompileNotificationPtr.IsValid()
		&& (FPlatformTime::Seconds() - LastEnableTime) > 5)
	{
		ShaderCompileStarted();
	}
	// Disable the notification when we are no longer doing an async compile
	else if (!GShaderCompilingManager->IsCompiling() && ShaderCompileNotificationPtr.IsValid())
	{
		ShaderCompileFinished();
	}
	else if (GShaderCompilingManager->IsCompiling() && ShaderCompileNotificationPtr.IsValid())
	{
		TSharedPtr<SNotificationItem> NotificationItem = ShaderCompileNotificationPtr.Pin();

		if (NotificationItem.IsValid())
		{
			FFormatNamedArguments Args;
			Args.Add( TEXT("ShaderJobs"), FText::AsNumber( GShaderCompilingManager->GetNumRemainingJobs() ) );
			FText ProgressMessage = FText::Format(NSLOCTEXT("ShaderCompile", "ShaderCompileInProgressFormat", "Compiling Shaders ({ShaderJobs})"), Args);

			NotificationItem->SetText( ProgressMessage );
		}
	}
}

TStatId FShaderCompilingNotificationImpl::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FShaderCompilingNotificationImpl, STATGROUP_Tickables);
}
