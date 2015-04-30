// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FeedbackContext.h"
#include "SNotificationList.h"
#include "INotificationWidget.h"
#include "MessageLog.h"
#include "AutoReimportUtilities.h"

/** Feedback context that overrides GWarn for import operations to prevent popup spam */
class FReimportFeedbackContext : public FFeedbackContext, public INotificationWidget, public TSharedFromThis<FReimportFeedbackContext>
{
public:
	/** Constructor */
	FReimportFeedbackContext(const TAttribute<FText>& InMainText, const FSimpleDelegate& InOnPauseClicked, const FSimpleDelegate& InOnAbortClicked);

	/** Destroy this reimport context */
	void Hide();

	/** Revive the notification if it was destroyed */
	void Show();

	/** Get the message log that this context is using */
	FMessageLog& GetMessageLog() { return MessageLog; }

	/** Add a message to the context (and message log) */
	void AddMessage(EMessageSeverity::Type Severity, const FText& Message);

	/** Add a custom widget to the context */
	void AddWidget(const TSharedRef<SWidget>& Widget);

	/** INotificationWidget and FFeedbackContext overrides */
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) override {}
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) override {}
	virtual TSharedRef<SWidget> AsWidget() override;
	virtual void StartSlowTask( const FText& Task, bool bShowCancelButton=false) override;

	/** True if we will suppress slow task messages from being shown on the UI, false if otherwise */
	bool bSuppressSlowTaskMessages;

private:

	/** Called when our notification has expired */
	void OnNotificationExpired();

	TAttribute<FText> MainTextAttribute;
	FSimpleDelegate OnPauseClickedEvent, OnAbortClickedEvent;

	/** Message log for output of errors and log messages */
	FMessageLog MessageLog;

	/** The notification that is shown when the context is active */
	TSharedPtr<class SNotificationItem> Notification;

	/** The notification content */
	TSharedPtr<class SReimportFeedback> NotificationContent;
};