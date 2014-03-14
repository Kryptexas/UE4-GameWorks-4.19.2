// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieScenePropertyTrack.h"
#include "MovieSceneVectorTrack.generated.h"

/**
 * Handles manipulation of component transforms in a movie scene
 */
UCLASS( dependson=UMovieScenePropertyTrack, MinimalAPI )
class UMovieSceneVectorTrack : public UMovieScenePropertyTrack
{
	GENERATED_UCLASS_BODY()
public:
	/** UMovieSceneTrack interface */
	virtual UMovieSceneSection* CreateNewSection() OVERRIDE;
	virtual TSharedPtr<IMovieSceneTrackInstance> CreateInstance() OVERRIDE;

	/**
	 * Adds a key to a section.  Will create the section if it doesn't exist
	 *
	 * @param ObjectHandle		Handle to the object(s) being changed
	 * @param InPropertyName	The name of the property being manipulated.  @todo Sequencer - Could be a UFunction name
	 * @param Time				The time relative to the owning movie scene where the section should be
	 * @param InKey				The vector key to add
	 * @param InChannelsUsed	The number of channels used, 2, 3, or 4, determining which type of vector this is
	 * @return True if the key was successfully added.
	 */
	virtual bool AddKeyToSection( float Time, const FVector4& InKey, int32 InChannelsUsed );
	virtual bool AddKeyToSection( float Time, const FVector4& InKey);
	virtual bool AddKeyToSection( float Time, const FVector& InKey);
	virtual bool AddKeyToSection( float Time, const FVector2D& InKey);
	
	/**
	 * Evaluates the track at the playback position
	 *
	 * @param Position	The current playback position
	 * @param LastPosition	The last plackback position
	 * @param OutVector 	The value at the playback position
	 * @return true if anything was evaluated. Note: if false is returned OutVector remains unchanged
	 */
	virtual bool Eval( float Position, float LastPostion, FVector4& OutVector ) const;
};
