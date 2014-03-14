// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherSettings.h: Declares the SSessionLauncherCookPage class.
=============================================================================*/

#pragma once


namespace ESessionLauncherWizardPages
{
	/**
	 * Enumerates the session launcher wizard pages.
	 */
	enum Type
	{
		/**
		 * The 'Build' page.
		 */
		BuildPage,

		/**
		 * The 'Cook' page.
		 */
		CookPage,

		/**
		 * The 'Package' page.
		 */
		PackagePage,

		/**
		 * The 'Deploy' page.
		 */
		DeployPage,

		/**
		 * The 'Launch' page.
		 */
		LaunchPage,

		/**
		 * The 'Preview' page.
		 */
		PreviewPage
	};
}


/**
 * Implements the launcher settings widget.
 */
class SSessionLauncherSettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherSettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherSettings( );



public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	// Callback for getting the visibility of the 'Select Profile' text block.
	EVisibility HandleSelectProfileTextBlockVisibility( ) const;

	// Callback for getting the visibility of the settings scroll box.
	EVisibility HandleSettingsScrollBoxVisibility( ) const;


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
