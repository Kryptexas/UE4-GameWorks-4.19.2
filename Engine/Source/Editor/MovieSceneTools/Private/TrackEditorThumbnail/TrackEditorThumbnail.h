// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


class FThumbnailSection;
class FSlateRenderTargetRHI;
class FSlateTexture2DRHIRef;


/**
 * Track Editor Thumbnail, which keeps a Texture to be displayed by a viewport.
 */
class FTrackEditorThumbnail
	: public ISlateViewport
	, public TSharedFromThis<FTrackEditorThumbnail>
{
public:

	/** Create and initialize a new instance. */
	FTrackEditorThumbnail(TSharedPtr<FThumbnailSection> InSection, const FIntPoint& InSize, TRange<float> InTimeRange);

	/** Virtual destructor. */
	virtual ~FTrackEditorThumbnail();

public:

	/** Copies the incoming render target to this thumbnails texture. */
	void CopyTextureIn(FSlateRenderTargetRHI* InTexture);

	/** Renders the thumbnail to the texture. */
	void DrawThumbnail();

	/** Gets the curve for fading in the thumbnail. */
	float GetFadeInCurve() const;

	/** Gets the time that this thumbnail is a rendering of. */
	float GetTime() const;

	bool IsValid() const;

	/** Returns whether this thumbnail is visible based on the thumbnail section geometry visibility. */
	bool IsVisible() const;

public:

	// ISlateViewport interface

	virtual FIntPoint GetSize() const override;
	virtual FSlateShaderResource* GetViewportRenderTargetTexture() const override;
	virtual bool RequiresVsync() const override;

private:

	/** Parent thumbnail section we are a thumbnail of. */
	TWeakPtr<FThumbnailSection> OwningSection;
	
	FIntPoint Size;

	/** The Texture RHI that holds the thumbnail. */
	FSlateTexture2DRHIRef* Texture;

	/** Where in time this thumbnail is a rendering of. */
	TRange<float> TimeRange;

	/** Fade curve to display while the thumbnail is redrawing. */
	FCurveSequence FadeInCurve;
};
