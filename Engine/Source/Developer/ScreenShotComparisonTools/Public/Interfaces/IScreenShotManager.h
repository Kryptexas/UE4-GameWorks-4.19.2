// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IScreenShotComparisonModule.h: Declares the IScreenShotComparisonModule interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IScreenShotManager.
 */
typedef TSharedPtr<class IScreenShotManager> IScreenShotManagerPtr;

/**
 * Type definition for shared references to instances of IScreenShotManager.
 */
typedef TSharedRef<class IScreenShotManager> IScreenShotManagerRef;


/**
 * Delegate type for session filter changing.
 */
DECLARE_DELEGATE(FOnScreenFilterChanged)


/**
 * Interface for screen manager module.
 */
class IScreenShotManager
{
public:

	/**
	* Generate the screen shot data
	*/
	virtual void GenerateLists( ) = 0;

	/**
	* Get the screen shot list
	*
	* @return the array of screen shot data
	*/
	virtual TArray<IScreenShotDataPtr>& GetLists( ) = 0;

	/**
	* Get the list of active platforms
	*
	* @return the array of platforms
	*/
	virtual TArray< TSharedPtr<FString> >& GetCachedPlatfomList( ) = 0;

	/**
	* Register for screen shot updates
	*
	* @param InDelegate - Delegate register
	*/
	virtual void RegisterScreenShotUpdate( const FOnScreenFilterChanged& InDelegate ) = 0;

	/**
	* Set the filter
	*
	* @param InFilter - The screen shot filter
	*/
	virtual void SetFilter(TSharedPtr<ScreenShotFilterCollection> InFilter ) = 0;
};
