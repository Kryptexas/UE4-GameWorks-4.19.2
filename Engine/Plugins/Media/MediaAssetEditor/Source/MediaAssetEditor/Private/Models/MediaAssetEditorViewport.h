// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class FMediaAssetEditorViewport
	: public ISlateViewport
{
public:

	/**
	 * Default constructor.
	 */
	FMediaAssetEditorViewport( )
		: EditorTexture(nullptr)
		, SlateTexture(nullptr)
	{ }

	/**
	 * Destructor.
	 */
	~FMediaAssetEditorViewport( )
	{
		ReleaseResources();
	}

public:

	/**
	 * Initializes the viewport with the specified video track.
	 *
	 * @param VideoTrack The video track to render to the viewport.
	 */
	void Initialize( const IMediaTrackPtr& VideoTrack )
	{
		ReleaseResources();

		if (VideoTrack.IsValid())
		{
			check(VideoTrack->GetType() == EMediaTrackTypes::Video);

			const bool bCreateEmptyTexture = true;
			const FIntPoint VideoDimensions = VideoTrack->GetVideoDetails().GetDimensions();

			SlateTexture = new FSlateTexture2DRHIRef(VideoDimensions.X, VideoDimensions.Y, PF_B8G8R8A8, nullptr, TexCreate_Dynamic, bCreateEmptyTexture);
			EditorTexture = new FMediaAssetEditorTexture(SlateTexture, VideoTrack.ToSharedRef());

			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				RegisterMediaAssetEditorTexture,
				FMediaAssetEditorTexture*, EditorTextureParam, EditorTexture,
				{
					EditorTextureParam->Register();
				}
			);
		}
	}

protected:

	void ReleaseResources( )
	{
		if (SlateTexture != nullptr)
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( 
				ReleaseMediaAssetEditorResources,
				FMediaAssetEditorTexture*, EditorTextureParam, EditorTexture,
				FSlateTexture2DRHIRef*, SlateTextureParam, SlateTexture,
				{
					EditorTextureParam->Unregister();
					delete EditorTextureParam;

					SlateTextureParam->ReleaseResource();
					delete SlateTextureParam;
				}
			);

			EditorTexture = nullptr;
			SlateTexture = nullptr;
		}
	}

public:

	// ISlateViewport interface

	virtual FIntPoint GetSize( ) const override
	{
		return (SlateTexture != nullptr)
			? FIntPoint(SlateTexture->GetWidth(), SlateTexture->GetHeight())
			: FIntPoint();
	}

	virtual class FSlateShaderResource* GetViewportRenderTargetTexture( ) const override
	{
		return SlateTexture;
	}

	virtual bool RequiresVsync( ) const override
	{
		return false;
	}

private:

	// The texture being rendered on this viewport
	FMediaAssetEditorTexture* EditorTexture;

	// Pointer to the Slate texture being rendered.
	FSlateTexture2DRHIRef* SlateTexture;
};