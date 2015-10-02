// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneColorTrack.h"


/**
* A property track editor for colors.
*/
class FColorPropertyTrackEditor
	: public FPropertyTrackEditor<UMovieSceneColorTrack, FColorKey>
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool
	 */
	FColorPropertyTrackEditor( TSharedRef<ISequencer> InSequencer )
		: FPropertyTrackEditor<UMovieSceneColorTrack, FColorKey>( InSequencer, NAME_Color, NAME_LinearColor, "SlateColor" )
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
	virtual bool TryGenerateKeyFromPropertyChanged( const UMovieSceneTrack* InTrack, const FPropertyChangedParams& PropertyChangedParams, FColorKey& OutKey ) override;
};
