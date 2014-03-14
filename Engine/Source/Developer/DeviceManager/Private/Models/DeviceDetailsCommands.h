// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceDetailsCommands.h: Declares the FDeviceDetailsCommands class.
=============================================================================*/

#pragma once


/**
 * The device details commands
 */
class FDeviceDetailsCommands
	: public TCommands<FDeviceDetailsCommands>
{
public:

	/**
	 * Default constructor.
	 */
	FDeviceDetailsCommands()
		: TCommands<FDeviceDetailsCommands>(
			"DeviceDetails",
			NSLOCTEXT("Contexts", "DeviceDetails", "Device Details"),
			NAME_None, FEditorStyle::GetStyleSetName()
		)
	{ }

public:

	// Begin TCommands interface

	virtual void RegisterCommands( ) OVERRIDE
	{
		UI_COMMAND(Claim, "Claim", "Claim the device", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Release, "Release", "Release the device", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Share, "Share", "Share the device with other users", EUserInterfaceActionType::ToggleButton, FInputGesture());

		UI_COMMAND(PowerOn, "Power On", "Power on the device", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(PowerOff, "Power Off", "Power off the device", EUserInterfaceActionType::Button, FInputGesture());
		UI_COMMAND(Reboot, "Reboot", "Reboot the device", EUserInterfaceActionType::Button, FInputGesture());
	}

	// End TCommands interface

public:

	// Device claim commands
	TSharedPtr<FUICommandInfo> Claim;
	TSharedPtr<FUICommandInfo> Release;
	TSharedPtr<FUICommandInfo> Share;

	// Device control commands
	TSharedPtr<FUICommandInfo> PowerOff;
	TSharedPtr<FUICommandInfo> PowerOn;
	TSharedPtr<FUICommandInfo> Reboot;
};
