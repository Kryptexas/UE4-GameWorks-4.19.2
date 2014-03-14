// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


/**
 * Implements details customization for the Preferences window.
 */
class FPreferencesDetails
	: public IDetailCustomization
{
public:

	/**
	 * Default constructor.
	 */
	FPreferencesDetails( );


public:

	/**
	 * Makes a new instance of this detail layout class for a specific detail view requesting it
	 */
	static TSharedRef<class IDetailCustomization> MakeInstance( );


public:

	// Begin ILayoutDetails interface

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

	// End ILayoutDetails interface


protected:

	/**
	 * Finds a window to parent to, used for the Open File dialog.
	 */
	void* ChooseParentWindowHandle( );

	/**
	 * Internal function for making a backup of the preferences, will display notifications and log errors.
	 */
	void MakeBackup( );

	/**
	 * Opens the directory where the backup for preferences is stored.
	 *
	 * The parameter is needed so it can be hooked up to hyper-links.
	 */
	void OpenBackupDirectory( );


private:

	// Sets up resetting the UI layout.
	FReply HandleResetUILayoutClicked( );


private:

	FString BackupUserSettingsIni;
};
