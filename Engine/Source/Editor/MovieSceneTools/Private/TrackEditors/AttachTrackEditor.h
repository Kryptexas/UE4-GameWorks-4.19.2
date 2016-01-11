// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene3DAttachSection.h"
#include "MovieScene3DAttachTrack.h"


/**
 * Tools for attaching an object to another object
 */
class F3DAttachTrackEditor
	: public FMovieSceneTrackEditor
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool
	 */
	F3DAttachTrackEditor( TSharedRef<ISequencer> InSequencer );

	/** Virtual destructor. */
	virtual ~F3DAttachTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

public:

	/** Add attach */
	void AddAttach(FGuid ObjectGuid, UObject* AdditionalAsset);

	/** Set attach */
	void SetAttach(UMovieSceneSection* Section, AActor* ActorWithSplineComponent);

public:

	// ISequencerTrackEditor interface

	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track ) override;
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;

private:

	/** Add attach sub menu */
	void AddAttachSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding);

	/** Delegate for AnimatablePropertyChanged in AddKey */
	void AddKeyInternal(float KeyTime, const TArray<UObject*> Objects, UObject* AdditionalAsset);
};
