// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDevicesPage.h: Declares the SSessionLauncherDevicesPage class.
=============================================================================*/

#pragma once


/**
 * Implements the devices page for the session launcher wizard.
 */
class SSessionLauncherDeployPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeployPage) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherDeployPage( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool IsFromRepository = false );


private:

	// Callback for getting the content text of the 'Deployment Mode' combo button.
	FString HandleDeploymentModeComboButtonContentText( ) const;

	// Callback for clicking an item in the 'Deployment Mode' menu.
	void HandleDeploymentModeMenuEntryClicked( ELauncherProfileDeploymentModes::Type DeploymentMode );

	// Callback for getting the visibility of the deployment targets area.
	EVisibility HandleDeployFileServerSettingsVisibility( ) const;

	// Callback for getting the visibility of the deployment targets area.
	EVisibility HandleDeployToDeviceSettingsVisibility( ) const;

	// Callback for getting the visibility of the deployment repository area.
	EVisibility HandleDeployRepositorySettingsVisibility( ) const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Callback for determining the visibility of a validation error icon.
	EVisibility HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const;


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
