// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerEditorPrivatePCH.h"
#include "IMediaVideoTrack.h"


/* FMediaPlayerEditorViewport structors
*****************************************************************************/

FMediaPlayerEditorViewport::FMediaPlayerEditorViewport()
: EditorTexture(nullptr)
, SlateTexture(nullptr)
{ }


FMediaPlayerEditorViewport::~FMediaPlayerEditorViewport()
{
	ReleaseResources();
}


/* FMediaPlayerEditorViewport interface
*****************************************************************************/

void FMediaPlayerEditorViewport::Initialize(const IMediaVideoTrackPtr& VideoTrack)
{
	// delete old editor texture
	if (EditorTexture != nullptr)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			DestroyMediaPlayerEditorTexture,
			FMediaPlayerEditorTexture*, EditorTextureParam, EditorTexture,
			{
				EditorTextureParam->Unregister();
				delete EditorTextureParam;
			}
		);

		EditorTexture = nullptr;
	}

	// create or resize slate texture
	if (SlateTexture != nullptr)
	{
		const FIntPoint VideoDimensions = VideoTrack.IsValid() ? VideoTrack->GetDimensions() : FIntPoint::ZeroValue;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			ResizeMediaPlayerEditorTexture,
			FSlateTexture2DRHIRef*, SlateTextureParam, SlateTexture,
			FIntPoint, DimensionsParam, VideoDimensions,
			{
				SlateTextureParam->Resize(DimensionsParam.X, DimensionsParam.Y);
			}
		);
	}
	else if (VideoTrack.IsValid())
	{
		const FIntPoint VideoDimensions = VideoTrack->GetDimensions();
		SlateTexture = new FSlateTexture2DRHIRef(VideoDimensions.X, VideoDimensions.Y, PF_B8G8R8A8, nullptr, TexCreate_Dynamic | TexCreate_RenderTargetable, true /*bCreateEmptyTexture*/);
	}

	// create new editor texture
	if (VideoTrack.IsValid())
	{
		EditorTexture = new FMediaPlayerEditorTexture(SlateTexture, VideoTrack.ToSharedRef());

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			RegisterMediaPlayerEditorTexture,
			FMediaPlayerEditorTexture*, EditorTextureParam, EditorTexture,
			{
				EditorTextureParam->Register();
			}
		);
	}
}


/* ISlateViewport interface
*****************************************************************************/

FIntPoint FMediaPlayerEditorViewport::GetSize() const
{
	return (SlateTexture != nullptr)
		? FIntPoint(SlateTexture->GetWidth(), SlateTexture->GetHeight())
		: FIntPoint();
}


class FSlateShaderResource* FMediaPlayerEditorViewport::GetViewportRenderTargetTexture() const
{
	return SlateTexture;
}


bool FMediaPlayerEditorViewport::RequiresVsync() const
{
	return false;
}


/* FMediaPlayerEditorViewport implementation
*****************************************************************************/

void FMediaPlayerEditorViewport::ReleaseResources()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		ReleaseMediaPlayerEditorResources,
		FMediaPlayerEditorTexture*, EditorTextureParam, EditorTexture,
		FSlateTexture2DRHIRef*, SlateTextureParam, SlateTexture,
		{
			if (EditorTextureParam != nullptr)
			{
				EditorTextureParam->Unregister();
				delete EditorTextureParam;
			}

			if (SlateTextureParam != nullptr)
			{
				SlateTextureParam->ReleaseResource();
				delete SlateTextureParam;
			}
		}
	);

	EditorTexture = nullptr;
	SlateTexture = nullptr;
}
