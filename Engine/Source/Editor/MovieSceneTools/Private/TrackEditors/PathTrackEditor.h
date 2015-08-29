// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MovieScene3DPathSection.h"
#include "MovieScene3DPathTrack.h"
/**
 * Tools for animating a transform with a path curve
 */
class F3DPathTrackEditor : public FMovieSceneTrackEditor
{
public:
	/**
	 * Constructor
	 *
	 * @param InSequencer	The sequencer instance to be used by this tool
	 */
	F3DPathTrackEditor( TSharedRef<ISequencer> InSequencer );
	~F3DPathTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer 
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<FMovieSceneTrackEditor> CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer );

	/** FMovieSceneTrackEditor Interface */
	virtual bool SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track ) override;
	virtual void AddKey( const FGuid& ObjectGuid, UObject* AdditionalAsset = NULL ) override;
	virtual void BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass) override;

	/** Add path */
	void AddPath(FGuid ObjectGuid, UObject* AdditionalAsset);

	/** Set path */
	void SetPath(UMovieSceneSection* Section, AActor* ActorWithSplineComponent);

private:

	/** Add path sub menu */
	void AddPathSubMenu(FMenuBuilder& MenuBuilder, FGuid ObjectBinding);

	/** Delegate for AnimatablePropertyChanged in AddKey */
	void AddKeyInternal(float KeyTime, const TArray<UObject*> Objects, UObject* AdditionalAsset);
};


