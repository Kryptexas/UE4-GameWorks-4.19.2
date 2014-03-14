// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SharedSettingsWidgetsPrivatePCH.h"
#include "SExternalImageReference.h"

#include "AssetSelection.h"
#include "EditorStyle.h"
#include "IExternalImagePickerModule.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "SExternalImageReference"

/////////////////////////////////////////////////////
// SExternalImageReference

SExternalImageReference::SExternalImageReference()
{
}

void SExternalImageReference::Construct(const FArguments& InArgs, const FString& InBaseFilename, const FString& InOverrideFilename)
{
	FileDescription = InArgs._FileDescription;

	FExternalImagePickerConfiguration GameSplashConfig;
	GameSplashConfig.TargetImagePath = InOverrideFilename;
	GameSplashConfig.DefaultImagePath = InBaseFilename;
	GameSplashConfig.OnExternalImagePicked = FOnExternalImagePicked::CreateSP(this, &SExternalImageReference::HandleExternalImagePicked);
	GameSplashConfig.RequiredImageDimensions = InArgs._RequiredSize;
	GameSplashConfig.bRequiresSpecificSize = InArgs._RequiredSize.X >= 0;
	GameSplashConfig.MaxDisplayedImageDimensions = InArgs._MaxDisplaySize;

	ChildSlot
	[
		IExternalImagePickerModule::Get().MakeEditorWidget(GameSplashConfig)
	];
}


bool SExternalImageReference::HandleExternalImagePicked(const FString& InChosenImage, const FString& InTargetImage)
{
	bool bSucceeded = true;

	ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();

	// first check for source control check out
	if (ISourceControlModule::Get().IsEnabled())
	{
		FSourceControlStatePtr SourceControlState = Provider.GetState(InTargetImage, EStateCacheUsage::ForceUpdate);
		if (SourceControlState.IsValid())
		{
			if (SourceControlState->IsSourceControlled() && SourceControlState->CanCheckout())
			{
				ECommandResult::Type Result = Provider.Execute(ISourceControlOperation::Create<FCheckOut>(), InTargetImage);
				bSucceeded = (Result == ECommandResult::Succeeded);
				if (!bSucceeded)
				{
					FText NotificationErrorText = FText::Format(LOCTEXT("ExternalImageSourceControlCheckoutError", "Could not check out {0} file."), FileDescription);

					FNotificationInfo Info(NotificationErrorText);
					Info.ExpireDuration = 3.0f;

					FSlateNotificationManager::Get().AddNotification(Info);
				}
			}
		}
	}

	// now try and copy the file
	if (bSucceeded)
	{
		bSucceeded = (IFileManager::Get().Copy(*InTargetImage, *InChosenImage, true, true) == COPY_OK);
		if (!bSucceeded)
		{
			FText NotificationErrorText = FText::Format(LOCTEXT("ExternalImageCopyError", "Could not overwrite {0} file."), FileDescription);

			FNotificationInfo Info(NotificationErrorText);
			Info.ExpireDuration = 3.0f;

			FSlateNotificationManager::Get().AddNotification(Info);
		}
	}

	// mark for add now if needed
	if (bSucceeded && ISourceControlModule::Get().IsEnabled())
	{
		FSourceControlStatePtr SourceControlState = Provider.GetState(InTargetImage, EStateCacheUsage::Use);
		if (SourceControlState.IsValid())
		{
			if (!SourceControlState->IsSourceControlled())
			{
				ECommandResult::Type Result = Provider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), InTargetImage);
				bSucceeded = (Result == ECommandResult::Succeeded);
				if (!bSucceeded)
				{
					FText NotificationErrorText = FText::Format(LOCTEXT("SplashScreenSourceControlMarkForAddError", "Could not mark {0} file for add."), FileDescription);

					FNotificationInfo Info(NotificationErrorText);
					Info.ExpireDuration = 3.0f;

					FSlateNotificationManager::Get().AddNotification(Info);
				}
			}
		}
	}

	return bSucceeded;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE