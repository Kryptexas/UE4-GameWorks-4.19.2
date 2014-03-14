// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISettingsEditorModule.h: Declares the ISettingsEditorModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for settings editor modules.
 */
class ISettingsEditorModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a settings editor widget.
	 *
	 * @param Model - The view model.
	 *
	 * @return The new widget.
	 *
	 * @see CreateController
	 */
	virtual TSharedRef<SWidget> CreateEditor( const ISettingsEditorModelRef& Model ) = 0;

	/**
	 * Creates a controller for the settings editor widget.
	 *
	 * @param SettingsContainer - The settings container.
	 *
	 * @return The controller.
	 *
	 * @see CreateEditor
	 */
	virtual ISettingsEditorModelRef CreateModel( const ISettingsContainerRef& SettingsContainer ) = 0;


public:

	/**
	 * Gets a reference to the SettingsEditor module instance.
	 *
	 * @todo gmp: better implementation using dependency injection / abstract factory.
	 *
	 * @return A reference to the SettingsEditor module.
	 */
	static ISettingsEditorModule& GetRef( )
	{
		return FModuleManager::GetModuleChecked<ISettingsEditorModule>("SettingsEditor");
	}


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISettingsEditorModule( ) { }
};
