// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements the launcher settings widget.
 */
class SSessionLauncherBuildTaskSettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherBuildTaskSettings) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherBuildTaskSettings( );

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 * @param InModel The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );

private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
