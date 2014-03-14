// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceBrowserFilterBar.cpp: Implements the SDeviceBrowserFilterBar class.
=============================================================================*/

#include "DeviceManagerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SDeviceBrowserFilterBar"


/* SSessionBrowserFilterBar structors
 *****************************************************************************/

SDeviceBrowserFilterBar::~SDeviceBrowserFilterBar( )
{
	if (Filter.IsValid())
	{
		Filter->OnFilterReset().RemoveAll(this);
	}	
}


/* SSessionBrowserFilterBar interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDeviceBrowserFilterBar::Construct( const FArguments& InArgs, FDeviceBrowserFilterRef InFilter )
{
	Filter = InFilter;

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Top)
			[
				// search box
				SAssignNew(FilterStringTextBox, SSearchBox)
					.HintText(LOCTEXT("SearchBoxHint", "Search devices"))
					.OnTextChanged(this, &SDeviceBrowserFilterBar::HandleFilterStringTextChanged)
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				// platform filter
				SNew(SComboButton)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("PlatformComboButtonText", "Platforms").ToString())
					]
					.ContentPadding(FMargin(6.0f, 2.0f))
					.MenuContent()
					[
						SAssignNew(PlatformListView, SListView<TSharedPtr<FString> >)
							.ItemHeight(24.0f)
							.ListItemsSource(&Filter->GetFilteredPlatforms())
							.OnGenerateRow(this, &SDeviceBrowserFilterBar::HandlePlatformListViewGenerateRow)
					]
			]
	];

	Filter->OnFilterReset().AddSP(this, &SDeviceBrowserFilterBar::HandleFilterReset);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SSessionBrowserFilterBar callbacks
 *****************************************************************************/

void SDeviceBrowserFilterBar::HandleFilterReset( )
{
	FilterStringTextBox->SetText(Filter->GetDeviceSearchText());
	PlatformListView->RequestListRefresh();
}


void SDeviceBrowserFilterBar::HandleFilterStringTextChanged( const FText& NewText )
{
	Filter->SetDeviceSearchString(NewText);
}


void SDeviceBrowserFilterBar::HandlePlatformListRowCheckStateChanged( ESlateCheckBoxState::Type CheckState, TSharedPtr<FString> PlatformName )
{
	Filter->SetPlatformEnabled(*PlatformName, CheckState == ESlateCheckBoxState::Checked);
}


ESlateCheckBoxState::Type SDeviceBrowserFilterBar::HandlePlatformListRowIsChecked( TSharedPtr<FString> PlatformName ) const
{
	if (Filter->IsPlatformEnabled(*PlatformName))
	{
		return ESlateCheckBoxState::Checked;
	}

	return ESlateCheckBoxState::Unchecked;
}


FString SDeviceBrowserFilterBar::HandlePlatformListRowText( TSharedPtr<FString> PlatformName ) const
{
	return FString::Printf(TEXT("%s (%i)"), **PlatformName, Filter->GetServiceCountPerPlatform(*PlatformName));
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<ITableRow> SDeviceBrowserFilterBar::HandlePlatformListViewGenerateRow( TSharedPtr<FString> PlatformName, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString> >, OwnerTable)
		.Content()
		[
			SNew(SCheckBox)
				.IsChecked(this, &SDeviceBrowserFilterBar::HandlePlatformListRowIsChecked, PlatformName)
				.Padding(FMargin(6.0, 2.0))
				.OnCheckStateChanged(this, &SDeviceBrowserFilterBar::HandlePlatformListRowCheckStateChanged, PlatformName)
				.Content()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
								.Image(FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), **PlatformName)))
						]

					+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(4.0f, 0.0f, 0.0f, 0.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(this, &SDeviceBrowserFilterBar::HandlePlatformListRowText, PlatformName)
						]
				]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


#undef LOCTEXT_NAMESPACE
