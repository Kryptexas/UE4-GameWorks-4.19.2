// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISettingsViewer.h: Declares the ISettingsViewer interface.
=============================================================================*/

#pragma once


/**
 * Interface for settings tabs.
 */
class ISettingsViewer
{
public:

	/**
	 * Shows the settings tab that belongs to this viewer with the specified settings section pre-selected.
	 *
	 * @param CategoryName - The name of the section's category.
	 * @param SectionName - The name of the section to select.
	 */
	virtual void ShowSettings( const FName& CategoryName, const FName& SectionName ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISettingsViewer( ) { }
};
