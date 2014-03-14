// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCookByTheBookSettings.cpp: Implements the SSessionLauncherCookByTheBookSettings class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherCookByTheBookSettings"


/* SSessionLauncherCookByTheBookSettings structors
 *****************************************************************************/

SSessionLauncherCookByTheBookSettings::~SSessionLauncherCookByTheBookSettings( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherCookByTheBookSettings interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherCookByTheBookSettings::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InShowSimple )
{
	Model = InModel;

	ChildSlot
	[
		InShowSimple ? MakeSimpleWidget() : MakeComplexWidget()
	];

	Model->OnProfileSelected().AddSP(this, &SSessionLauncherCookByTheBookSettings::HandleProfileManagerProfileSelected);

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();
	if (SelectedProfile.IsValid())
	{
		SelectedProfile->OnProjectChanged().AddSP(this, &SSessionLauncherCookByTheBookSettings::HandleProfileProjectChanged);
	}

	ShowMapsChoice = EShowMapsChoices::ShowAllMaps;

	RefreshMapList();
	RefreshCultureList();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionLauncherCookByTheBookSettings implementation
 *****************************************************************************/

TSharedRef<SWidget> SSessionLauncherCookByTheBookSettings::MakeComplexWidget()
{
	TSharedRef<SWidget> Widget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(320.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoCookedPlatformSelectedError", "At least one Platform must be selected when cooking by the book."))
					.ErrorVisibility(this, &SSessionLauncherCookByTheBookSettings::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoPlatformSelected)
					.LabelText(LOCTEXT("CookedPlatformsLabel", "Cooked Platforms:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherCookedPlatforms, Model.ToSharedRef())
					]
			]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(256.0f)
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoCookedCulturesSelectedError", "At least one Culture must be selected when cooking by the book."))
					.ErrorVisibility(this, &SSessionLauncherCookByTheBookSettings::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoCookedCulturesSelected)
					.LabelText(LOCTEXT("CookedCulturesLabel", "Cooked Cultures:"))
				]

				+ SVerticalBox::Slot()
					.FillHeight(1.0)
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						// culture menu
						SAssignNew(CultureListView, SListView<TSharedPtr<FString> >)
						.HeaderRow
						(
						SNew(SHeaderRow)
						.Visibility(EVisibility::Collapsed)

						+ SHeaderRow::Column("Culture")
						.DefaultLabel(LOCTEXT("CultureListMapNameColumnHeader", "Culture"))
						.FillWidth(1.0f)
						)
						.ItemHeight(16.0f)
						.ListItemsSource(&CultureList)
						.OnGenerateRow(this, &SSessionLauncherCookByTheBookSettings::HandleCultureListViewGenerateRow)
						.SelectionMode(ESelectionMode::None)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 4.0f)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SelectLabel", "Select:"))
						]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(8.0f, 0.0f)
							[
								// all cultures hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkNavigate, true)
								.Text(LOCTEXT("AllPlatformsHyperlinkLabel", "All"))
								.ToolTipText(LOCTEXT("AllPlatformsButtonTooltip", "Select all available platforms."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkVisibility)									
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								// no cultures hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkNavigate, false)
								.Text(LOCTEXT("NoCulturesHyperlinkLabel", "None"))
								.ToolTipText(LOCTEXT("NoCulturesHyperlinkTooltip", "Deselect all platforms."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkVisibility)									
							]
					]
			]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(256.0f)
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.LabelText(LOCTEXT("CookedMapsLabel", "Cooked Maps:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// all maps radio button
							SNew(SCheckBox)
							.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxIsChecked, EShowMapsChoices::ShowAllMaps)
							.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxCheckStateChanged, EShowMapsChoices::ShowAllMaps)
							.Style(FEditorStyle::Get(), "RadioButton")
							[
								SNew(STextBlock)
								.Text(LOCTEXT("AllMapsCheckBoxText", "Show all"))
							]
						]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								// cooked maps radio button
								SNew(SCheckBox)
								.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxIsChecked, EShowMapsChoices::ShowCookedMaps)
								.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxCheckStateChanged, EShowMapsChoices::ShowCookedMaps)
								.Style(FEditorStyle::Get(), "RadioButton")
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CookedMapsCheckBoxText", "Show cooked"))
								]
							]
					]

				+ SVerticalBox::Slot()
					.FillHeight(1.0)
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// map list
						SAssignNew(MapListView, SListView<TSharedPtr<FString> >)
						.HeaderRow
						(
						SNew(SHeaderRow)
						.Visibility(EVisibility::Collapsed)

						+ SHeaderRow::Column("MapName")
						.DefaultLabel(LOCTEXT("MapListMapNameColumnHeader", "Map"))
						.FillWidth(1.0f)
						)
						.ItemHeight(16.0f)
						.ListItemsSource(&MapList)
						.OnGenerateRow(this, &SSessionLauncherCookByTheBookSettings::HandleMapListViewGenerateRow)
						.SelectionMode(ESelectionMode::None)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleNoMapSelectedBoxVisibility)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush(TEXT("Icons.Warning")))
						]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(4.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SSessionLauncherCookByTheBookSettings::HandleNoMapsTextBlockText)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 4.0f)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SelectLabel", "Select:"))
							.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)
						]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(8.0f, 0.0f)
							[
								// all maps hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllMapsHyperlinkNavigate, true)
								.Text(LOCTEXT("AllMapsHyperlinkLabel", "All"))
								.ToolTipText(LOCTEXT("AllMapsHyperlinkTooltip", "Select all available maps."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)									
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								// no maps hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllMapsHyperlinkNavigate, false)
								.Text(LOCTEXT("NoMapsHyperlinkLabel", "None"))
								.ToolTipText(LOCTEXT("NoMapsHyperlinkTooltip", "Deselect all maps."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)
							]
					]
			]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced Settings"))
			.InitiallyCollapsed(true)
			.Padding(8.0f)
			.BodyContent()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					// incremental cook check box
					SNew(SCheckBox)
					.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxIsChecked)
					.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxCheckStateChanged)
					.Padding(FMargin(4.0f, 0.0f))
					.ToolTipText(LOCTEXT("IncrementalCheckBoxTooltip", "If checked, only modified content will be cooked, resulting in much faster cooking times. It is recommended to enable this option whenever possible."))
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("IncrementalCheckBoxText", "Only cook modified content"))
					]
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// incremental cook check box
						SNew(SCheckBox)
						.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxIsChecked)
						.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxCheckStateChanged)
						.Padding(FMargin(4.0f, 0.0f))
						.ToolTipText(LOCTEXT("UnversionedCheckBoxTooltip", "If checked, the version is assumed to be current at load. This is potentially dangerous, but results in smaller patch sizes."))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("UnversionedCheckBoxText", "Save packages without versions"))
						]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("CookConfigurationSelectorLabel", "Cooker build configuration:"))							
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// cooker build configuration selector
						SNew(SSessionLauncherBuildConfigurationSelector)
						.OnConfigurationSelected(this, &SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorConfigurationSelected)
						.Text(this, &SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorText)
						.ToolTipText(LOCTEXT("CookConfigurationToolTipText", "Sets the build configuration to use for the cooker commandlet."))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("CookerOptionsTextBoxLabel", "Additional Cooker Options:"))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// cooker command line options
						SNew(SEditableTextBox)
						.ToolTipText(LOCTEXT("CookerOptionsTextBoxTooltip", "Additional cooker command line parameters can be specified here."))
					]
			]
		];

	return Widget;
}

TSharedRef<SWidget> SSessionLauncherCookByTheBookSettings::MakeSimpleWidget()
{
	TSharedRef<SWidget> Widget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(256.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.ErrorToolTipText(NSLOCTEXT("SSessionLauncherBuildValidation", "NoCookedPlatformSelectedError", "At least one Platform must be selected when cooking by the book."))
					.ErrorVisibility(this, &SSessionLauncherCookByTheBookSettings::HandleValidationErrorIconVisibility, ELauncherProfileValidationErrors::NoPlatformSelected)
					.LabelText(LOCTEXT("CookedPlatformsLabel", "Cooked Platforms:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherCookedPlatforms, Model.ToSharedRef())
					]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(256.0f)
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(8.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSessionLauncherFormLabel)
					.LabelText(LOCTEXT("CookedMapsLabel", "Cooked Maps:"))
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							// all maps radio button
							SNew(SCheckBox)
							.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxIsChecked, EShowMapsChoices::ShowAllMaps)
							.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxCheckStateChanged, EShowMapsChoices::ShowAllMaps)
							.Style(FEditorStyle::Get(), "RadioButton")
							[
								SNew(STextBlock)
								.Text(LOCTEXT("AllMapsCheckBoxText", "Show all"))
							]
						]

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.Padding(8.0f, 0.0f, 0.0f, 0.0f)
							[
								// cooked maps radio button
								SNew(SCheckBox)
								.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxIsChecked, EShowMapsChoices::ShowCookedMaps)
								.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxCheckStateChanged, EShowMapsChoices::ShowCookedMaps)
								.Style(FEditorStyle::Get(), "RadioButton")
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CookedMapsCheckBoxText", "Show cooked"))
								]
							]
					]

				+ SVerticalBox::Slot()
					.FillHeight(1.0)
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// map list
						SAssignNew(MapListView, SListView<TSharedPtr<FString> >)
						.HeaderRow
						(
						SNew(SHeaderRow)
						.Visibility(EVisibility::Collapsed)

						+ SHeaderRow::Column("MapName")
						.DefaultLabel(LOCTEXT("MapListMapNameColumnHeader", "Map"))
						.FillWidth(1.0f)
						)
						.ItemHeight(16.0f)
						.ListItemsSource(&MapList)
						.OnGenerateRow(this, &SSessionLauncherCookByTheBookSettings::HandleMapListViewGenerateRow)
						.SelectionMode(ESelectionMode::None)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleNoMapSelectedBoxVisibility)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush(TEXT("Icons.Warning")))
						]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(4.0f, 0.0f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SSessionLauncherCookByTheBookSettings::HandleNoMapsTextBlockText)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 4.0f)
					[
						SNew(SSeparator)
						.Orientation(Orient_Horizontal)
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SelectLabel", "Select:"))
							.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)
						]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(8.0f, 0.0f)
							[
								// all maps hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllMapsHyperlinkNavigate, true)
								.Text(LOCTEXT("AllMapsHyperlinkLabel", "All"))
								.ToolTipText(LOCTEXT("AllMapsHyperlinkTooltip", "Select all available maps."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)									
							]

						+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								// no maps hyper link
								SNew(SHyperlink)
								.OnNavigate(this, &SSessionLauncherCookByTheBookSettings::HandleAllMapsHyperlinkNavigate, false)
								.Text(LOCTEXT("NoMapsHyperlinkLabel", "None"))
								.ToolTipText(LOCTEXT("NoMapsHyperlinkTooltip", "Deselect all maps."))
								.Visibility(this, &SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility)
							]
					]
			]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("AdvancedAreaTitle", "Advanced Settings"))
			.InitiallyCollapsed(true)
			.Padding(8.0f)
			.BodyContent()
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					// incremental cook check box
					SNew(SCheckBox)
					.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxIsChecked)
					.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxCheckStateChanged)
					.Padding(FMargin(4.0f, 0.0f))
					.ToolTipText(LOCTEXT("IncrementalCheckBoxTooltip", "If checked, only modified content will be cooked, resulting in much faster cooking times. It is recommended to enable this option whenever possible."))
					.Content()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("IncrementalCheckBoxText", "Only cook modified content"))
					]
				]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// incremental cook check box
						SNew(SCheckBox)
						.IsChecked(this, &SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxIsChecked)
						.OnCheckStateChanged(this, &SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxCheckStateChanged)
						.Padding(FMargin(4.0f, 0.0f))
						.ToolTipText(LOCTEXT("UnversionedCheckBoxTooltip", "If checked, the version is assumed to be current at load. This is potentially dangerous, but results in smaller patch sizes."))
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("UnversionedCheckBoxText", "Save packages without versions"))
						]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("CookConfigurationSelectorLabel", "Cooker build configuration:"))							
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// cooker build configuration selector
						SNew(SSessionLauncherBuildConfigurationSelector)
						.OnConfigurationSelected(this, &SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorConfigurationSelected)
						.Text(this, &SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorText)
						.ToolTipText(LOCTEXT("CookConfigurationToolTipText", "Sets the build configuration to use for the cooker commandlet."))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(SSessionLauncherFormLabel)
						.LabelText(LOCTEXT("CookerOptionsTextBoxLabel", "Additional Cooker Options:"))
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// cooker command line options
						SNew(SEditableTextBox)
						.ToolTipText(LOCTEXT("CookerOptionsTextBoxTooltip", "Additional cooker command line parameters can be specified here."))
					]
			]
		];

	return Widget;
}

void SSessionLauncherCookByTheBookSettings::RefreshCultureList()
{
	CultureList.Reset();

	TArray<FString> CultureNames;
	FInternationalization::GetCultureNames(CultureNames);

	if (CultureNames.Num() > 0)
	{
		for (int32 Index = 0; Index < CultureNames.Num(); ++Index)
		{
			FString CultureName = CultureNames[Index];
			CultureList.Add(MakeShareable(new FString(CultureName)));
		}
	}

	if (CultureListView.IsValid())
	{
		CultureListView->RequestListRefresh();
	}
}

void SSessionLauncherCookByTheBookSettings::RefreshMapList( )
{
	MapList.Reset();

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		TArray<FString> AvailableMaps = FGameProjectHelper::GetAvailableMaps(SelectedProfile->GetProjectBasePath(), SelectedProfile->SupportsEngineMaps(), true);

		for (int32 AvailableMapIndex = 0; AvailableMapIndex < AvailableMaps.Num(); ++AvailableMapIndex)
		{
			FString& Map = AvailableMaps[AvailableMapIndex];

			if ((ShowMapsChoice == EShowMapsChoices::ShowAllMaps) || SelectedProfile->GetCookedMaps().Contains(Map))
			{
				MapList.Add(MakeShareable(new FString(Map)));
			}
		}
	}

	MapListView->RequestListRefresh();
}


/* SSessionLauncherCookByTheBookSettings callbacks
 *****************************************************************************/

void SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkNavigate( bool AllPlatforms )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (AllPlatforms)
		{
			TArray<FString> CultureNames;
			FInternationalization::GetCultureNames(CultureNames);

			for (int32 ExtensionIndex = 0; ExtensionIndex < CultureNames.Num(); ++ExtensionIndex)
			{
				SelectedProfile->AddCookedCulture(CultureNames[ExtensionIndex]);
			}
		}
		else
		{
			SelectedProfile->ClearCookedCultures();
		}
	}
}


EVisibility SSessionLauncherCookByTheBookSettings::HandleAllCulturesHyperlinkVisibility( ) const
{
	TArray<FString> CultureNames;
	FInternationalization::GetCultureNames(CultureNames);

	if (CultureNames.Num() > 1)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherCookByTheBookSettings::HandleAllMapsHyperlinkNavigate( bool AllPlatforms )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (AllPlatforms)
		{
			TArray<FString> AvailableMaps = FGameProjectHelper::GetAvailableMaps(SelectedProfile->GetProjectBasePath(), SelectedProfile->SupportsEngineMaps(), false);

			for (int32 AvailableMapIndex = 0; AvailableMapIndex < AvailableMaps.Num(); ++AvailableMapIndex)
			{
				SelectedProfile->AddCookedMap(AvailableMaps[AvailableMapIndex]);
			}
		}
		else
		{
			SelectedProfile->ClearCookedMaps();
		}
	}
}


EVisibility SSessionLauncherCookByTheBookSettings::HandleMapSelectionHyperlinkVisibility( ) const
{
	if (MapList.Num() > 1)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorConfigurationSelected( EBuildConfigurations::Type Configuration )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetCookConfiguration(Configuration);
	}
}


FString SSessionLauncherCookByTheBookSettings::HandleCookConfigurationSelectorText( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		return EBuildConfigurations::ToString(SelectedProfile->GetCookConfiguration());
	}

	return TEXT("");
}


void SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetIncrementalCooking(NewState == ESlateCheckBoxState::Checked);
	}
}


ESlateCheckBoxState::Type SSessionLauncherCookByTheBookSettings::HandleIncrementalCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsCookingIncrementally())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


TSharedRef<ITableRow> SSessionLauncherCookByTheBookSettings::HandleMapListViewGenerateRow( TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SSessionLauncherMapListRow, Model.ToSharedRef())
		.MapName(InItem)
		.OwnerTableView(OwnerTable);
}


TSharedRef<ITableRow> SSessionLauncherCookByTheBookSettings::HandleCultureListViewGenerateRow( TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(SSessionLauncherCultureListRow, Model.ToSharedRef())
		.CultureName(InItem)
		.OwnerTableView(OwnerTable);
}


EVisibility SSessionLauncherCookByTheBookSettings::HandleNoMapSelectedBoxVisibility( ) const
{
	if (MapList.Num() == 0)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


FText SSessionLauncherCookByTheBookSettings::HandleNoMapsTextBlockText( ) const
{
	if (MapList.Num() == 0)
	{
		if (ShowMapsChoice == EShowMapsChoices::ShowAllMaps)
		{
			return LOCTEXT("NoMapsFoundText", "No available maps were found.");
		}
		else if (ShowMapsChoice == EShowMapsChoices::ShowCookedMaps)
		{
			return LOCTEXT("NoMapsSelectedText", "No map selected. Only startup packages will be cooked!");
		}
	}

	return FText();
}


void SSessionLauncherCookByTheBookSettings::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{
	if (PreviousProfile.IsValid())
	{
		PreviousProfile->OnProjectChanged().RemoveAll(this);
	}
	if (SelectedProfile.IsValid())
	{
		SelectedProfile->OnProjectChanged().AddSP(this, &SSessionLauncherCookByTheBookSettings::HandleProfileProjectChanged);
	}
	RefreshMapList();
	RefreshCultureList();
}

void SSessionLauncherCookByTheBookSettings::HandleProfileProjectChanged()
{
	RefreshMapList();
	RefreshCultureList();
}

ESlateCheckBoxState::Type SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxIsChecked( EShowMapsChoices::Type Choice ) const
{
	if (ShowMapsChoice == Choice)
	{
		return ESlateCheckBoxState::Checked;
	}

	return ESlateCheckBoxState::Unchecked;
}


void SSessionLauncherCookByTheBookSettings::HandleShowCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState, EShowMapsChoices::Type Choice )
{
	if (NewState == ESlateCheckBoxState::Checked)
	{
		ShowMapsChoice = Choice;
		RefreshMapList();
	}
}


void SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		SelectedProfile->SetUnversionedCooking((NewState == ESlateCheckBoxState::Checked));
	}
}


ESlateCheckBoxState::Type SSessionLauncherCookByTheBookSettings::HandleUnversionedCheckBoxIsChecked( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->IsCookingUnversioned())
		{
			return ESlateCheckBoxState::Checked;
		}
	}

	return ESlateCheckBoxState::Unchecked;
}


EVisibility SSessionLauncherCookByTheBookSettings::HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->HasValidationError(Error))
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Hidden;
}


#undef LOCTEXT_NAMESPACE
