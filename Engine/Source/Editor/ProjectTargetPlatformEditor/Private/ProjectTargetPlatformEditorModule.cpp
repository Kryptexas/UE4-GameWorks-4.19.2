// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectTargetPlatformEditorPrivatePCH.h"
#include "Settings.h"

#define LOCTEXT_NAMESPACE "FProjectTargetPlatformEditorModule"

/**
 * Implements the platform target platform editor module
 */
class FProjectTargetPlatformEditorModule : public IProjectTargetPlatformEditorModule
{
public:

	// Begin IProjectTargetPlatformEditorModule interface

	virtual TWeakPtr<SWidget> CreateProjectTargetPlatformEditorPanel() OVERRIDE
	{
		TSharedPtr<SWidget> Panel = SNew(SProjectTargetPlatformSettings);
		EditorPanels.Add(Panel);

		return Panel;
	}

	virtual void DestroyProjectTargetPlatformEditorPanel(const TWeakPtr<SWidget>& Panel) OVERRIDE
	{
		EditorPanels.Remove(Panel.Pin());
	}

	virtual void AddOpenProjectTargetPlatformEditorMenuItem(FMenuBuilder& MenuBuilder) const OVERRIDE
	{
		struct Local
		{
			static void OpenSettings( FName ContainerName, FName CategoryName, FName SectionName )
			{
				FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer(ContainerName, CategoryName, SectionName);
			}
		};

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TargetPlatformsMenuLabel", "Target Platforms..."),
			LOCTEXT("ProjectSettingsMenuToolTip", "Change which platforms this project is targeting"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&Local::OpenSettings, FName("Project"), FName("Game"), FName("TargetPlatforms")))
			);
	}

	virtual TSharedRef<SWidget> MakePlatformMenuItemWidget(const ITargetPlatform* const Platform, const bool bForCheckBox = false, const FText& DisplayNameOverride = FText()) const OVERRIDE
	{
		struct Local
		{
			static EVisibility IsUnsupportedPlatformWarningVisible(const FName PlatformName)
			{
				FProjectStatus ProjectStatus;
				return (!IProjectManager::Get().QueryStatusForCurrentProject(ProjectStatus) || ProjectStatus.IsTargetPlatformSupported(PlatformName)) ? EVisibility::Hidden : EVisibility::Visible;
			}
		};

		return 
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin((bForCheckBox) ? 2 : 13, 0, 2, 0))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride(MultiBoxConstants::MenuIconSize)
					.HeightOverride(MultiBoxConstants::MenuIconSize)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName())))
					]
				]
				+SOverlay::Slot()
				.Padding(FMargin(MultiBoxConstants::MenuIconSize * 0.5f, 0, 0, 0))
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Bottom)
				[
					SNew(SBox)
					.WidthOverride(MultiBoxConstants::MenuIconSize * 0.9f)
					.HeightOverride(MultiBoxConstants::MenuIconSize * 0.9f)
					[
						SNew(SImage)
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&Local::IsUnsupportedPlatformWarningVisible, FName(*Platform->PlatformName()))))
						.Image(FEditorStyle::GetBrush("Launcher.Platform.Warning"))
					]
				]
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(FMargin((bForCheckBox) ? 2 : 7, 0, 6, 0))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FEditorStyle::Get(), "Menu.Label")
				.Text((DisplayNameOverride.IsEmpty()) ? Platform->DisplayName() : DisplayNameOverride)
			];
	}

	virtual bool ShowUnsupportedTargetWarning(const FName PlatformName) const OVERRIDE
	{
		// Don't show the warning during automation testing; the dlg is modal and blocks
		FProjectStatus ProjectStatus;
		if(!GIsAutomationTesting && IProjectManager::Get().QueryStatusForCurrentProject(ProjectStatus) && !ProjectStatus.IsTargetPlatformSupported(PlatformName))
		{
			ITargetPlatform *const Platform = GetTargetPlatformManager()->FindTargetPlatform(PlatformName.ToString());
			check(Platform);

			FFormatNamedArguments Args;
			Args.Add(TEXT("DisplayName"), Platform->DisplayName());
			FText WarningText = FText::Format(LOCTEXT("ShowUnsupportedTargetWarning_Message", "{DisplayName} is not listed as a target platform for this project, so may not run as expected.\n\nDo you wish to continue?"), Args);

			FSuppressableWarningDialog::FSetupInfo Info(
				WarningText, 
				LOCTEXT("ShowUnsupportedTargetWarning_Title", "Unsupported Platform"), 
				TEXT("SuppressUnsupportedTargetWarningDialog")
				);
			Info.ConfirmText = LOCTEXT("ShowUnsupportedTargetWarning_Confirm", "Continue");
			Info.CancelText = LOCTEXT("ShowUnsupportedTargetWarning_Cancel", "Cancel");
			FSuppressableWarningDialog UnsupportedTargetWarningDialog(Info);

			return UnsupportedTargetWarningDialog.ShowModal() != FSuppressableWarningDialog::EResult::Cancel;
		}

		return true;
	}

	// End IProjectTargetPlatformEditorModule interface

private:

	// Holds the collection of created editor panels.
	TArray<TSharedPtr<SWidget> > EditorPanels;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectTargetPlatformEditorModule, ProjectTargetPlatformEditor);
