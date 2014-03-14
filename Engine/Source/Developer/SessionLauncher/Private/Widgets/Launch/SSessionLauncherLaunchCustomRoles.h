// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherLaunchCustomRoles.h: Declares the SSessionLauncherLaunchCustomRoles class.
=============================================================================*/

#pragma once


/**
 * Implements the settings panel for launching with custom roles.
 */
class SSessionLauncherLaunchCustomRoles
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherLaunchCustomRoles) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherLaunchCustomRoles( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


protected:


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
