// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SDeviceToolbar.h: Declares the SDeviceToolbar class.
=============================================================================*/

#pragma once


/**
 * Implements the device toolbar widget.
 */
class SDeviceToolbar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SDeviceToolbar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The construction arguments.
	 * @param InModel - The view model to use.
	 */
	void Construct( const FArguments& InArgs, const FDeviceManagerModelRef& InModel );

protected:

	/**
	 * Binds the device commands on our toolbar.
	 */
	void BindCommands( );

	/**
	 * Validates actions on the specified device.
	 *
	 * @param Device - The device to perform an action on.
	 *
	 * @return true if actions on the device are permitted, false otherwise.
	 */
	bool ValidateDeviceAction( const ITargetDeviceRef& Device ) const;

private:

	// Callback for executing the 'Claim' action.
	void HandleClaimActionExecute( );

	// Callback for determining whether the 'Claim' action can execute.
	bool HandleClaimActionCanExecute( );

	// Callback for executing the 'Power Off' action.
	void HandlePowerOffActionExecute( );

	// Callback for determining whether the 'Power Off' action can execute.
	bool HandlePowerOffActionCanExecute( );

	// Callback for executing the 'Power On' action.
	void HandlePowerOnActionExecute( );

	// Callback for determining whether the 'Power On' action can execute.
	bool HandlePowerOnActionCanExecute( );

	// Callback for executing the 'Reboot' action.
	void HandleRebootActionExecute( );

	// Callback for determining whether the 'Reboot' action can execute.
	bool HandleRebootActionCanExecute( );

	// Callback for executing the 'Release' action.
	void HandleReleaseActionExecute( );

	// Callback for determining whether the 'Release' action can execute.
	bool HandleReleaseActionCanExecute( );

	// Callback for executing the 'Share' action.
	void HandleShareActionExecute( );

	// Callback for determining the checked state of the 'Share' action.
	bool HandleShareActionIsChecked( );

	// Callback for determining whether the 'Share' action can execute.
	bool HandleShareActionCanExecute( );

	// Callback for getting the enabled state of the toolbar.
	bool HandleToolbarIsEnabled( ) const;

private:

	// Holds a pointer the device manager's view model.
	FDeviceManagerModelPtr Model;

	// The command list for controlling the device
	TSharedPtr<FUICommandList> UICommandList;
};
