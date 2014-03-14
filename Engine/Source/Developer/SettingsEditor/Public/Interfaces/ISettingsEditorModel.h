// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISettingsEditorModel.h: Declares the ISettingsEditorModel interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ISettingsEditorModel.
 */
typedef TSharedPtr<class ISettingsEditorModel> ISettingsEditorModelPtr;

/**
 * Type definition for shared references to instances of ISettingsEditorModel.
 */
typedef TSharedRef<class ISettingsEditorModel> ISettingsEditorModelRef;


/**
 * Interface for settings editor view models.
 */
class ISettingsEditorModel
{
public:

	/**
	 * Gets the selected settings section.
	 *
	 * @return The selected section, if any.
	 */
	virtual const ISettingsSectionPtr& GetSelectedSection( ) const = 0;

	/**
	 * Gets the settings container.
	 *
	 * @return The settings container.
	 */
	virtual const ISettingsContainerRef& GetSettingsContainer( ) const = 0;

	/**
	 * Selects the specified settings section to be displayed in the editor.
	 *
	 * @param Section - The section to select.
	 *
	 * @see SelectPreviousSection
	 */
	virtual void SelectSection( const ISettingsSectionPtr& Section ) = 0;


public:

	/**
	 * Returns a delegate that is executed when the selected settings section has changed.
	 *
	 * @return The delegate.
	 */
	virtual FSimpleMulticastDelegate& OnSelectionChanged( ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISettingsEditorModel( ) { }
};
