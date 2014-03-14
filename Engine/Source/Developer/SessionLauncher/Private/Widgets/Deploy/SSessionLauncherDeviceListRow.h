// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDevicesListRow.h: Declares the SSessionLauncherDevicesListRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionLauncherDevicesListRow"


/**
 * Implements a row widget for the launcher's device proxy list.
 */
class SSessionLauncherDeviceListRow
	: public SMultiColumnTableRow<ITargetDeviceProxyPtr>
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeviceListRow) { }
		
		/**
		 * The currently selected device group.
		 */
		SLATE_ATTRIBUTE(ILauncherDeviceGroupPtr, DeviceGroup)

		/**
		 * The device proxy shown in this row.
		 */
		SLATE_ARGUMENT(ITargetDeviceProxyPtr, DeviceProxy)

		/**
		 * The row's highlight text.
		 */
		SLATE_ATTRIBUTE(FText, HighlightText)

	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InDeviceGroup - The device group that is being edited.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		DeviceGroup = InArgs._DeviceGroup;
		DeviceProxy = InArgs._DeviceProxy;
		HighlightText = InArgs._HighlightText;

		SMultiColumnTableRow<ITargetDeviceProxyPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}


public:

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName - The name of the column to generate the widget for.
	 *
	 * @return The widget.
	 */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == "CheckBox")
		{
			return SNew(SCheckBox)
				.IsChecked(this, &SSessionLauncherDeviceListRow::HandleCheckBoxIsChecked)
				.OnCheckStateChanged(this, &SSessionLauncherDeviceListRow::HandleCheckBoxStateChanged)
				.ToolTipText(LOCTEXT("CheckBoxToolTip", "Check this box to include this device in the current device group").ToString());
		}
		else if (ColumnName == "Device")
		{
			return SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SImage)
						.Image(this, &SSessionLauncherDeviceListRow::HandleDeviceImage)
				]

			+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(FMargin(4.0, 0.0))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SSessionLauncherDeviceListRow::HandleDeviceNameText)
				];
		}
		else if (ColumnName == "Platform")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0, 0.0))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SSessionLauncherDeviceListRow::HandleHostPlatformText)
				];
		}
		else if (ColumnName == "Host")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0, 0.0))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SSessionLauncherDeviceListRow::HandleHostNameText)
				];
		}
		else if (ColumnName == "Owner")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0, 0.0))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(this, &SSessionLauncherDeviceListRow::HandleHostUserText)
				];
		}

		return SNullWidget::NullWidget;
	}


private:

	// Callback for changing this row's check box state.
	void HandleCheckBoxStateChanged( ESlateCheckBoxState::Type NewState )
	{
		ILauncherDeviceGroupPtr ActiveGroup = DeviceGroup.Get();

		if (ActiveGroup.IsValid())
		{
			if (NewState == ESlateCheckBoxState::Checked)
			{
				ActiveGroup->AddDevice(DeviceProxy->GetDeviceId());
			}
			else
			{
				ActiveGroup->RemoveDevice(DeviceProxy->GetDeviceId());
			}
		}
	}

	// Callback for determining this row's check box state.
	ESlateCheckBoxState::Type HandleCheckBoxIsChecked( ) const
	{
		if (IsEnabled())
		{
			ILauncherDeviceGroupPtr ActiveGroup = DeviceGroup.Get();

			if (ActiveGroup.IsValid())
			{
				if (ActiveGroup->GetDevices().Contains(DeviceProxy->GetDeviceId()))
				{
					return ESlateCheckBoxState::Checked;
				}
			}
		}

		return ESlateCheckBoxState::Unchecked;
	}

	// Callback for getting the icon image of the device.
	const FSlateBrush* HandleDeviceImage( ) const
	{
		return FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), *DeviceProxy->GetPlatformName()));
	}

	// Callback for getting the friendly name.
	FString HandleDeviceNameText( ) const
	{
		const FString& Name = DeviceProxy->GetName();

		if (Name.IsEmpty())
		{
			return LOCTEXT("UnnamedDeviceName", "<unnamed>").ToString();
		}

		return Name;
	}

	// Callback for getting the host name.
	FString HandleHostNameText( ) const
	{
		return DeviceProxy->GetHostName();
	}

	// Callback for getting the host user name.
	FString HandleHostUserText( ) const
	{
		return DeviceProxy->GetHostUser();
	}

	// Callback for getting the host platform name.
	FString HandleHostPlatformText( ) const
	{
		return DeviceProxy->GetPlatformName();
	}

private:

	// Holds a pointer to the device group that is being edited.
	TAttribute<ILauncherDeviceGroupPtr> DeviceGroup;

	// Holds a reference to the device proxy that is displayed in this row.
	ITargetDeviceProxyPtr DeviceProxy;

	// Holds the highlight string for the log message.
	TAttribute<FText> HighlightText;
};


#undef LOCTEXT_NAMESPACE
