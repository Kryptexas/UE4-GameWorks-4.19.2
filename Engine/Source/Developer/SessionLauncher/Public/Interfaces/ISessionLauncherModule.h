// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISessionLauncherModule.h: Declares the ISessionLauncherModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for launcher UI modules.
 */
class ISessionLauncherModule
	: public IModuleInterface
{
public:
	
	/**
	 * Creates a session launcher progress panel widget.
	 *
	 * @param LauncherWorker - The launcher worker.
	 *
	 * @return The new widget.
	 */
	virtual TSharedRef<class SWidget> CreateLauncherProgressPanel( const ILauncherWorkerRef& LauncherWorker ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISessionLauncherModule( ) { }
};
