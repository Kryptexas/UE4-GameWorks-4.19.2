// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceBrowserTooltip.h: Declares the SDeviceBrowserTooltip class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SDeviceBrowserTooltip"


/**
 * Implements a tool tip for widget the device browser.
 */
class SDeviceBrowserTooltip
	: public SToolTip
{
public:

	SLATE_BEGIN_ARGS(SDeviceBrowserTooltip) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InDeviceServiceManager - The target device service manager to use.
	 * @param InDeviceManagerState - The optional device manager view state.
	 */
	void Construct( const FArguments& InArgs, const ITargetDeviceServiceRef& InDeviceService )
	{
		DeviceService = InDeviceService;

		SToolTip::Construct(
			SToolTip::FArguments()
				.Content()
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
								.HeightOverride(96.0f)
								.WidthOverride(96.0f)
								[
									SNew(SImage)
										.Image(this, &SDeviceBrowserTooltip::HandleDevicePlatformIcon)
								]
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(16.0f, 0.0f, 0.0f, 0.0f)
						.VAlign(VAlign_Center)
						[
							SNew(SGridPanel)
								.FillColumn(1, 1.0f)

							// name
							+ SGridPanel::Slot(0, 0)
								[
									SNew(STextBlock)
										.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
										.Text(LOCTEXT("DeviceNameLabel", "Name:"))
								]

							+ SGridPanel::Slot(1, 0)
								.Padding(16.0f, 0.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(this, &SDeviceBrowserTooltip::HandleDeviceNameText)
								]

							// platform
							+ SGridPanel::Slot(0, 1)
								.Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
										.Text(LOCTEXT("DevicePlatformLabel", "Platform:"))
								]

							+ SGridPanel::Slot(1, 1)
								.Padding(16.0f, 4.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(this, &SDeviceBrowserTooltip::HandleDevicePlatformText)
								]

							// operating system
							+ SGridPanel::Slot(0, 2)
								.Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
										.Text(LOCTEXT("DeviceMakeModelLabel", "OS Version:"))
								]

							+ SGridPanel::Slot(1, 2)
								.Padding(16.0f, 4.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(this, &SDeviceBrowserTooltip::HandleDeviceOsText)
								]

							// ID
							+ SGridPanel::Slot(0, 3)
								.Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
										.Text(LOCTEXT("DeviceIdLabel", "Device ID:"))
								]

							+ SGridPanel::Slot(1, 3)
								.Padding(16.0f, 4.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(this, &SDeviceBrowserTooltip::HandleDeviceIdText)
								]

							// default device
							+ SGridPanel::Slot(0, 4)
								.Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
										.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 9))
										.Text(LOCTEXT("DefaultIdLabel", "Default device:"))
								]

							+ SGridPanel::Slot(1, 4)
								.Padding(16.0f, 4.0f, 8.0f, 0.0f)
								[
									SNew(STextBlock)
										.Text(this, &SDeviceBrowserTooltip::HandleDeviceIsDefaultText)
								]
						]				
				]
		);
	}

private:

	// Callback for getting the device's unique identifier.
	FString HandleDeviceIdText( ) const
	{
		if (DeviceService.IsValid())
		{
			return DeviceService->GetDeviceId().ToString();
		}

		return TEXT("");
	}

	// Callback for getting the text that indicates whether the shown device is the platform's default device.
	FString HandleDeviceIsDefaultText( ) const
	{
		if (DeviceService.IsValid())
		{
			ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

			if (TargetDevice.IsValid() && TargetDevice->IsDefault())
			{
				return LOCTEXT("YesText", "yes").ToString();
			}

			return LOCTEXT("NoText", "no").ToString();
		}

		return TEXT("");
	}

	// Callback for getting the name of the shown device.
	FString HandleDeviceNameText( ) const
	{
		if (DeviceService.IsValid())
		{
			return DeviceService->GetCachedDeviceName();
		}

		return TEXT("");
	}

	// Callback for getting the make and model of the shown device.
	FString HandleDeviceOsText( ) const
	{
		if (DeviceService.IsValid())
		{
			ITargetDevicePtr TargetDevice = DeviceService->GetDevice();

			if (TargetDevice.IsValid())
			{
				return TargetDevice->GetOperatingSystemName();
			}
		}

		return TEXT("");
	}

	// Callback for getting the icon of the device's platform.
	const FSlateBrush* HandleDevicePlatformIcon( ) const
	{
		if (DeviceService.IsValid())
		{
			return FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s.XLarge"), *DeviceService->GetDeviceId().GetPlatformName()));
		}

		return NULL;
	}

	// Callback for getting the name of the device's platform.
	FString HandleDevicePlatformText( ) const
	{
		if (DeviceService.IsValid())
		{
			return DeviceService->GetDeviceId().GetPlatformName();
		}

		return TEXT("");
	}

private:

	// Holds the service for the device whose details are being shown.
	ITargetDeviceServicePtr DeviceService;
};


#undef LOCTEXT_NAMESPACE
