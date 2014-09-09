// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements the profile page for the session launcher wizard.
 */
class SProjectLauncherProjectPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectLauncherProjectPage) { }
		SLATE_ATTRIBUTE(ILauncherProfilePtr, LaunchProfile)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 * @param InModel The data model.
	 */
	void Construct(	const FArguments& InArgs, const FProjectLauncherModelRef& InModel, bool InShowConfig = true );

private:

	// Callback for getting the enabled state of the project settings area.
	bool HandleProjectSettingsAreaIsEnabled( ) const;

private:

	// Holds a pointer to the data model.
	FProjectLauncherModelPtr Model;
};
