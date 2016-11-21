// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieSceneIntegerTrack.h"
#include "MovieSceneIntegerSection.h"
#include "PropertyTrackEditor.h"


class ISequencer;


/**
 * A property track editor for integers.
 */
class FIntegerPropertyTrackEditor
	: public FPropertyTrackEditor<UMovieSceneIntegerTrack, UMovieSceneIntegerSection, int32>
{
public:

	/**
	 * Constructor.
	 *
	 * @param InSequencer The sequencer instance to be used by this tool.
	 */
	FIntegerPropertyTrackEditor(TSharedRef<ISequencer> InSequencer)
		: FPropertyTrackEditor(InSequencer, NAME_IntProperty)
	{ }

	/**
	 * Creates an instance of this class (called by a sequencer).
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:

	//~ ISequencerTrackEditor interface

	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;

protected:

	virtual void GenerateKeysFromPropertyChanged(const FPropertyChangedParams& PropertyChangedParams, TArray<int32>& NewGeneratedKeys, TArray<int32>& DefaultGeneratedKeys) override;
};
