// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IInputBindingEditorModule.h: Declares the IInputBindingEditorModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for input binding editor modules.
 */
class IInputBindingEditorModule
	: public IModuleInterface
{
public:

	/**
	 * Creates an input binding editor panel widget.
	 *
	 * @return The created widget.
	 */
	virtual TWeakPtr<SWidget> CreateInputBindingEditorPanel( ) = 0;

	/**
	 * Destroys a previously created editor panel widget.
	 *
	 * @param Panel - The panel to destroy.
	 */
	virtual void DestroyInputBindingEditorPanel( const TWeakPtr<SWidget>& Panel ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IInputBindingEditorModule( ) { }
};
