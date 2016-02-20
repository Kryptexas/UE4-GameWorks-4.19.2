// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


class FTrackEditorThumbnail;
class FTrackEditorThumbnailPool;
class IMenu;
class ISectionLayoutBuilder;
class UMovieSceneSection;


/**
 * Thumbnail section, which paints and ticks the appropriate section.
 */
class FThumbnailSection
	: public ISequencerSection
	, public TSharedFromThis<FThumbnailSection>
{
public:

	/** Create and initialize a new instance. */
	FThumbnailSection(TSharedPtr<ISequencer> InSequencer, TSharedPtr<FTrackEditorThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection);

	/** Virtual destructor. */
	virtual ~FThumbnailSection();

public:

	/** Draws the passed in viewport thumbnail and copies it to the thumbnail's texture. */
	void DrawViewportThumbnail(TSharedPtr<FTrackEditorThumbnail> TrackEditorThumbnail);

	/** @return The sequencer widget owning the MovieScene section. */
	TSharedRef<SWidget> GetSequencerWidget()
	{
		return SequencerPtr.Pin()->GetSequencerWidget();
	}

	/** Gets the thumbnail width. */
	uint32 GetThumbnailWidth() const;

	/** Gets the time range of what in the sequencer is visible. */
	TRange<float> GetVisibleTimeRange() const
	{
		return VisibleTimeRange;
	}

	/** Regenerates all viewports and thumbnails at the new size. */
	void RegenerateViewportThumbnails(const FIntPoint& Size);

	/** Get the camera that this thumbnail should draw from */
	virtual AActor* GetCameraObject() const { return nullptr; }

	/** Get whether the text is renameable */
	virtual bool CanRename() const { return false; }

	/** Callback for getting the text of the track name text block. */
	virtual FText HandleThumbnailTextBlockText() const { return FText::GetEmpty(); }

	/** Callback for when the text of the track name text block has changed. */
	virtual void HandleThumbnailTextBlockTextCommitted(const FText& NewThumbnailName, ETextCommit::Type CommitType) { }

public:

	// ISequencerSection interface

	virtual bool AreSectionsConnected() const override;
	virtual void GenerateSectionLayout(ISectionLayoutBuilder& LayoutBuilder) const override { }
	virtual TSharedRef<SWidget> GenerateSectionWidget() override;
	virtual float GetSectionGripSize() const override;
	virtual float GetSectionHeight() const override;
	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual int32 OnPaintSection( FSequencerSectionPainter& InPainter ) const override;
	virtual void Tick( const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime ) override;

protected:

	/** The section we are visualizing. */
	UMovieSceneSection* Section;

	/** The parent sequencer we are a part of. */
	TWeakPtr<ISequencer> SequencerPtr;

	/** The thumbnail pool that we are sending all of our thumbnails to. */
	TWeakPtr<FTrackEditorThumbnailPool> ThumbnailPool;

	/** A list of all thumbnails this CameraCut section has. */
	TArray<TSharedPtr<FTrackEditorThumbnail>> Thumbnails;

	/** The width of our thumbnails. */
	uint32 ThumbnailWidth;

	/** The stored size of this section in the Slate geometry. */
	FIntPoint StoredSize;

	/** The stored start time, to query for invalidations. */
	float StoredStartTime;

	/** Cached Time Range of the visible parent section area. */
	TRange<float> VisibleTimeRange;
	
	/** An internal viewport scene we use to render the thumbnails with. */
	TSharedPtr<FSceneViewport> InternalViewportScene;

	/** An internal editor viewport client to render the thumbnails with. */
	TSharedPtr<FLevelEditorViewportClient> InternalViewportClient;
	
	/** Fade brush. */
	const FSlateBrush* WhiteBrush;
};
