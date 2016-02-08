// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FThumbnailSection;
class FTrackEditorThumbnail;
class FTrackEditorThumbnailPool;
class IMenu;
class ISectionLayoutBuilder;
class UMovieSceneCameraCutSection;


/**
 * CameraCut section, which paints and ticks the appropriate section.
 */
class FCameraCutSection
	: public FThumbnailSection
{
public:

	/** Create and initialize a new instance. */
	FCameraCutSection(TSharedPtr<ISequencer> InSequencer, TSharedPtr<FTrackEditorThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection);

	/** Virtual destructor. */
	virtual ~FCameraCutSection();

public:

	// ISequencerSection interface

	virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding) override;
	virtual FText GetDisplayName() const override;

	// FThumbnail interface

	virtual ACameraActor* GetCameraObject() const override;
	virtual FText HandleThumbnailTextBlockText() const override;

private:

	/** Callback for executing a "Set Camera" menu entry in the context menu. */
	void HandleSetCameraMenuEntryExecute(AActor* InCamera);

};
