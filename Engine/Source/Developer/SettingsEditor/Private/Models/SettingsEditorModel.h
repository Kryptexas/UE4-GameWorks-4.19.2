// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SettingsEditorModel.h: Declares the FSettingsEditorModel interface.
=============================================================================*/

#pragma once


/**
 * Implements a view model for the settings editor widget.
 */
class FSettingsEditorModel
	: public ISettingsEditorModel
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InSettingsContainer - The settings container to use.
	 */
	FSettingsEditorModel( const ISettingsContainerRef& InSettingsContainer )
		: SettingsContainer(InSettingsContainer)
	{
		SettingsContainer->OnSectionRemoved().AddRaw(this, &FSettingsEditorModel::HandleSettingsContainerSectionRemoved);

	}

	/**
	 * Destructor.
	 */
	~FSettingsEditorModel( )
	{
		SettingsContainer->OnSectionRemoved().RemoveAll(this);
	}

public:

	// Begin ISettingsEditorModel interface

	virtual const ISettingsSectionPtr& GetSelectedSection( ) const OVERRIDE
	{
		return SelectedSection;
	}

	virtual const ISettingsContainerRef& GetSettingsContainer( ) const OVERRIDE
	{
		return SettingsContainer;
	}

	virtual FSimpleMulticastDelegate& OnSelectionChanged( ) OVERRIDE
	{
		return OnSelectionChangedDelegate;
	}

	virtual void SelectSection( const ISettingsSectionPtr& Section ) OVERRIDE
	{
		if (Section == SelectedSection)
		{
			return;
		}

		SelectedSection = Section;

		OnSelectionChangedDelegate.Broadcast();
	}

	// End ISettingsEditorModel interface

private:

	// Handles the removal of sections from the settings container.
	void HandleSettingsContainerSectionRemoved( const ISettingsSectionRef& Section )
	{
		if (SelectedSection == Section)
		{
			SelectSection(nullptr);
		}
	}

private:

	// Holds the currently selected settings section.
	ISettingsSectionPtr SelectedSection;

	// Holds a reference to the settings container.
	ISettingsContainerRef SettingsContainer;

private:

	// Holds a delegate that is executed when the selected settings section has changed.
	FSimpleMulticastDelegate OnSelectionChangedDelegate;
};
