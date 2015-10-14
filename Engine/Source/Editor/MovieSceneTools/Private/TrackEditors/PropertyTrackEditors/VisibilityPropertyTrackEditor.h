// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneVisibilityTrack.h"


/**
 * A property track for bool properties which have been set
 * to use a UMovieSceneVisibilityTrack through metadata.
 */
class FVisibilityPropertyTrackEditor
	: public FPropertyTrackEditor<UMovieSceneVisibilityTrack, bool>
{
public:

	/** Constructor. */
	FVisibilityPropertyTrackEditor( TSharedRef<ISequencer> InSequencer)
		// Don't supply any property type names to watch since the FBoolPropertyTrackEditor is already watching for bool property changes.
		: FPropertyTrackEditor<UMovieSceneVisibilityTrack, bool>( InSequencer )
	{ }

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

public:

	// ISequencerTrackEditor interface

	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track ) override;

protected:

	// FPropertyTrackEditor interface

	virtual bool TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, bool& OutKey ) override;
};
