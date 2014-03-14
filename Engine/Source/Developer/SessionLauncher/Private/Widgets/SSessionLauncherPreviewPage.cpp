// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPreviewPage.cpp: Implements the SSessionLauncherPreviewPage class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherPreviewPage"


/* SSessionLauncherPreviewPage interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherPreviewPage::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNew(SScrollBox)

		+ SScrollBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SGridPanel)
					.FillColumn(1, 1.0f)

				// build section
				+ SGridPanel::Slot(0, 0)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("BuildSectionHeader", "Build").ToString())
					]

				+ SGridPanel::Slot(1, 0)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ProjectLabel", "Project:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// selected project
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleProjectTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoProjectSelected)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("ConfigurationLabel", "Build Configuration:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// selected build configuration
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleBuildConfigurationTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoBuildConfigurationSelected)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("PlatformsLabel", "Platforms:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// build platforms
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleBuildPlatformsTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoPlatformSelected)
											]
									]
							]
					]

				// cook section
				+ SGridPanel::Slot(0, 1)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
							.Orientation(Orient_Horizontal)
					]

				+ SGridPanel::Slot(0, 2)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("CookSectionHeader", "Cook").ToString())
					]

				+ SGridPanel::Slot(1, 2)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleCookSummaryBoxVisibility, ELauncherProfileCookModes::DoNotCook)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("CookModeLabel", "Cook Mode:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// do not cook
												SNew(STextBlock)
													.Text(LOCTEXT("DoNotCookLabel", "Do not cook").ToString())
											]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleCookSummaryBoxVisibility, ELauncherProfileCookModes::OnTheFly)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("BuildModeLabel", "Cook Mode:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cook on the fly
												SNew(STextBlock)
													.Text(LOCTEXT("CookOnTheFlyLabel", "On the fly").ToString())
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("CookerOptionsLabel", "Cooker Options:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cooker options
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleCookerOptionsTextBlockText)
											]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleCookSummaryBoxVisibility, ELauncherProfileCookModes::ByTheBook)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("BuildModeLabel", "Cook Mode:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cook by the book
												SNew(STextBlock)
													.Text(LOCTEXT("CookByTheBookLabel", "By the book").ToString())
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("CulturesBuildLabel", "Cooked Cultures:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cooked cultures
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleCookedCulturesTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoCookedCulturesSelected)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("MapsBuildLabel", "Cooked Maps:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cooked maps
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleCookedMapsTextBlockText)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("CookerOptionsLabel", "Cooker Options:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// cooker options
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleCookerOptionsTextBlockText)
											]
									]
							]
					]

/*
				// package section
				+ SGridPanel::Slot(0, 3)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
							.Orientation(Orient_Horizontal)
					]

				+ SGridPanel::Slot(0, 4)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("PackageSectionHeader", "Package").ToString())
					]

				+ SGridPanel::Slot(1, 4)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNullWidget::NullWidget
					]
*/

				// deploy section
				+ SGridPanel::Slot(0, 5)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
							.Orientation(Orient_Horizontal)
					]

				+ SGridPanel::Slot(0, 6)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("DeploySectionHeader", "Deploy").ToString())
					]

				+ SGridPanel::Slot(1, 6)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleDeploySummaryBoxVisibility, ELauncherProfileDeploymentModes::DoNotDeploy)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("DeployModeLabel", "Deploy Mode:").ToString())
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// do not cook
												SNew(STextBlock)
													.Text(LOCTEXT("DoNotDeployLabel", "Do not deploy").ToString())
											]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)
											.Visibility(this, &SSessionLauncherPreviewPage::HandleDeploySummaryBoxVisibility, ELauncherProfileDeploymentModes::CopyToDevice)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("DeployModeLabel", "Deploy Mode:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// copy to device
												SNew(STextBlock)
													.Text(LOCTEXT("CopyToDeviceLabel", "Copy to device"))
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)
											.Visibility(this, &SSessionLauncherPreviewPage::HandleDeploySummaryBoxVisibility, ELauncherProfileDeploymentModes::FileServer)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("DeployModeLabel", "Deploy Mode:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// copy to device
												SNew(STextBlock)
													.Text(LOCTEXT("FileServerLabel", "File server"))
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SVerticalBox)
											.Visibility(this, &SSessionLauncherPreviewPage::HandleDeployDetailsBoxVisibility)

										+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(0.0f, 8.0f, 0.0f, 0.0f)
											[
												SNew(SHorizontalBox)

												+ SHorizontalBox::Slot()
													.AutoWidth()
													[
														SNew(STextBlock)
															.Text(LOCTEXT("DeviceGroupLabel", "Device Group:"))
													]

												+ SHorizontalBox::Slot()
													.FillWidth(1.0f)
													.HAlign(HAlign_Right)
													.Padding(8.0f, 0.0f, 0.0f, 0.0f)
													[
														// copy to device
														SNew(STextBlock)
															.Text(this, &SSessionLauncherPreviewPage::HandleSelectedDeviceGroupTextBlockText)
													]
											]

										+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(0.0f, 8.0f, 0.0f, 0.0f)
											[
												SNew(SHorizontalBox)

												+ SHorizontalBox::Slot()
													.AutoWidth()
													[
														SNew(STextBlock)
															.Text(LOCTEXT("DeviceListLabel", "Devices:"))
													]

												+ SHorizontalBox::Slot()
													.FillWidth(1.0f)
													.HAlign(HAlign_Right)
													.Padding(8.0f, 0.0f, 0.0f, 0.0f)
													[
														// device list
														SAssignNew(DeviceProxyListView, SListView<ITargetDeviceProxyPtr>)
															.ItemHeight(16.0f)
															.ListItemsSource(&DeviceProxyList)
															.SelectionMode(ESelectionMode::None)
															.OnGenerateRow(this, &SSessionLauncherPreviewPage::HandleDeviceProxyListViewGenerateRow)
															.HeaderRow
															(
																SNew(SHeaderRow)
																	.Visibility(EVisibility::Collapsed)

																+ SHeaderRow::Column("Device")
																	.DefaultLabel(LOCTEXT("DeviceListDeviceColumnHeader", "Device").ToString())
															)
													]
											]
									]
							]
					]

				// launch section
				+ SGridPanel::Slot(0, 7)
					.ColumnSpan(3)
					.Padding(0.0f, 16.0f)
					[
						SNew(SSeparator)
							.Orientation(Orient_Horizontal)
					]

				+ SGridPanel::Slot(0, 8)
					.Padding(8.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Top)
					[
						SNew(STextBlock)
							.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13))
							.Text(LOCTEXT("LaunchSectionHeader", "Launch"))
					]

				+ SGridPanel::Slot(1, 8)
					.Padding(32.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleLaunchSummaryBoxVisibility, ELauncherProfileLaunchModes::DoNotLaunch)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("LaunchModeLabel", "Launch Mode:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// do not cook
												SNew(STextBlock)
													.Text(LOCTEXT("DoNotLaunchLabel", "Do not launch"))
											]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleLaunchSummaryBoxVisibility, ELauncherProfileLaunchModes::DefaultRole)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("LaunchModeLabel", "Launch Mode:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// do not cook
												SNew(STextBlock)
													.Text(LOCTEXT("LaunchUsingDefaultRoleLabel", "Using default role"))
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("InitialCultureLabel", "Initial Culture:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// initial culture
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleInitialCultureTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::InitialCultureNotAvailable)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("InitialMapLabel", "Initial Map:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// initial map
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleInitialMapTextBlockText)
											]

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(SImage)
													.Image(FEditorStyle::GetBrush(TEXT("Icons.Error")))
													.Visibility(this, &SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::InitialMapNotAvailable)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("CommandLineLabel", "Command Line:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// command line
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleCommandLineTextBlockText)
											]
									]

								+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 8.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("VsyncLabel", "VSync:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0f)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// command line
												SNew(STextBlock)
													.Text(this, &SSessionLauncherPreviewPage::HandleLaunchVsyncTextBlockText)
											]
									]
							]

						+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SVerticalBox)
									.Visibility(this, &SSessionLauncherPreviewPage::HandleLaunchSummaryBoxVisibility, ELauncherProfileLaunchModes::CustomRoles)

								+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										+ SHorizontalBox::Slot()
											.AutoWidth()
											[
												SNew(STextBlock)
													.Text(LOCTEXT("LaunchModeLabel", "Launch Mode:"))
											]

										+ SHorizontalBox::Slot()
											.FillWidth(1.0)
											.HAlign(HAlign_Right)
											.Padding(8.0f, 0.0f, 0.0f, 0.0f)
											[
												// do not cook
												SNew(STextBlock)
													.Text(LOCTEXT("UseCustomRolesLabel", "Using custom roles"))
											]
									]
							]
					]
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SCompoundWidget overrides
 *****************************************************************************/

void SSessionLauncherPreviewPage::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	RefreshDeviceProxyList();
}


/* SSessionLauncherDevicesSelector implementation
 *****************************************************************************/

void SSessionLauncherPreviewPage::RefreshDeviceProxyList( )
{
	DeviceProxyList.Reset();

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const ILauncherDeviceGroupPtr& DeployedDeviceGroup = SelectedProfile->GetDeployedDeviceGroup();

		if (DeployedDeviceGroup.IsValid())
		{
			const TArray<FString>& Devices = DeployedDeviceGroup->GetDevices();

			for (int32 Index = 0; Index < Devices.Num(); ++Index)
			{
				DeviceProxyList.Add(Model->GetDeviceProxyManager()->FindOrAddProxy(Devices[Index]));
			}
		}
	}

	DeviceProxyListView->RequestListRefresh();
}


/* SSessionLauncherPreviewPage callbacks
 *****************************************************************************/

FString SSessionLauncherPreviewPage::HandleBuildConfigurationTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		return EBuildConfigurations::ToString(SelectedProfile->GetBuildConfiguration());
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


FString SSessionLauncherPreviewPage::HandleBuildPlatformsTextBlockText( ) const
{
	FString Result;

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const TArray<FString>& Platforms = SelectedProfile->GetCookedPlatforms();

		for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
		{
			Result += Platforms[PlatformIndex];

			if (PlatformIndex + 1 < Platforms.Num())
			{
				Result += LINE_TERMINATOR;
			}
		}

		if (Result.IsEmpty())
		{
			Result = LOCTEXT("NotSetText", "<not set>").ToString();
		}
	}

	return Result;
}


FString SSessionLauncherPreviewPage::HandleCommandLineTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const FString& CommandLine = SelectedProfile->GetDefaultLaunchRole()->GetCommandLine();

		if (CommandLine.IsEmpty())
		{
			return LOCTEXT("EmptyText", "<empty>").ToString();
		}

		return CommandLine;
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


FString SSessionLauncherPreviewPage::HandleCookedCulturesTextBlockText( ) const
{
	FString Result;

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const TArray<FString>& CookedCultures = SelectedProfile->GetCookedCultures();

		for (int32 CookedCultureIndex = 0; CookedCultureIndex < CookedCultures.Num(); ++CookedCultureIndex)
		{
			Result += CookedCultures[CookedCultureIndex];
			
			if (CookedCultureIndex + 1 < CookedCultures.Num())
			{
				Result += LINE_TERMINATOR;
			}
		}

		if (Result.IsEmpty())
		{
			Result = LOCTEXT("NotSetText", "<not set>").ToString();
		}
	}

	return Result;
}


FString SSessionLauncherPreviewPage::HandleCookedMapsTextBlockText( ) const
{
	FString Result;

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const TArray<FString>& CookedMaps = SelectedProfile->GetCookedMaps();

		for (int32 CookedMapIndex = 0; CookedMapIndex < CookedMaps.Num(); ++CookedMapIndex)
		{
			Result += CookedMaps[CookedMapIndex];

			if (CookedMapIndex + 1 < CookedMaps.Num())
			{
				Result += LINE_TERMINATOR;
			}
		}

		if (Result.IsEmpty())
		{
			Result = LOCTEXT("NoneText", "<none>").ToString();
		}
	}

	return Result;
}


FString SSessionLauncherPreviewPage::HandleCookerOptionsTextBlockText( ) const
{
	FString Result;

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetCookOptions().IsEmpty())
		{
			return LOCTEXT("NoneText", "<none>").ToString();
		}

		return SelectedProfile->GetCookOptions();
	}

	return Result;
}


EVisibility SSessionLauncherPreviewPage::HandleCookSummaryBoxVisibility( ELauncherProfileCookModes::Type CookMode ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetCookMode() == CookMode)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherPreviewPage::HandleDeployDetailsBoxVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() != ELauncherProfileDeploymentModes::DoNotDeploy)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherPreviewPage::HandleDeploySummaryBoxVisibility( ELauncherProfileDeploymentModes::Type DeploymentMode ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeploymentMode() == DeploymentMode)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


TSharedRef<ITableRow> SSessionLauncherPreviewPage::HandleDeviceProxyListViewGenerateRow( ITargetDeviceProxyPtr InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	check(Model->GetSelectedProfile().IsValid());

	ILauncherDeviceGroupPtr DeployedDeviceGroup = Model->GetSelectedProfile()->GetDeployedDeviceGroup();

	check(DeployedDeviceGroup.IsValid());

	return SNew(SSessionLauncherDeviceListRow, OwnerTable)
		.DeviceProxy(InItem);
}


FString SSessionLauncherPreviewPage::HandleInitialCultureTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const FString& InitialCulture = SelectedProfile->GetDefaultLaunchRole()->GetInitialCulture();

		if (InitialCulture.IsEmpty())
		{
			return LOCTEXT("DefaultText", "<default>").ToString();
		}

		return InitialCulture;
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


FString SSessionLauncherPreviewPage::HandleInitialMapTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		const FString& InitialMap = SelectedProfile->GetDefaultLaunchRole()->GetInitialMap();

		if (InitialMap.IsEmpty())
		{
			return LOCTEXT("DefaultText", "<default>").ToString();
		}

		return InitialMap;
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


EVisibility SSessionLauncherPreviewPage::HandleLaunchSummaryBoxVisibility( ELauncherProfileLaunchModes::Type LaunchMode ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetLaunchMode() == LaunchMode)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FString SSessionLauncherPreviewPage::HandleLaunchVsyncTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDefaultLaunchRole()->IsVsyncEnabled())
		{
			return LOCTEXT("YesText", "Yes").ToString();
		}

		return LOCTEXT("NoText", "No").ToString();
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


EVisibility SSessionLauncherPreviewPage::HandlePackageSummaryBoxVisibility( ELauncherProfilePackagingModes::Type PackagingMode ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetPackagingMode() == PackagingMode)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


FString SSessionLauncherPreviewPage::HandleProjectTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		FString GameName = SelectedProfile->GetProjectName();

		if (GameName.IsEmpty())
		{
			GameName = LOCTEXT("NotSetText", "<not set>").ToString();
		}

		return GameName;
	}

	return LOCTEXT("NotAvailableText", "n/a").ToString();
}


FString SSessionLauncherPreviewPage::HandleSelectedDeviceGroupTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ILauncherDeviceGroupPtr SelectedGroup = SelectedProfile->GetDeployedDeviceGroup();

		if (SelectedGroup.IsValid())
		{
			return SelectedGroup->GetName();
		}
	}
		
	return LOCTEXT("NoneText", "<none>").ToString();
}


FString SSessionLauncherPreviewPage::HandleSelectedProfileTextBlockText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		return SelectedProfile->GetName();
	}

	return LOCTEXT("NoneText", "<none>").ToString();
}


EVisibility SSessionLauncherPreviewPage::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->HasValidationError(Error))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
