// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SInlineEditableTextBlock.h"


#define LOCTEXT_NAMESPACE "FThumbnailSection"


namespace ThumbnailSectionConstants
{
	const uint32 ThumbnailHeight = 90;
	const uint32 TrackHeight = ThumbnailHeight+10; // some extra padding
	const uint32 TrackWidth = 90;
	const float SectionGripSize = 4.0f;
	const FName ThumbnailGripBrushName = FName("Sequencer.Thumbnail.SectionHandle");
}


/* FThumbnailSection structors
 *****************************************************************************/

FThumbnailSection::FThumbnailSection(TSharedPtr<ISequencer> InSequencer, TSharedPtr<FTrackEditorThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection)
	: Section(&InSection)
	, SequencerPtr(InSequencer)
	, ThumbnailPool(InThumbnailPool)
	, Thumbnails()
	, ThumbnailWidth(0)
	, StoredSize(ForceInit)
	, StoredStartTime(0.f)
	, VisibleTimeRange(TRange<float>::Empty())
	, InternalViewportScene(nullptr)
	, InternalViewportClient(nullptr)
{
	// @todo Sequencer optimize This code shouldn't even be called if this is offscreen
	// the following code could be generated on demand when the section is scrolled onscreen
	InternalViewportClient = MakeShareable(new FLevelEditorViewportClient(nullptr));
	{
		InternalViewportClient->ViewportType = LVT_Perspective;
		InternalViewportClient->bDisableInput = true;
		InternalViewportClient->bDrawAxes = false;
		InternalViewportClient->EngineShowFlags = FEngineShowFlags(ESFIM_Game);
		InternalViewportClient->EngineShowFlags.DisableAdvancedFeatures();
		InternalViewportClient->SetRealtime( false );
	}

	InternalViewportScene = MakeShareable(new FSceneViewport(InternalViewportClient.Get(), nullptr));
	InternalViewportClient->Viewport = InternalViewportScene.Get();

	WhiteBrush = FEditorStyle::GetBrush("WhiteBrush");
}


FThumbnailSection::~FThumbnailSection()
{
	if (ThumbnailPool.IsValid())
	{
		ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);
	}
	
	if (InternalViewportClient.IsValid())
	{
		InternalViewportClient->Viewport = nullptr;
	}
}


/* FThumbnailSection interface
 *****************************************************************************/

void FThumbnailSection::DrawViewportThumbnail(TSharedPtr<FTrackEditorThumbnail> TrackEditorThumbnail)
{
	if (InternalViewportScene.IsValid())
	{
		SequencerPtr.Pin()->SetGlobalTime(TrackEditorThumbnail->GetTime());
		InternalViewportClient->SetActorLock(GetCameraObject());
		InternalViewportClient->UpdateViewForLockedActor();
		GWorld->SendAllEndOfFrameUpdates();
		InternalViewportScene->Draw(false);
		TrackEditorThumbnail->CopyTextureIn((FSlateRenderTargetRHI*)InternalViewportScene->GetViewportRenderTargetTexture());
	}
}


uint32 FThumbnailSection::GetThumbnailWidth() const
{
	return ThumbnailWidth;
}


void FThumbnailSection::RegenerateViewportThumbnails(const FIntPoint& Size)
{
	ACameraActor* CameraActor = GetCameraObject();

	if (CameraActor != nullptr && InternalViewportScene.IsValid())
	{
		ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);
		Thumbnails.Empty();

		if (Size.X == 0 || Size.Y == 0)
		{
			return;
		}

		StoredSize = Size;
		StoredStartTime = Section->GetStartTime();
	
		float AspectRatio = CameraActor->CameraComponent->AspectRatio;

		int32 NewThumbnailWidth = FMath::TruncToInt(ThumbnailSectionConstants::TrackWidth * AspectRatio);
		int32 ThumbnailCount = FMath::DivideAndRoundUp(StoredSize.X, NewThumbnailWidth);

		if (InternalViewportClient.IsValid())
		{
			InternalViewportClient->SetAllowCinematicPreview(false);
			InternalViewportClient->SetActorLock(CameraActor);
		}

		if (NewThumbnailWidth != ThumbnailWidth)
		{
			ThumbnailWidth = NewThumbnailWidth;
			InternalViewportScene->UpdateViewportRHI(false, ThumbnailWidth, ThumbnailSectionConstants::ThumbnailHeight, EWindowMode::Windowed);
		}

		float StartTime = Section->GetStartTime();
		float EndTime = Section->GetEndTime();
		float DeltaTime = EndTime - StartTime;

		int32 TotalX = StoredSize.X;

		// @todo sequencer optimize this to use a single viewport and a single thumbnail
		// instead paste the textures into the thumbnail at specific offsets
		for (int32 ThumbnailIndex = 0; ThumbnailIndex < ThumbnailCount; ++ThumbnailIndex)
		{
			int32 StartX = ThumbnailIndex * ThumbnailWidth;
			float FractionThrough = (float)StartX / (float)TotalX;
			float Time = StartTime + DeltaTime * FractionThrough;

			int32 NextStartX = (ThumbnailIndex+1) * ThumbnailWidth;
			float NextFractionThrough = (float)NextStartX / (float)TotalX;
			float NextTime = FMath::Min(StartTime + DeltaTime * NextFractionThrough, EndTime);

			FIntPoint ThumbnailSize(NextStartX-StartX, ThumbnailSectionConstants::ThumbnailHeight);

			check(FractionThrough >= 0.f && FractionThrough <= 1.f && NextFractionThrough >= 0.f);
			TSharedPtr<FTrackEditorThumbnail> NewThumbnail = MakeShareable(new FTrackEditorThumbnail(SharedThis(this), ThumbnailSize, TRange<float>(Time, NextTime)));

			Thumbnails.Add(NewThumbnail);
		}

		// @todo Sequencer Optimize Only say a thumbnail needs redraw if it is onscreen
		ThumbnailPool.Pin()->AddThumbnailsNeedingRedraw(Thumbnails);
	}
}


/* ISequencerSection interface
 *****************************************************************************/

bool FThumbnailSection::ShouldDrawKeyAreaBackground() const
{
	return false;
}

bool FThumbnailSection::AreSectionsConnected() const
{
	return true;
}

TSharedRef<SWidget> FThumbnailSection::GenerateSectionWidget()
{
	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(15.0f, 7.0f, 0.0f, 0.0f))
		[
			SNew(SInlineEditableTextBlock)
				.ToolTipText(CanRename() ? LOCTEXT("RenameThumbnail", "Click or hit F2 to rename") : FText::GetEmpty())
				.Text(this, &FThumbnailSection::HandleThumbnailTextBlockText)
				.ShadowOffset(FVector2D(1,1))
				.OnTextCommitted(this, &FThumbnailSection::HandleThumbnailTextBlockTextCommitted)
				.IsReadOnly(!CanRename())
		];
}


FName FThumbnailSection::GetSectionGripLeftBrushName() const
{
	return ThumbnailSectionConstants::ThumbnailGripBrushName;
}


FName FThumbnailSection::GetSectionGripRightBrushName() const
{
	return ThumbnailSectionConstants::ThumbnailGripBrushName;
}


float FThumbnailSection::GetSectionGripSize() const
{
	return ThumbnailSectionConstants::SectionGripSize;
}


float FThumbnailSection::GetSectionHeight() const
{
	return ThumbnailSectionConstants::TrackHeight;
}


UMovieSceneSection* FThumbnailSection::GetSectionObject() 
{
	return Section;
}


FText FThumbnailSection::GetSectionTitle() const
{
	return FText::GetEmpty();
}


int32 FThumbnailSection::OnPaintSection(const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled) const
{
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	if (Thumbnails.Num() != 0)
	{
		FGeometry ThumbnailAreaGeometry = AllottedGeometry.MakeChild( FVector2D(GetSectionGripSize(), 0.0f), AllottedGeometry.GetDrawSize() - FVector2D( GetSectionGripSize()*2, 0.0f ) );

		FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				ThumbnailAreaGeometry.ToPaintGeometry(),
				FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
				SectionClippingRect,
				DrawEffects
			);

		// @todo Sequencer: Need a way to visualize the key here

		int32 ThumbnailIndex = 0;
		for( const TSharedPtr<FTrackEditorThumbnail>& Thumbnail : Thumbnails )
		{
			FGeometry TruncatedGeometry = ThumbnailAreaGeometry.MakeChild(
				Thumbnail->GetSize(),
				FSlateLayoutTransform(ThumbnailAreaGeometry.Scale, FVector2D( ThumbnailIndex * ThumbnailWidth, 5.f))
				);
			
			FSlateDrawElement::MakeViewport(
				OutDrawElements,
				LayerId,
				TruncatedGeometry.ToPaintGeometry(),
				Thumbnail,
				SectionClippingRect,
				false,
				false,
				DrawEffects,
				FLinearColor::White
			);

			if(Thumbnail->GetFadeInCurve() > 0.0f )
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId+1,
					TruncatedGeometry.ToPaintGeometry(),
					WhiteBrush,
					SectionClippingRect,
					DrawEffects,
					FLinearColor(1.0f, 1.0f, 1.0f, Thumbnail->GetFadeInCurve())
					);
			}

			++ThumbnailIndex;
		}
	}
	else
	{
		FSlateDrawElement::MakeBox( 
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FEditorStyle::GetBrush("Sequencer.GenericSection.Background"),
			SectionClippingRect,
			DrawEffects
		); 
	}

	return LayerId + 2;
}


void FThumbnailSection::Tick(const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime)
{
	FTimeToPixel TimeToPixelConverter(AllottedGeometry, TRange<float>(Section->GetStartTime(), Section->GetEndTime()));

	// get the visible time range
	VisibleTimeRange = TRange<float>(
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X),
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X + ParentGeometry.Size.X)
	);

	FIntPoint AllocatedSize = AllottedGeometry.MakeChild( FVector2D( GetSectionGripSize(), 0.0f ), FVector2D( AllottedGeometry.Size.X - (GetSectionGripSize()*2), AllottedGeometry.Size.Y) ).Size.IntPoint();
	AllocatedSize.X = FMath::Max(AllocatedSize.X, 1);

	float StartTime = Section->GetStartTime();

	if (!FSlateApplication::Get().HasAnyMouseCaptor() && ( AllocatedSize.X != StoredSize.X || !FMath::IsNearlyEqual(StartTime, StoredStartTime) ) )
	{
		RegenerateViewportThumbnails(AllocatedSize);
	}
}

#undef LOCTEXT_NAMESPACE
