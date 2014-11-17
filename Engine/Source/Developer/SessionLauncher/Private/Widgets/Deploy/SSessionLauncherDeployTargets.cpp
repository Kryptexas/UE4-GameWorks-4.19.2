// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployTargets.cpp: Implements the SSessionLauncherDeployTargets class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherDeployTargets"


/* SSessionLauncherDeployTargets structors
 *****************************************************************************/

SSessionLauncherDeployTargets::~SSessionLauncherDeployTargets( )
{
	if (Model.IsValid())
	{
		Model->GetDeviceProxyManager()->OnProxyAdded().RemoveAll(this);
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherDeployTargets interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherDeployTargets::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InIsAlwaysEnabled )
{
	Model = InModel;
	IsAlwaysEnabled = InIsAlwaysEnabled;

	TSharedPtr<SWidget> ComplexWidget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(DeviceGroupSelector, SSessionLauncherDeviceGroupSelector, InModel->GetProfileManager())
			.OnGroupSelected(this, &SSessionLauncherDeployTargets::HandleDeviceGroupSelectorGroupSelected)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 12.0f)
		[
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			.Visibility(this, &SSessionLauncherDeployTargets::HandleSelectDeviceGroupWarningVisibility)

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
					.Text(LOCTEXT("SelectDeviceGroupText", "Select or create a new device group to continue."))
				]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SVerticalBox)
			.Visibility(this, &SSessionLauncherDeployTargets::HandleDeviceSelectorVisibility)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.Visibility(IsAlwaysEnabled ? EVisibility::Hidden : EVisibility::Visible)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign((VAlign_Center))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowLabel", "Show:"))
				]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f)
					[
						// all devices radio button
						SNew(SCheckBox)
						.IsChecked(this, &SSessionLauncherDeployTargets::HandleShowCheckBoxIsChecked, EShowDevicesChoices::ShowAllDevices)
						.OnCheckStateChanged(this, &SSessionLauncherDeployTargets::HandleShowCheckBoxCheckStateChanged, EShowDevicesChoices::ShowAllDevices)
						.Style(FEditorStyle::Get(), "RadioButton")
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AllDevicesCheckBoxText", "All available devices"))
						]
					]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4.0f, 0.0f)
					[
						// group devices radio button
						SNew(SCheckBox)
						.IsChecked(this, &SSessionLauncherDeployTargets::HandleShowCheckBoxIsChecked, EShowDevicesChoices::ShowGroupDevices)
						.OnCheckStateChanged(this, &SSessionLauncherDeployTargets::HandleShowCheckBoxCheckStateChanged, EShowDevicesChoices::ShowGroupDevices)
						.Style(FEditorStyle::Get(), "RadioButton")
						[
							SNew(STextBlock)
							.Text(LOCTEXT("GroupDevicesCheckBoxText", "Devices in this group"))
						]
					]
			]

			+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.MaxHeight(288.0f)
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					// device list
					SAssignNew(DeviceProxyListView, SListView<ITargetDeviceProxyPtr>)
					.ItemHeight(16.0f)
					.HeaderRow
					(
					SNew(SHeaderRow)

					+ SHeaderRow::Column("CheckBox")
					.DefaultLabel(LOCTEXT("DeviceListCheckboxColumnHeader", " "))
					.FixedWidth(24.0f)

					+ SHeaderRow::Column("Device")
					.DefaultLabel(LOCTEXT("DeviceListDeviceColumnHeader", "Device"))
					.FillWidth(0.35f)

					+ SHeaderRow::Column("Platform")
					.DefaultLabel(LOCTEXT("DeviceListPlatformColumnHeader", "Platform"))
					.FillWidth(0.3f)

					+ SHeaderRow::Column("Host")
					.DefaultLabel(LOCTEXT("DeviceListHostColumnHeader", "Host"))
					.FillWidth(0.3f)

					+ SHeaderRow::Column("Owner")
					.DefaultLabel(LOCTEXT("DeviceListOwnerColumnHeader", "Owner"))
					.FillWidth(0.35f)
					)
					.ListItemsSource(&DeviceProxyList)
					.OnGenerateRow(this, &SSessionLauncherDeployTargets::HandleDeviceProxyListViewGenerateRow)
					.SelectionMode(ESelectionMode::Single)
					.Visibility(this, &SSessionLauncherDeployTargets::HandleDeviceProxyListViewVisibility)
				]

			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					.Visibility(this, &SSessionLauncherDeployTargets::HandleNoDevicesBoxVisibility)

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
							.Text(this, &SSessionLauncherDeployTargets::HandleNoDevicesTextBlockText)
						]					
				]
		];

		TSharedPtr<SWidget> SimpleWidget = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVerticalBox)
				.Visibility(this, &SSessionLauncherDeployTargets::HandleDeviceSelectorVisibility)

				+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.MaxHeight(288.0f)
					.Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						// device list
						SAssignNew(DeviceProxyListView, SListView<ITargetDeviceProxyPtr>)
						.ItemHeight(16.0f)
						.HeaderRow
						(
						SNew(SHeaderRow)

						+ SHeaderRow::Column("CheckBox")
						.DefaultLabel(LOCTEXT("DeviceListCheckboxColumnHeader", " "))
						.FixedWidth(24.0f)

						+ SHeaderRow::Column("Device")
						.DefaultLabel(LOCTEXT("DeviceListDeviceColumnHeader", "Device"))
						.FillWidth(0.35f)

						+ SHeaderRow::Column("Platform")
						.DefaultLabel(LOCTEXT("DeviceListPlatformColumnHeader", "Platform"))
						.FillWidth(0.3f)

						+ SHeaderRow::Column("Host")
						.DefaultLabel(LOCTEXT("DeviceListHostColumnHeader", "Host"))
						.FillWidth(0.3f)

						+ SHeaderRow::Column("Owner")
						.DefaultLabel(LOCTEXT("DeviceListOwnerColumnHeader", "Owner"))
						.FillWidth(0.35f)
						)
						.ListItemsSource(&DeviceProxyList)
						.OnGenerateRow(this, &SSessionLauncherDeployTargets::HandleDeviceProxyListViewGenerateRow)
						.SelectionMode(ESelectionMode::Single)
						.Visibility(this, &SSessionLauncherDeployTargets::HandleDeviceProxyListViewVisibility)
					]
			];

	ChildSlot
	[
		IsAlwaysEnabled ? SimpleWidget.ToSharedRef() : ComplexWidget.ToSharedRef()
	];

	Model->GetDeviceProxyManager()->OnProxyAdded().AddSP(this, &SSessionLauncherDeployTargets::HandleDeviceProxyManagerProxyAdded);
	Model->OnProfileSelected().AddSP(this, &SSessionLauncherDeployTargets::HandleProfileManagerProfileSelected);

	ShowDevicesChoice = EShowDevicesChoices::ShowAllDevices;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SCompoundWidget overrides
 *****************************************************************************/

void SSessionLauncherDeployTargets::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// @todo gmp: refactor this to be event-driven
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ILauncherDeviceGroupPtr SelectedDeviceGroup = SelectedProfile->GetDeployedDeviceGroup();

		if (DeployedDeviceGroup != SelectedDeviceGroup)
		{
			DeployedDeviceGroup = SelectedDeviceGroup;
		}

		RefreshDeviceProxyList();
	}

	if (DeviceGroupSelector->GetSelectedGroup() != DeployedDeviceGroup)
	{
		DeviceGroupSelector->SetSelectedGroup(DeployedDeviceGroup);
	}
}


/* SSessionLauncherDeployTargets implementation
 *****************************************************************************/

void SSessionLauncherDeployTargets::RefreshDeviceProxyList( )
{
	DeviceProxyList.Reset();

	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		ILauncherDeviceGroupPtr SelectedGroup = SelectedProfile->GetDeployedDeviceGroup();

		DeviceGroupSelector->SetSelectedGroup(SelectedGroup);

		if (SelectedGroup.IsValid())
		{
			const TArray<FString>& Devices = SelectedGroup->GetDevices();

			for (int32 Index = 0; Index < Devices.Num(); ++Index)
			{
				DeviceProxyList.Add(Model->GetDeviceProxyManager()->FindOrAddProxy(Devices[Index]));
			}

			if (ShowDevicesChoice == EShowDevicesChoices::ShowAllDevices)
			{
				Model->GetDeviceProxyManager()->GetProxies(FString(), false, DeviceProxyList);
			}		
		}
	}

	DeviceProxyListView->RequestListRefresh();
}


/* SSessionLauncherDeployTargets callbacks
 *****************************************************************************/

void SSessionLauncherDeployTargets::HandleDeviceGroupSelectorGroupSelected( const ILauncherDeviceGroupPtr& SelectedGroup )
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedGroup.IsValid())
		{
			SelectedProfile->SetDeployedDeviceGroup(SelectedGroup);
		}
		else
		{
			SelectedProfile->SetDeployedDeviceGroup(NULL);
		}
	}
}


ILauncherDeviceGroupPtr SSessionLauncherDeployTargets::HandleDeviceListRowDeviceGroup( ) const
{
	return DeployedDeviceGroup;
}


bool SSessionLauncherDeployTargets::HandleDeviceListRowIsEnabled( ITargetDeviceProxyPtr DeviceProxy ) const
{
	if (DeviceProxy.IsValid())
	{
		ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

		if (SelectedProfile.IsValid())
		{
			bool RetVal = SelectedProfile->IsDeployablePlatform(DeviceProxy->GetPlatformName()) || IsAlwaysEnabled;
			return RetVal;
		}
	}

	return false;
}


FText SSessionLauncherDeployTargets::HandleDeviceListRowToolTipText( ITargetDeviceProxyPtr DeviceProxy ) const
{
	FTextBuilder Builder;
	Builder.AppendLineFormat(LOCTEXT("DeviceListRowToolTipName", "Name: {0}"), FText::FromString(DeviceProxy->GetName()));
	Builder.AppendLineFormat(LOCTEXT("DeviceListRowToolTipPlatform", "Platform: {0}"), FText::FromString(DeviceProxy->GetPlatformName()));
	Builder.AppendLineFormat(LOCTEXT("DeviceListRowToolTipDeviceId", "Device ID: {0}"), FText::FromString(DeviceProxy->GetDeviceId()));

	return Builder.ToText();
}


TSharedRef<ITableRow> SSessionLauncherDeployTargets::HandleDeviceProxyListViewGenerateRow( ITargetDeviceProxyPtr InItem, const TSharedRef<STableViewBase>& OwnerTable ) const
{
	check(Model->GetSelectedProfile().IsValid());
	check(Model->GetSelectedProfile()->GetDeployedDeviceGroup().IsValid());

	return SNew(SSessionLauncherDeviceListRow, OwnerTable)
		.DeviceGroup(this, &SSessionLauncherDeployTargets::HandleDeviceListRowDeviceGroup)
		.DeviceProxy(InItem)
		.IsEnabled(this, &SSessionLauncherDeployTargets::HandleDeviceListRowIsEnabled, InItem)
		.ToolTipText(this, &SSessionLauncherDeployTargets::HandleDeviceListRowToolTipText, InItem);
}


EVisibility SSessionLauncherDeployTargets::HandleDeviceProxyListViewVisibility( ) const
{
	if (DeviceProxyList.Num() > 0)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


void SSessionLauncherDeployTargets::HandleDeviceProxyManagerProxyAdded( const ITargetDeviceProxyRef& AddedProxy )
{
	RefreshDeviceProxyList();
}


EVisibility SSessionLauncherDeployTargets::HandleDeviceSelectorVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeployedDeviceGroup().IsValid())
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}


EVisibility SSessionLauncherDeployTargets::HandleNoDevicesBoxVisibility( ) const
{
	if (DeviceProxyList.Num() == 0)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}


FText SSessionLauncherDeployTargets::HandleNoDevicesTextBlockText( ) const
{
	if (DeviceProxyList.Num() == 0)
	{
		if (ShowDevicesChoice == EShowDevicesChoices::ShowAllDevices)
		{
			return LOCTEXT("NoDevicesText", "No available devices were detected.");
		}
		else if (ShowDevicesChoice == EShowDevicesChoices::ShowGroupDevices)
		{
			return LOCTEXT("EmptyGroupText", "There are no devices in this group.");
		}
	}

	return FText::GetEmpty();
}


void SSessionLauncherDeployTargets::HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile )
{
	RefreshDeviceProxyList();
}


EVisibility SSessionLauncherDeployTargets::HandleSelectDeviceGroupWarningVisibility( ) const
{
	ILauncherProfilePtr SelectedProfile = Model->GetSelectedProfile();

	if (SelectedProfile.IsValid())
	{
		if (SelectedProfile->GetDeployedDeviceGroup().IsValid())
		{
			return EVisibility::Collapsed;
		}
	}

	return EVisibility::Visible;
}


void SSessionLauncherDeployTargets::HandleShowCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState, EShowDevicesChoices::Type Choice )
{
	if (NewState == ESlateCheckBoxState::Checked)
	{
		ShowDevicesChoice = Choice;

		RefreshDeviceProxyList();
	}
}


ESlateCheckBoxState::Type SSessionLauncherDeployTargets::HandleShowCheckBoxIsChecked( EShowDevicesChoices::Type Choice ) const
{
	if (ShowDevicesChoice == Choice)
	{
		return ESlateCheckBoxState::Checked;
	}

	return ESlateCheckBoxState::Unchecked;
}


#undef LOCTEXT_NAMESPACE
