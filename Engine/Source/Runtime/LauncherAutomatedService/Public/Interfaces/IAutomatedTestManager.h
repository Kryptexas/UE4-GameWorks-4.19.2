// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IAutomatedTestManager.h: Declares the IAutomatedTestManager interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IAutomatedTestManager.
 */
typedef TSharedPtr<class IAutomatedTestManager> IAutomatedTestManagerPtr;

/**
 * Type definition for shared references to instances of IAutomatedTestManager.
 */
typedef TSharedRef<class IAutomatedTestManager> IAutomatedTestManagerRef;


/**
 * Interface for the automated test manager
 */
class IAutomatedTestManager
	: public TSharedFromThis<IAutomatedTestManager>
{
public:

	/*
	 * Check to see if we have finished running the automation tests
	 *
	 * @return True if we have finished
	 */
	virtual bool IsTestingComplete() = 0;

	/*
	 * Start the testing process. Called after we have discovered some devices
	 */
	virtual void RunTests() = 0;

	/*
	 * Tick function to update automation tests
	 *
	 * @param DeltaTime - Time since last update
	 */
	virtual void Tick( float DeltaTime ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IAutomatedTestManager( ) { }
};