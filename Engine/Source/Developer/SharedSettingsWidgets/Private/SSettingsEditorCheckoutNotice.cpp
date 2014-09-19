// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SharedSettingsWidgetsPrivatePCH.h"
#include "EditorStyle.h"
#include "SSettingsEditorCheckoutNotice.h"
#include "ISourceControlModule.h"


#define LOCTEXT_NAMESPACE "SSettingsEditorCheckoutNotice"


/* SSettingsEditorCheckoutNotice interface
 *****************************************************************************/

void SSettingsEditorCheckoutNotice::Construct( const FArguments& InArgs )
{
	OnFileProbablyModifiedExternally = InArgs._OnFileProbablyModifiedExternally;
	ConfigFilePath = InArgs._ConfigFilePath;

	DefaultConfigCheckOutTimer = 0.0f;
	DefaultConfigCheckOutNeeded = false;
	DefaultConfigQueryInProgress = false;

	// default configuration notice
	ChildSlot
	[
		SNew(SBorder)
			.BorderBackgroundColor(this, &SSettingsEditorCheckoutNotice::GetLockedOrUnlockedStatusBarColor)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.LightGroupBorder"))
			.Padding(8.0f)
			[
				SNew(SWidgetSwitcher)
					.WidgetIndex(this, &SSettingsEditorCheckoutNotice::HandleNoticeSwitcherWidgetIndex)

				// Locked slot
				+ SWidgetSwitcher::Slot()
					[
						SNew(SHorizontalBox)

						// Locked icon
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericLock"))
							]

						// Locked notice
						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleLockedStatusText)
									.ColorAndOpacity(FLinearColor::White)
									.ShadowColorAndOpacity(FLinearColor::Black)
									.ShadowOffset(FVector2D::UnitVector)
							]

						// Check out button
						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
									.OnClicked(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonClicked)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonText)
									.ToolTipText(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonToolTip)
									.Visibility(this, &SSettingsEditorCheckoutNotice::HandleCheckOutButtonVisibility)
							]

						// Source control status throbber
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0.0f, 1.0f)
							[
								SNew(SThrobber)
									.Visibility(this, &SSettingsEditorCheckoutNotice::HandleThrobberVisibility)
							]
					]

				// Unlocked slot
				+ SWidgetSwitcher::Slot()
					[
						SNew(SHorizontalBox)

						// Unlocked icon
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
									.Image(FEditorStyle::GetBrush("GenericUnlock"))
							]

						// Unlocked notice
						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(16.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.Text(this, &SSettingsEditorCheckoutNotice::HandleUnlockedStatusText)
									.ColorAndOpacity(FLinearColor::White)
									.ShadowColorAndOpacity(FLinearColor::Black)
									.ShadowOffset(FVector2D::UnitVector)
							]
					]
			]
	];
}


/* SSettingsEditorCheckoutNotice callbacks
 *****************************************************************************/

FReply SSettingsEditorCheckoutNotice::HandleCheckOutButtonClicked()
{
	FString TargetFilePath = ConfigFilePath.Get();

	if (ISourceControlModule::Get().IsEnabled())
	{
		FText ErrorMessage;

		if (!SourceControlHelpers::CheckoutOrMarkForAdd(TargetFilePath, FText::FromString(TargetFilePath), NULL, ErrorMessage))
		{
			FNotificationInfo Info(ErrorMessage);
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}
	else
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*TargetFilePath, false))
		{
			FText NotificationErrorText = FText::Format(LOCTEXT("FailedToMakeWritable", "Could not make {0} writable."), FText::FromString(TargetFilePath));

			FNotificationInfo Info(NotificationErrorText);
			Info.ExpireDuration = 3.0f;

			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}

	return FReply::Handled();
}


FText SSettingsEditorCheckoutNotice::HandleCheckOutButtonText( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("CheckOutFile", "Check Out File");
	}

	return LOCTEXT("MakeWritable", "Make Writable");
}


FText SSettingsEditorCheckoutNotice::HandleCheckOutButtonToolTip( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return LOCTEXT("CheckOutFileTooltip", "Check out the default configuration file that holds these settings.");
	}

	return LOCTEXT("MakeWritableTooltip", "Make the default configuration file that holds these settings writable.");
}

EVisibility SSettingsEditorCheckoutNotice::HandleCheckOutButtonVisibility( ) const
{
	if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
	{
		return !DefaultConfigQueryInProgress ? EVisibility::Visible : EVisibility::Hidden;
	}

	return EVisibility::Hidden;
}

FText SSettingsEditorCheckoutNotice::HandleLockedStatusText() const
{
	FText ConfigFilename = FText::FromString(FPaths::GetCleanFilename(ConfigFilePath.Get()));

	return FText::Format(ISourceControlModule::Get().IsEnabled() ?
		LOCTEXT("DefaultSettingsNotice_WithSourceControl", "These settings are saved in {0}, which is not currently checked out.") :
		LOCTEXT("DefaultSettingsNotice_Source", "These settings are saved {0}, which is not currently writable."), ConfigFilename);
}

FText SSettingsEditorCheckoutNotice::HandleUnlockedStatusText() const
{
	FText ConfigFilename = FText::FromString(FPaths::GetCleanFilename(ConfigFilePath.Get()));
	
	return FText::Format(ISourceControlModule::Get().IsEnabled() ?
		LOCTEXT("DefaultSettingsNotice_CheckedOut", "These settings are saved in {0}, which is currently checked out.") :
		LOCTEXT("DefaultSettingsNotice_Writable", "These settings are saved in {0}, which is currently writable."), ConfigFilename);
}

FSlateColor SSettingsEditorCheckoutNotice::GetLockedOrUnlockedStatusBarColor() const
{
	return IsUnlocked() ? FLinearColor::Green : FLinearColor::Yellow;
}


EVisibility SSettingsEditorCheckoutNotice::HandleThrobberVisibility() const
{
	if(ISourceControlModule::Get().IsEnabled())
	{
		return DefaultConfigQueryInProgress ? EVisibility::Visible : EVisibility::Collapsed;
	}
	return EVisibility::Collapsed;
}


void SSettingsEditorCheckoutNotice::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// cache selected settings object's configuration file state
	DefaultConfigCheckOutTimer += InDeltaTime;

	if (DefaultConfigCheckOutTimer >= 1.0f)
	{
		bool NewCheckOutNeeded = false;

		DefaultConfigQueryInProgress = false;
		FString CachedConfigFileName = ConfigFilePath.Get();
		if (!CachedConfigFileName.IsEmpty())
		{
			if (ISourceControlModule::Get().IsEnabled())
			{
				// note: calling QueueStatusUpdate often does not spam status updates as an internal timer prevents this
				ISourceControlModule::Get().QueueStatusUpdate(CachedConfigFileName);

				ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
				FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(CachedConfigFileName, EStateCacheUsage::Use);
				NewCheckOutNeeded = SourceControlState.IsValid() && SourceControlState->CanCheckout();
				DefaultConfigQueryInProgress = SourceControlState.IsValid() && SourceControlState->IsUnknown();
			}
			else
			{
				NewCheckOutNeeded = (FPaths::FileExists(CachedConfigFileName) && IFileManager::Get().IsReadOnly(*CachedConfigFileName));
			}

			// file has been checked in or reverted
			if ((NewCheckOutNeeded == true) && (DefaultConfigCheckOutNeeded == false))
			{
				OnFileProbablyModifiedExternally.ExecuteIfBound();
			}
		}

		DefaultConfigCheckOutNeeded = NewCheckOutNeeded;
		DefaultConfigCheckOutTimer = 0.0f;
	}
}


bool SSettingsEditorCheckoutNotice::IsUnlocked() const
{
	return !DefaultConfigCheckOutNeeded && !DefaultConfigQueryInProgress;
}

#undef LOCTEXT_NAMESPACE
