// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployToDeviceSettings.h: Declares the SSessionLauncherDeployToDeviceSettings class.
=============================================================================*/

#pragma once


/**
 * Implements the deploy-to-device settings panel.
 */
class SSessionLauncherDeployToDeviceSettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeployToDeviceSettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel, EVisibility InShowAdvanced = EVisibility::Visible );


private:

	// Callback for check state changes of the 'UnrealPak' check box.
	void HandleUnrealPakCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState );

	// Callback for determining the checked state of the 'UnrealPak' check box.
	ESlateCheckBoxState::Type HandleUnrealPakCheckBoxIsChecked( ) const;


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
