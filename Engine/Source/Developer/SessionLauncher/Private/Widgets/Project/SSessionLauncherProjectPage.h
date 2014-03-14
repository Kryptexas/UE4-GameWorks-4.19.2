// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherProjectPage.h: Declares the SSessionLauncherProjectPage class.
=============================================================================*/

#pragma once


/**
 * Implements the profile page for the session launcher wizard.
 */
class SSessionLauncherProjectPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherProjectPage) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InShowConfig = true );


private:

	// Callback for getting the enabled state of the project settings area.
	bool HandleProjectSettingsAreaIsEnabled( ) const;


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
