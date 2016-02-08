// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "Runtime/Engine/Public/Slate/SlateTextures.h"


namespace TrackEditorThumbnailConstants
{
	const double ThumbnailFadeInDuration = 0.25f;
}


/* FTrackEditorThumbnail structors
 *****************************************************************************/

FTrackEditorThumbnail::FTrackEditorThumbnail(TSharedPtr<FThumbnailSection> InSection, const FIntPoint& InSize, TRange<float> InTimeRange)
	: OwningSection(InSection)
	, Size(InSize)
	, Texture(nullptr)
	, TimeRange(InTimeRange)
	, FadeInCurve(0.0f, TrackEditorThumbnailConstants::ThumbnailFadeInDuration)
{
	Texture = new FSlateTexture2DRHIRef(GetSize().X, GetSize().Y, PF_B8G8R8A8, nullptr, TexCreate_Dynamic, true);

	BeginInitResource( Texture );

}


FTrackEditorThumbnail::~FTrackEditorThumbnail()
{
	BeginReleaseResource(Texture);
	FlushRenderingCommands();

	if (Texture) 
	{
		delete Texture;
	}
}


/* FTrackEditorThumbnail interface
 *****************************************************************************/

void FTrackEditorThumbnail::CopyTextureIn(FSlateRenderTargetRHI* InTexture)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(ReadTexture,
		FSlateRenderTargetRHI*, RenderTarget, InTexture,
		FSlateTexture2DRHIRef*, TargetTexture, Texture,
	{
		RHICmdList.CopyToResolveTarget(RenderTarget->GetRHIRef(), TargetTexture->GetTypedResource(), false, FResolveParams());
	});
}


void FTrackEditorThumbnail::DrawThumbnail()
{
	OwningSection.Pin()->DrawViewportThumbnail(SharedThis(this));
	FadeInCurve.PlayReverse( OwningSection.Pin()->GetSequencerWidget() );
}


float FTrackEditorThumbnail::GetFadeInCurve() const 
{
	return FadeInCurve.GetLerp();
}


float FTrackEditorThumbnail::GetTime() const
{
	return TimeRange.GetLowerBoundValue();
}


bool FTrackEditorThumbnail::IsValid() const
{
	return OwningSection.IsValid();
}


bool FTrackEditorThumbnail::IsVisible() const
{
	return OwningSection.IsValid()
		? TimeRange.Overlaps(OwningSection.Pin()->GetVisibleTimeRange())
		: false;
}


/* ISlateViewport interface
 *****************************************************************************/

FIntPoint FTrackEditorThumbnail::GetSize() const
{
	return Size;
}


FSlateShaderResource* FTrackEditorThumbnail::GetViewportRenderTargetTexture() const
{
	return Texture;
}


bool FTrackEditorThumbnail::RequiresVsync() const 
{
	return false;
}
