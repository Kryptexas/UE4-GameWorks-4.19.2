// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IUndoHistoryModule.h: Declares the IUndoHistoryModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for the UndoHistory module.
 */
class IUndoHistoryModule
	: public IModuleInterface
{
public:

	/**
	 * Gets a reference to the UndoHistory module instance.
	 *
	 * @todo gmp - better implementation using dependency injection.
	 *
	 * @return A reference to the module.
	 */
	static IUndoHistoryModule& Get( )
	{
		return FModuleManager::LoadModuleChecked<IUndoHistoryModule>("UndoHistory");
	}


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IUndoHistoryModule( ) { }
};
