// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* A central repository that can be used to track and manage chunks of slate style data.
*/
class SLATE_API FSlateStyleRegistry
{
public:
	/**
	 * Add a slate style to the repository.
	 * @param ISlateStyle - interface to a user-definable style to add.
	 */
	static void RegisterSlateStyle( const ISlateStyle& InSlateStyle );

	/**
	 * Remove a slate style from the repository.
	 * @param ISlateStyle - interface to the user-definable style to remove.
	 */
	static void UnRegisterSlateStyle( const ISlateStyle& InSlateStyle );

	static void UnRegisterSlateStyle( const FName StyleSetName);

	/**
	 * Find a slate style in the repository.
	 * @param InSlateStyleName - The name of the slate style to find.
	 * @return The slate style, or null if it couldn't be found
	 */
	static const ISlateStyle* FindSlateStyle( const FName& InSlateStyleName );

	/**
	 * Populate an array of slate brushes with all of the resources used by the registered styles.
	 * @param OutResources - array of slate brushes to populate.
	 */
	static void GetAllResources( TArray< const FSlateBrush* >& OutResources );


protected:
private:

	/** Repository is just a collection of shared style pointers. */
	static TMap< FName, const ISlateStyle* > SlateStyleRepository;
};