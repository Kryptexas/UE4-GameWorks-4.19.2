// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "DesktopPlatformModule.h"
#include "PreferencesEditor.h"
#include "MainFrame.h"
#include "MessageLog.h"
#include "PropertyEditing.h"
#include "PreferencesDetails.h"

#define LOCTEXT_NAMESPACE "PreferencesEditor"


/* FPreferencesDetails structors
 *****************************************************************************/

FPreferencesDetails::FPreferencesDetails( )
{
	BackupUserSettingsIni = FString::Printf(TEXT("%s_Backup.ini"), *FPaths::GetBaseFilename(GEditorUserSettingsIni, false));
}


/* FPreferencesDetails static interface
 *****************************************************************************/

TSharedRef<class IDetailCustomization> FPreferencesDetails::MakeInstance( )
{
	return MakeShareable(new FPreferencesDetails());
}


/* ILayoutDetails implementation
 *****************************************************************************/

void FPreferencesDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory(NAME_None, LOCTEXT("BackupAndReset", "Backup and Reset").ToString(), ECategoryPriority::Important );

	Category.AddCustomRow( LOCTEXT("ResetUILayout", "Reset UI Layout").ToString() )
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(2.0f))
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("ResetUILayout", "Reset UI Layout").ToString())
					.OnClicked(this, &FPreferencesDetails::HandleResetUILayoutClicked)
				]
			
			+SHorizontalBox::Slot()
				.Padding(FMargin(0.0f, 2.0f, 0.0f, 0.0f))
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LOCTEXT("ResettingDescription", "Resetting the layout will make a backup of your current user settings.").ToString())
				]
		];
}


/* FPreferencesDetails implementation
 *****************************************************************************/

void* FPreferencesDetails::ChooseParentWindowHandle( )
{
	void* ParentWindowWindowHandle = NULL;
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	return ParentWindowWindowHandle;
}


void FPreferencesDetails::MakeBackup()
{
	GEditor->SaveEditorUserSettings();

	if( COPY_Fail == IFileManager::Get().Copy(*BackupUserSettingsIni, *GEditorUserSettingsIni) )
	{
		FMessageLog EditorErrors("EditorErrors");
		if(!FPaths::FileExists(GEditorUserSettingsIni))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("FileName"), FText::FromString(GEditorUserSettingsIni));
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_NoExist_Notification", "Unsuccessful backup! {FileName} does not exist!"), Arguments));
		}
		else if(IFileManager::Get().IsReadOnly(*BackupUserSettingsIni))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("FileName"), FText::FromString(FPaths::ConvertRelativePathToFull(BackupUserSettingsIni)));
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_ReadOnly_Notification", "Unsuccessful backup! {FileName} is read-only!"), Arguments));
		}
		else
		{
			// We don't specifically know why it failed, this is a fallback.
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("SourceFileName"), FText::FromString(GEditorUserSettingsIni));
			Arguments.Add(TEXT("BackupFileName"), FText::FromString(FPaths::ConvertRelativePathToFull(BackupUserSettingsIni)));
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_Fallback_Notification", "Unsuccessful backup of {SourceFileName} to {BackupFileName}"), Arguments));
		}
		EditorErrors.Notify(LOCTEXT("BackupUnsuccessful_Title", "Backup Unsuccessful!"));
	}
	else
	{
		FNotificationInfo ErrorNotification( FText::GetEmpty() );
		ErrorNotification.bFireAndForget = true;
		ErrorNotification.ExpireDuration = 3.0f;
		ErrorNotification.bUseThrobber = true;
		ErrorNotification.Hyperlink = FSimpleDelegate::CreateRaw(this, &FPreferencesDetails::OpenBackupDirectory);
		ErrorNotification.HyperlinkText = LOCTEXT("SuccessfulBackup_Notification_Hyperlink", "Open Directory");
		ErrorNotification.Text = LOCTEXT("SuccessfulBackup_Notification", "Backup Successful!");
		ErrorNotification.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
		FSlateNotificationManager::Get().AddNotification(ErrorNotification);
	}
}


void FPreferencesDetails::OpenBackupDirectory( )
{
	FString BackupFile = FPaths::ConvertRelativePathToFull(BackupUserSettingsIni);
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*FPaths::GetPath(BackupFile));
}


/* FPreferencesDetails callbacks
 *****************************************************************************/

FReply FPreferencesDetails::HandleResetUILayoutClicked()
{
	if(EAppReturnType::Ok == OpenMsgDlgInt(EAppMsgType::OkCancel, LOCTEXT( "ActionRestartMsg", "This action requires the editor to restart; you will be prompted to save any changes. Continue?" ), LOCTEXT( "ResetUILayout_Title", "Reset UI Layout" ) ) )
	{
		MakeBackup();
			
		// Clean out the Editor Layouts, so the layout will be set to the default when the editor restarts.
		FUnrealEdMisc::Get().AllowSavingLayoutOnClose(false);

		FUnrealEdMisc::Get().RestartEditor(false);
	};
		
	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
