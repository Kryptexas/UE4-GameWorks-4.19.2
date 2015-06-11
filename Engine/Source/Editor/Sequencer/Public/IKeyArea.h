// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Information for how to draw each key */
struct FKeyDrawingInfo
{
	FKeyDrawingInfo()
		: Brush( NULL )
		, Size( FVector2D::ZeroVector )
	{}

	/** The brush to use for each key */
	const FSlateBrush* Brush;
	/** The size of each key */
	const FVector2D Size;
};

/**
 * Interface that should be implemented for the UI portion of a key area within a section
 */
class IKeyArea
{
public:
	virtual ~IKeyArea() {}

	/** @return The array of unsorted key handles in the key area */
	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const = 0;

	/** 
	 * Gets the time of a key given a handle
	 *
	 * @param KeyHandle Handle of the key
	 * @return The time of the key
	 */
	virtual float GetKeyTime( FKeyHandle KeyHandle ) const = 0;

	/**
	 * Moves a key
	 * 
	 * @param KeyHandle		Handle of the key to move
	 * @param DeltaPosition	The delta position of the key
	 * @return The handle of the key
	 */
	virtual FKeyHandle MoveKey( FKeyHandle KeyHandle, float DeltaPosition ) = 0;
	
	/**
	 * Deletes a key
	 * 
	 * @param KeyHandle The key to delete
	 */
	virtual void DeleteKey(FKeyHandle KeyHandle) = 0;

	/**
	 * Adds a key at the specified time if there isn't already a key present.  The value of the added key should
	 * be the value which would be returned if the animation containing this key area was evaluated at the specified time.
	 */
	virtual void AddKeyUnique(float Time) = 0;

	/** Gets the rich curve associated with this key area.  This can be null but it must be valid in order to be
	  * edited by the curve editor. */
	virtual FRichCurve* GetRichCurve() = 0;

	/**
	 * Gets the section which owns this key area.
	 */
	virtual UMovieSceneSection* GetOwningSection() = 0;

	/**
	 * Returns true if this key area can create a key editor widget for the animation outliner.
	 */
	virtual bool CanCreateKeyEditor() = 0;

	/**
	* Creates a key editor for this key area for use in the animation outliner.
	*/
	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) = 0;
};


