// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneVectorTrack.h"


/**
 * A property track editor for vectors.
 */
class FVectorPropertyTrackEditor
	: public FPropertyTrackEditor<UMovieSceneVectorTrack, FVectorKey>
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool
	 */
	FVectorPropertyTrackEditor( TSharedRef<ISequencer> InSequencer )
		: FPropertyTrackEditor<UMovieSceneVectorTrack, FVectorKey>( InSequencer, NAME_Vector, NAME_Vector4, NAME_Vector2D )
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

	virtual bool TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, FVectorKey& OutKey ) override;
};
