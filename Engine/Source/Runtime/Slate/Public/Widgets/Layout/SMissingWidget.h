// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMissingWidget.h: Declares the SMissingWidget class.
=============================================================================*/

#pragma once


/**
 * Like a null widget, but visualizes itself as being explicitly missing.
 */
class SLATE_API SMissingWidget
{
public:

	/**
	 * Creates a new instance.
	 *
	 * @return The widget.
	 */
	static TSharedRef<class SWidget> MakeMissingWidget( );
};
