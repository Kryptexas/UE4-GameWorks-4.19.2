// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SettingsEditorPrivatePCH.h"
#include "ISettingsEditorModule.h"
#include "NotificationManager.h"


#define LOCTEXT_NAMESPACE "SSettingsEditor"


/** Manages the notification for when the application needs to be restarted due to a settings change */
class FApplicationRestartRequiredNotification
{
public:
	void SetOnRestartApplicationCallback( FSimpleDelegate InRestartApplicationDelegate )
	{
		RestartApplicationDelegate = InRestartApplicationDelegate;
	}

	void OnRestartRequired()
	{
		TSharedPtr<SNotificationItem> NotificationPin = NotificationPtr.Pin();
		if (NotificationPin.IsValid() || !RestartApplicationDelegate.IsBound())
		{
			return;
		}
		
		FNotificationInfo Info( LOCTEXT("RestartRequiredTitle", "Restart required to apply new settings") );

		// Add the buttons with text, tooltip and callback
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			LOCTEXT("RestartNow", "Restart Now"), 
			LOCTEXT("RestartNowToolTip", "Restart now to finish applying your new settings."), 
			FSimpleDelegate::CreateRaw(this, &FApplicationRestartRequiredNotification::OnRestartClicked))
			);
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			LOCTEXT("RestartLater", "Restart Later"), 
			LOCTEXT("RestartLaterToolTip", "Dismiss this notificaton without restarting. Some new settings will not be applied."), 
			FSimpleDelegate::CreateRaw(this, &FApplicationRestartRequiredNotification::OnDismissClicked))
			);

		// We will be keeping track of this ourselves
		Info.bFireAndForget = false;

		// Set the width so that the notification doesn't resize as its text changes
		Info.WidthOverride = 300.0f;

		Info.bUseLargeFont = false;
		Info.bUseThrobber = false;
		Info.bUseSuccessFailIcons = false;

		// Launch notification
		NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);
		NotificationPin = NotificationPtr.Pin();

		if (NotificationPin.IsValid())
		{
			NotificationPin->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}



private:
	void OnRestartClicked()
	{
		TSharedPtr<SNotificationItem> NotificationPin = NotificationPtr.Pin();
		if (NotificationPin.IsValid())
		{
			NotificationPin->SetText(LOCTEXT("RestartingNow", "Restarting..."));
			NotificationPin->SetCompletionState(SNotificationItem::CS_Success);
			NotificationPin->ExpireAndFadeout();
			NotificationPtr.Reset();
		}

		RestartApplicationDelegate.ExecuteIfBound();
	}

	void OnDismissClicked()
	{
		TSharedPtr<SNotificationItem> NotificationPin = NotificationPtr.Pin();
		if (NotificationPin.IsValid())
		{
			NotificationPin->SetText(LOCTEXT("RestartDismissed", "Restart Dismissed..."));
			NotificationPin->SetCompletionState(SNotificationItem::CS_None);
			NotificationPin->ExpireAndFadeout();
			NotificationPtr.Reset();
		}
	}

	/** Used to reference to the active restart notification */
	TWeakPtr<SNotificationItem> NotificationPtr;

	/** Used to actually restart the application */
	FSimpleDelegate RestartApplicationDelegate;
};


/**
 * Implements the SettingsEditor module.
 */
class FSettingsEditorModule
	: public ISettingsEditorModule
{
public:

	// ISettingsEditorModule interface

	virtual TSharedRef<SWidget> CreateEditor( const TSharedRef<ISettingsEditorModel>& Model ) override
	{
		return SNew(SSettingsEditor, Model)
			.OnApplicationRestartRequired(FSimpleDelegate::CreateRaw(this, &FSettingsEditorModule::OnApplicationRestartRequired));
	}

	virtual ISettingsEditorModelRef CreateModel( const TSharedRef<ISettingsContainer>& SettingsContainer ) override
	{
		return MakeShareable(new FSettingsEditorModel(SettingsContainer));
	}

	virtual void OnApplicationRestartRequired() override
	{
		ApplicationRestartRequiredNotification.OnRestartRequired();
	}

	virtual void SetRestartApplicationCallback( FSimpleDelegate InRestartApplicationDelegate ) override
	{
		ApplicationRestartRequiredNotification.SetOnRestartApplicationCallback(InRestartApplicationDelegate);
	}

private:

	FApplicationRestartRequiredNotification ApplicationRestartRequiredNotification;
};


IMPLEMENT_MODULE(FSettingsEditorModule, SettingsEditor);


#undef LOCTEXT_NAMESPACE
