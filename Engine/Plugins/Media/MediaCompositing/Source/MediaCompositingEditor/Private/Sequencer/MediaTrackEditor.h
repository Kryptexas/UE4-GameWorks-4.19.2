// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimatedPropertyKey.h"
#include "AssetData.h"
#include "MovieSceneTrackEditor.h"
#include "Templates/SharedPointer.h"

class ISequencer;
class FTrackEditorThumbnailPool;
class UMediaSource;
class UMovieSceneMediaTrack;


/**
 * Track editor that understands how to animate MediaPlayer properties on objects
 */
class FMediaTrackEditor
	: public FMovieSceneTrackEditor
{
public:

	/**
	 * Create a new media track editor instance.
	 *
	 * @param OwningSequencer The sequencer object that will own the track editor.
	 * @return The new track editor.
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer)
	{
		return MakeShared<FMediaTrackEditor>(OwningSequencer);
	}

	/**
	 * Get the list of all property types that this track editor animates.
	 *
	 * @return List of animated properties.
	 */
	static TArray<FAnimatedPropertyKey, TInlineAllocator<1>> GetAnimatedPropertyTypes();

public:

	/**
	 * Create and initialize a new instance.
	 *
	 * @param InSequencer The sequencer object that owns this track editor.
	 */
	FMediaTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Virtual destructor. */
	virtual ~FMediaTrackEditor();

public:

	//~ FMovieSceneTrackEditor interface

	virtual UMovieSceneTrack* AddTrack(UMovieScene* FocusedMovieScene, const FGuid& ObjectHandle, TSubclassOf<class UMovieSceneTrack> TrackClass, FName UniqueTypeName) override;
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> TrackClass) const override;
	virtual void Tick(float DeltaTime) override;
	virtual const FSlateBrush* GetIconBrush() const override;

protected:

	/** Callback for AnimatablePropertyChanged in HandleAssetAdded for attached media sources. */
	FKeyPropertyResult AddAttachedMediaSource(float KeyTime, class UMediaSource* MediaSource, TArray<TWeakObjectPtr<UObject>> ObjectsToAttachTo);

	/** Callback for AnimatablePropertyChanged in HandleAssetAdded for master media sources. */
	FKeyPropertyResult AddMasterMediaSource(float KeyTime, class UMediaSource* MediaSource);

	void AddNewSection(const FAssetData& Asset, UMovieSceneMediaTrack* Track);

private:

	/** Callback for executing the "Add Media Track" menu entry. */
	void HandleAddMediaTrackMenuEntryExecute();

private:

	TSharedPtr<FTrackEditorThumbnailPool> ThumbnailPool;
};
