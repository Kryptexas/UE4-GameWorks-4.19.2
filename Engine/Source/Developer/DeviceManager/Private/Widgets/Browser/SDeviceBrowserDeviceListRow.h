// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceBrowserDeviceListRow.h: Declares the SDeviceBrowserDeviceListRow class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SDeviceBrowserDeviceListRow"


/**
 * Implements a row widget for the device list view.
 */
class SDeviceBrowserDeviceListRow
	: public SMultiColumnTableRow<ITargetDeviceProxyPtr>
{
public:

	SLATE_BEGIN_ARGS(SDeviceBrowserDeviceListRow) { }
		SLATE_ARGUMENT(ITargetDeviceServicePtr, DeviceService)
		SLATE_ATTRIBUTE(FText, HighlightText)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The arguments.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		check(InArgs._DeviceService.IsValid());

		DeviceService = InArgs._DeviceService;
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
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE
	{
		if (ColumnName == TEXT("Claimed"))
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SDeviceBrowserDeviceListRow::HandleTextColorAndOpacity)
						.Text(this, &SDeviceBrowserDeviceListRow::HandleClaimedText)					
				];
		}
		else if (ColumnName == TEXT("Icon"))
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				[
					SNew(SImage)
						.Image(FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), *DeviceService->GetDeviceId().GetPlatformName())))
				];
		}
		else if (ColumnName == TEXT("Name"))
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SDeviceBrowserDeviceListRow::HandleTextColorAndOpacity)
						.HighlightText(HighlightText)
						.Text(this, &SDeviceBrowserDeviceListRow::HandleNameText)
				];
		}
		else if (ColumnName == TEXT("Platform"))
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SDeviceBrowserDeviceListRow::HandleTextColorAndOpacity)
						.Text(DeviceService->GetDeviceId().GetPlatformName())
				];
		}
		else if (ColumnName == TEXT("Share"))
		{
			return SNew(SBox)
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SCheckBox)
						.IsChecked(this, &SDeviceBrowserDeviceListRow::HandleShareCheckBoxIsChecked)
						.IsEnabled(this, &SDeviceBrowserDeviceListRow::HandleShareCheckBoxIsEnabled)
						.OnCheckStateChanged(this, &SDeviceBrowserDeviceListRow::HandleShareCheckBoxStateChanged)
						.ToolTipText(LOCTEXT("ShareCheckBoxToolTip", "Check this box to share this device with other users on the network").ToString())
				];
		}
		else if (ColumnName == TEXT("Status"))
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(this, &SDeviceBrowserDeviceListRow::HandleTextColorAndOpacity)
						.Text(this, &SDeviceBrowserDeviceListRow::HandleStatusTextBlockText)
				];
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:

	// Callback for getting the text in the 'Claimed' column.
	FString HandleClaimedText( ) const
	{
		FString ClaimUser = DeviceService->GetClaimUser();

		if ((ClaimUser == FPlatformProcess::UserName(false)) && !DeviceService->IsRunning())
		{
			ClaimUser += LOCTEXT("RemotelyHint", " (remotely)").ToString();
		}

		return ClaimUser;
	}

	// Callback for getting the text in the 'Name' column.
	FString HandleNameText( ) const
	{
		return DeviceService->GetCachedDeviceName();
	}

	// Callback for changing this row's Share check box state.
	void HandleShareCheckBoxStateChanged( ESlateCheckBoxState::Type NewState )
	{
		DeviceService->SetShared(NewState == ESlateCheckBoxState::Checked);
	}

	// Callback for getting the state of the 'Share' check box.
	ESlateCheckBoxState::Type HandleShareCheckBoxIsChecked( ) const
	{
		if (DeviceService->IsShared())
		{
			return ESlateCheckBoxState::Checked;
		}

		return ESlateCheckBoxState::Unchecked;
	}

	// Callback for getting the enabled state of the 'Share' check box.
	bool HandleShareCheckBoxIsEnabled( ) const
	{
		return DeviceService->IsRunning();
	}

	// Callback for getting the status text.
	FString HandleStatusTextBlockText( ) const
	{
		ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

		if (TargetDevice.IsValid())
		{
			if (TargetDevice->IsConnected())
			{
				return LOCTEXT("StatusConnected", "Connected").ToString();
			}

			return LOCTEXT("StatusDisconnected", "Disconnected").ToString();
		}

		return LOCTEXT("StatusUnavailable", "Unavailable").ToString();
	}

	// Callback for getting the text color.
	FSlateColor HandleTextColorAndOpacity( ) const
	{
		if (DeviceService->CanStart())
		{
			return FSlateColor::UseForeground();
		}

		return FSlateColor::UseSubduedForeground();
	}

private:

	// Holds the target device service used to populate this row.
	ITargetDeviceServicePtr DeviceService;

	// Holds the highlight text for the log message.
	TAttribute<FText> HighlightText;
};


#undef LOCTEXT_NAMESPACE
