// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessagingModule.h: Declares the IMessagingModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for messaging modules.
 */
class ISlateReflectorModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a widget reflector widget.
	 *
	 * @return The widget.
	 */
	virtual TSharedRef<SWidget> GetWidgetReflector( ) = 0;

	/**
	 * Registers a tab spawner for the widget reflector.
	 *
	 * @param WorkspaceGroup The workspace group to insert the tab into.
	 */
	virtual void RegisterTabSpawner( const TSharedRef<FWorkspaceItem>& WorkspaceGroup ) = 0;

	/**
	 * Unregisters the tab spawner for the widget reflector.
	 */
	virtual void UnregisterTabSpawner( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISlateReflectorModule( ) { }
};
