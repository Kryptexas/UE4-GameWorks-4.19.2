// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployTaskSettings.h: Declares the SSessionLauncherDeployTaskSettings class.
=============================================================================*/

#pragma once


/**
 * Implements the launcher settings widget.
 */
class SSessionLauncherDeployTaskSettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeployTaskSettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherDeployTaskSettings( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
