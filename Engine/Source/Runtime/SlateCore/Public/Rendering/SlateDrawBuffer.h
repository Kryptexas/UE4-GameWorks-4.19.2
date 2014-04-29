// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateDrawBuffer.h: Declares the FSlateDrawBuffer class.
=============================================================================*/

#pragma once


/**
 * Implements a draw buffer for Slate.
 */
class SLATECORE_API FSlateDrawBuffer
{
public:

	/**
	 * Default constructor.
	 */
	explicit FSlateDrawBuffer( )
		: Locked(0)
	{ }

public:

	/**
	 * Removes all data from the buffer.
	 */
	void ClearBuffer( )
	{
		WindowElementLists.Empty();
	}

	/**
	 * Creates a new FSlateWindowElementList and returns a reference to it so it can have draw elements added to it
	 *
	 * @param ForWindow    The window for which we are creating a list of paint elements.
	 */
	FSlateWindowElementList& AddWindowElementList( TSharedRef<SWindow> ForWindow );

	/**
	 * Gets all window element lists in this buffer.
	 */
	TArray<FSlateWindowElementList>& GetWindowElementLists( )
	{
		return WindowElementLists;
	}

	/** 
	 * Locks the draw buffer.  Indicates that the viewport is in use
	 *
	 * @return true if the viewport could be locked.  False otherwise
	 */
	bool Lock( );

	/**
	 * Unlocks the buffer.  Indicates that the buffer is free                   
	 */
	void Unlock( );

protected:

	// List of window element lists.
	TArray<FSlateWindowElementList> WindowElementLists;

	// 1 if this buffer is locked, 0 otherwise.
	int32 Locked;
};
