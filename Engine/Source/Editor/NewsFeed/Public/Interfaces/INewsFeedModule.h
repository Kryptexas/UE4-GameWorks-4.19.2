// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	INewsFeedModule.h: Declares the INewsFeedModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for the news feed module.
 */
class INewsFeedModule
	: public IModuleInterface
{
public:

	/**
	 * Creates a news feed button widget.
	 *
	 * @return The new widget.
	 */
	virtual TSharedRef<class SWidget> CreateNewsFeedButton( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~INewsFeedModule( ) { }
};
