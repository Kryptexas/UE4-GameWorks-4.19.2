// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceDetails.cpp: Implements the SDeviceDetails class.
=============================================================================*/

#include "DeviceManagerPrivatePCH.h"
//#include "PropertyEditing.h"


#define LOCTEXT_NAMESPACE "SDeviceDetails"


/* SMessagingEndpoints structors
*****************************************************************************/

SDeviceDetails::~SDeviceDetails( )
{
	if (Model.IsValid())
	{
		Model->OnSelectedDeviceServiceChanged().RemoveAll(this);
	}
}


/* SDeviceDetails interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SDeviceDetails::Construct( const FArguments& InArgs, const FDeviceManagerModelRef& InModel )
{
	Model = InModel;
/*
	// initialize details view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bObjectsUseNameArea = false;
		DetailsViewArgs.bSearchInitialKeyFocus = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
	}

	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
*/
	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
					.IsEnabled(this, &SDeviceDetails::HandleDetailsIsEnabled)
					.Visibility(this, &SDeviceDetails::HandleDetailsVisibility)

				// details
				+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						//DetailsView.ToSharedRef()
						SNullWidget::NullWidget
					]
			]

		+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("NotificationList.ItemBackground"))
					.Padding(8.0f)
					.Visibility(this, &SDeviceDetails::HandleSelectDeviceOverlayVisibility)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("SelectSessionOverlayText", "Please select a device from the Device Browser").ToString())
					]
			]
	];

	Model->OnSelectedDeviceServiceChanged().AddRaw(this, &SDeviceDetails::HandleModelSelectedDeviceServiceChanged);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SDeviceDetails callbacks
 *****************************************************************************/

bool SDeviceDetails::HandleDetailsIsEnabled( ) const
{
	ITargetDeviceServicePtr DeviceService = Model->GetSelectedDeviceService();

	return (DeviceService.IsValid() && DeviceService->GetDevice().IsValid());
}


EVisibility SDeviceDetails::HandleDetailsVisibility( ) const
{
	if (Model->GetSelectedDeviceService().IsValid())
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}


EVisibility SDeviceDetails::HandleSelectDeviceOverlayVisibility( ) const
{
	if (Model->GetSelectedDeviceService().IsValid())
	{
		return EVisibility::Hidden;
	}

	return EVisibility::Visible;
}


void SDeviceDetails::HandleModelSelectedDeviceServiceChanged( )
{
}


#undef LOCTEXT_NAMESPACE
