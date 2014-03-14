// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncher.cpp: Implements the SSessionLauncher class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncher"

/* SSessionLauncher structors
 *****************************************************************************/

SSessionLauncher::~SSessionLauncher( )
{
	Model->OnProfileSelected().RemoveAll(this);
}


/* SSessionLauncher interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncher::Construct( const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow, const FSessionLauncherModelRef& InModel )
{
	CurrentTask = ESessionLauncherTasks::NoTask;
	Model = InModel;

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
					.Padding(FMargin(8.0f, 6.0f, 8.0f, 4.0f))
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						// common launcher task bar
						SAssignNew(Toolbar, SSessionLauncherToolbar, InModel, this)
					]
			]

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(8.0f, 8.0f, 0.0f, 8.0f)
					[
						SAssignNew(WidgetSwitcher, SWidgetSwitcher)
							.WidgetIndex((int32)ESessionLauncherTasks::QuickLaunch)

						+ SWidgetSwitcher::Slot()
							[
								// empty panel
								SNew(SBorder)
							]

						+ SWidgetSwitcher::Slot()
							[
								// deploy build panel
								SNew(SSessionLauncherBuildTaskSettings, InModel)
							]

						+ SWidgetSwitcher::Slot()
							[
								// deploy build panel
								SNew(SSessionLauncherDeployTaskSettings, InModel)
							]

						+ SWidgetSwitcher::Slot()
							[
								// Quick launch panel
								SNew(SSessionLauncherLaunchTaskSettings, InModel)
							]

						+ SWidgetSwitcher::Slot()
							[
								// advanced settings panel
								SNew(SSessionLauncherSettings, InModel)
							]

						+ SWidgetSwitcher::Slot()
							[
								// preview panel
								SNew(SSessionLauncherPreviewPage, InModel)
							]

						+ SWidgetSwitcher::Slot()
							.Padding(8.0f, 0.0f)
							[
								// progress panel
								SAssignNew(ProgressPanel, SSessionLauncherProgress)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(8.0f)
					[
						SNew(SSessionLauncherValidation, InModel)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SBorder)
							.Padding(FMargin(8.0f, 6.0f, 8.0f, 4.0f))
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.HAlign(HAlign_Left)
									[
										SNew(SOverlay)

										+ SOverlay::Slot()
											[
												// edit button
												SNew(SButton)
													.ContentPadding(FMargin(6.0f, 2.0f))
													.OnClicked(this, &SSessionLauncher::HandleEditButtonClicked)
													.ToolTipText(LOCTEXT("EditButtonTooltip", "Edit the settings of the selected profile.").ToString())
													[
														SNew(STextBlock)
															.Text(LOCTEXT("EditButtonLabel", "Edit Settings").ToString())
													]
													.Visibility(this, &SSessionLauncher::HandleEditButtonVisibility)
											]

										+ SOverlay::Slot()
											[
												// preview button
												SNew(SButton)
													.ContentPadding(FMargin(6.0f, 2.0f))
													.OnClicked(this, &SSessionLauncher::HandlePreviewButtonClicked)
													.ToolTipText(LOCTEXT("PreviewButtonTooltip", "Preview the settings of the selected profile.").ToString())
													[
														SNew(STextBlock)
															.Text(LOCTEXT("PreviewButtonLabel", "Preview Settings").ToString())
													]
													.Visibility(this, &SSessionLauncher::HandlePreviewButtonVisibility)
											]
									]

								+ SHorizontalBox::Slot()
									.FillWidth(1.0f)
									.HAlign(HAlign_Right)
									[
										SNew(SOverlay)

										+ SOverlay::Slot()
											[
												// launch button
												SNew(SButton)
													.ContentPadding(FMargin(6.0f, 2.0f))
													.IsEnabled(this, &SSessionLauncher::HandleFinishButtonIsEnabled)
													.OnClicked(this, &SSessionLauncher::HandleFinishButtonClicked)
													.ToolTipText(LOCTEXT("FinishButtonTooltip", "Launch the task as configured in the selected profile.").ToString())
													[
														SNew(STextBlock)
															.Text(LOCTEXT("FinishButtonLabel", "Launch").ToString())
													]
													.Visibility(this, &SSessionLauncher::HandleFinishButtonVisibility)
											]

										+ SOverlay::Slot()
											[
												// cancel button
												SNew(SButton)
													.ContentPadding(FMargin(6.0f, 2.0f))
													.OnClicked(this, &SSessionLauncher::HandleCancelButtonClicked)
													.ToolTipText(LOCTEXT("CancelButtonTooltip", "Cancel the task.").ToString())
													[
														SNew(STextBlock)
															.Text(LOCTEXT("CancelButtonLabel", "Cancel").ToString())
													]
													.Visibility(this, &SSessionLauncher::HandleCancelButtonVisibility)
											]

										+ SOverlay::Slot()
											[
												SNew(SHorizontalBox)
													.Visibility(this, &SSessionLauncher::HandleCancelingThrobberVisibility)

												+ SHorizontalBox::Slot()
													.AutoWidth()
													[
														SNew(SCircularThrobber)
															.NumPieces(5)
															.Radius(10.0f)
													]

												+ SHorizontalBox::Slot()
													.AutoWidth()
													.Padding(4.0f, 0.0f, 0.0f, 0.0f)
													.VAlign(VAlign_Center)
													[
														SNew(STextBlock)
															.Text(LOCTEXT("CancellingText", "Canceling...").ToString())
													]
											]

										+ SOverlay::Slot()
											[
												SNew(SHorizontalBox)
													.Visibility(this, &SSessionLauncher::HandleDoneButtonVisibility)

												+ SHorizontalBox::Slot()
													.AutoWidth()
													.Padding(4.0f, 0.0f, 0.0f, 0.0f)
													[
														// re-run button
														SNew(SButton)
															.ContentPadding(FMargin(6.0f, 2.0f))
															.OnClicked(this, &SSessionLauncher::HandleFinishButtonClicked)
															.ToolTipText(LOCTEXT("RerunButtonTooltip", "Execute the selected profile one more time.").ToString())
															[
																SNew(STextBlock)
																	.Text(LOCTEXT("RerunButtonLabel", "Run Again").ToString())
															]								
													]

												+ SHorizontalBox::Slot()
													.AutoWidth()
													.Padding(4.0f, 0.0f, 0.0f, 0.0f)
													[
														// done button
														SNew(SButton)
															.ContentPadding(FMargin(6.0f, 2.0f))
															.OnClicked(this, &SSessionLauncher::HandleDoneButtonClicked)
															.ToolTipText(LOCTEXT("DoneButtonTooltip", "Close this page.").ToString())
															[
																SNew(STextBlock)
																	.Text(LOCTEXT("DoneButtonLabel", "Done").ToString())
															]
													]
											]
									]
							]
					]
			]
	];

	ConstructUnderMajorTab->SetOnPersistVisualState(SDockTab::FOnPersistVisualState::CreateRaw(this, &SSessionLauncher::HandleMajorTabPersistVisualState));

	Model->OnProfileSelected().AddSP(this, &SSessionLauncher::HandleProfileManagerProfileSelected);
	ShowWidget(ESessionLauncherTasks::QuickLaunch);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncher implementation
*****************************************************************************/

void SSessionLauncher::LaunchSelectedProfile( )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		LauncherWorker = Model->GetLauncher()->Launch(Model->GetDeviceProxyManager(), SelectedProfile.ToSharedRef());

		if (LauncherWorker.IsValid())
		{
			ProgressPanel->SetLauncherWorker(LauncherWorker.ToSharedRef());
		}
	}
}


/* SSessionLauncher callbacks
*****************************************************************************/

FReply SSessionLauncher::HandleCancelButtonClicked( )
{
	if (LauncherWorker.IsValid())
	{
		LauncherWorker->Cancel();
	}

	return FReply::Handled();
}


EVisibility SSessionLauncher::HandleCancelButtonVisibility( ) const
{
	if (LauncherWorker.IsValid())
	{
		if (LauncherWorker->GetStatus() == ELauncherWorkerStatus::Busy)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncher::HandleCancelingThrobberVisibility( ) const
{
	if (LauncherWorker.IsValid())
	{
		if (LauncherWorker->GetStatus() == ELauncherWorkerStatus::Canceling)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FReply SSessionLauncher::HandleDoneButtonClicked( )
{
	LauncherWorker = NULL;

	WidgetSwitcher->SetActiveWidgetIndex(CurrentTask);
	Toolbar->SetCurrentTask(CurrentTask);

	return FReply::Handled();
}


EVisibility SSessionLauncher::HandleDoneButtonVisibility( ) const
{
	if (LauncherWorker.IsValid())
	{
		if ((LauncherWorker->GetStatus() == ELauncherWorkerStatus::Canceled) ||
			(LauncherWorker->GetStatus() == ELauncherWorkerStatus::Completed))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FReply SSessionLauncher::HandleEditButtonClicked( )
{
	WidgetSwitcher->SetActiveWidgetIndex(CurrentTask);
	Toolbar->SetCurrentTask(CurrentTask);

	return FReply::Handled();
}


EVisibility SSessionLauncher::HandleEditButtonVisibility( ) const
{
	if (WidgetSwitcher->GetActiveWidgetIndex() == ESessionLauncherTasks::Preview)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


FReply SSessionLauncher::HandleFinishButtonClicked( )
{
	WidgetSwitcher->SetActiveWidgetIndex(ESessionLauncherTasks::Progress);
	Toolbar->SetCurrentTask(ESessionLauncherTasks::Progress);

	LaunchSelectedProfile();

	return FReply::Handled();
}


bool SSessionLauncher::HandleFinishButtonIsEnabled( ) const
{
	if (WidgetSwitcher->GetActiveWidgetIndex() < ESessionLauncherTasks::Progress)
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid() && SelectedProfile->IsValidForLaunch())
		{
			return true;
		}
	}

	return false;
}


EVisibility SSessionLauncher::HandleFinishButtonVisibility( ) const
{
	if (WidgetSwitcher->GetActiveWidgetIndex() < ESessionLauncherTasks::Progress)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSessionLauncher::HandleMajorTabPersistVisualState( )
{
	Model->GetProfileManager()->SaveSettings();
}


FReply SSessionLauncher::HandlePreviewButtonClicked( )
{
	WidgetSwitcher->SetActiveWidgetIndex(ESessionLauncherTasks::Preview);
	Toolbar->SetCurrentTask(ESessionLauncherTasks::Preview);

	return FReply::Handled();
}


EVisibility SSessionLauncher::HandlePreviewButtonVisibility( ) const
{
	if (WidgetSwitcher->GetActiveWidgetIndex() < ESessionLauncherTasks::Preview)
	{
		if (Model->GetSelectedProfile().IsValid())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


void SSessionLauncher::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{
	if (!SelectedProfile.IsValid())
	{
		WidgetSwitcher->SetActiveWidgetIndex(ESessionLauncherTasks::Advanced);
		Toolbar->SetCurrentTask(ESessionLauncherTasks::Advanced);
	}
}


bool SSessionLauncher::HandleProfileSelectorIsEnabled( ) const
{
	return (WidgetSwitcher->GetActiveWidgetIndex() < ESessionLauncherTasks::Progress);
}


void SSessionLauncher::ShowWidget( int32 Index )
{
	if ((CurrentTask < ESessionLauncherTasks::Advanced) && (CurrentTask != 0))
	{
		if (CurrentTask == ESessionLauncherTasks::QuickLaunch)
		{
			ILauncherDeviceGroupPtr DevGroup = Model->GetSelectedProfile()->GetDeployedDeviceGroup();
			Model->GetSelectedProfile()->SetDeployedDeviceGroup(NULL);
			Model->GetProfileManager()->RemoveDeviceGroup(DevGroup.ToSharedRef());
		}

		Model->DeleteSelectedProfile();
	}

	if (Index < ESessionLauncherTasks::Advanced)
	{
		Model->GetProfileManager()->AddNewProfile();

		if (Index == ESessionLauncherTasks::QuickLaunch)
		{
			Model->GetSelectedProfile()->SetBuildGame(FParse::Param( FCommandLine::Get(), TEXT("development")));
			Model->GetSelectedProfile()->SetCookMode(ELauncherProfileCookModes::OnTheFly);
			Model->GetSelectedProfile()->SetBuildConfiguration(EBuildConfigurations::Development);
			Model->GetSelectedProfile()->SetDeploymentMode(ELauncherProfileDeploymentModes::FileServer);
			ILauncherDeviceGroupPtr NewGroup = Model->GetProfileManager()->AddNewDeviceGroup();
			Model->GetSelectedProfile()->SetDeployedDeviceGroup(NewGroup);
			Model->GetSelectedProfile()->SetLaunchMode(ELauncherProfileLaunchModes::DefaultRole);
		}
		else if (Index == ESessionLauncherTasks::DeployRepository)
		{
			Model->GetSelectedProfile()->SetCookMode(ELauncherProfileCookModes::DoNotCook);
			Model->GetSelectedProfile()->SetDeploymentMode(ELauncherProfileDeploymentModes::CopyRepository);
			ILauncherDeviceGroupPtr NewGroup = Model->GetProfileManager()->AddNewDeviceGroup();
			Model->GetSelectedProfile()->SetDeployedDeviceGroup(NewGroup);
			Model->GetSelectedProfile()->SetLaunchMode(ELauncherProfileLaunchModes::DefaultRole);
		}
	}

	WidgetSwitcher->SetActiveWidgetIndex(Index);
	CurrentTask = (ESessionLauncherTasks::Type)Index;
	Toolbar->SetCurrentTask(CurrentTask);
}


#undef LOCTEXT_NAMESPACE
