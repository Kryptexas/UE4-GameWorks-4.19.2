// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "SInlineEditableTextBlock.h"


#define LOCTEXT_NAMESPACE "FThumbnailSection"


namespace ThumbnailSectionConstants
{
	const uint32 ThumbnailHeight = 90;
	const uint32 TrackWidth = 90;
	const float SectionGripSize = 4.0f;
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
		EMovieScenePlayerStatus::Type PlaybackStatus = SequencerPtr.Pin()->GetPlaybackStatus();
		SequencerPtr.Pin()->SetPlaybackStatus(EMovieScenePlayerStatus::Jumping);
		SequencerPtr.Pin()->SetGlobalTimeDirectly(TrackEditorThumbnail->GetTime());
		InternalViewportClient->SetActorLock(GetCameraObject());
		InternalViewportClient->UpdateViewForLockedActor();
		GWorld->SendAllEndOfFrameUpdates();
		InternalViewportScene->Draw(false);
		TrackEditorThumbnail->CopyTextureIn((FSlateRenderTargetRHI*)InternalViewportScene->GetViewportRenderTargetTexture());
		SequencerPtr.Pin()->SetPlaybackStatus(PlaybackStatus);
	}
}


uint32 FThumbnailSection::GetThumbnailWidth() const
{
	return ThumbnailWidth;
}


void FThumbnailSection::RegenerateViewportThumbnails(const FIntPoint& Size)
{
	AActor* CameraActor = GetCameraObject();

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
	
		UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromActor(CameraActor);
		float AspectRatio = CameraComponent ? CameraComponent->AspectRatio : 1.f;

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

bool FThumbnailSection::AreSectionsConnected() const
{
	return true;
}

TSharedRef<SWidget> FThumbnailSection::GenerateSectionWidget()
{
	return SNew(SBox)
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(GetContentPadding())
		[
			SNew(SInlineEditableTextBlock)
				.ToolTipText(CanRename() ? LOCTEXT("RenameThumbnail", "Click or hit F2 to rename") : FText::GetEmpty())
				.Text(this, &FThumbnailSection::HandleThumbnailTextBlockText)
				.ShadowOffset(FVector2D(1,1))
				.OnTextCommitted(this, &FThumbnailSection::HandleThumbnailTextBlockTextCommitted)
				.IsReadOnly(!CanRename())
		];
}


float FThumbnailSection::GetSectionGripSize() const
{
	return ThumbnailSectionConstants::SectionGripSize;
}


float FThumbnailSection::GetSectionHeight() const
{
	return ThumbnailSectionConstants::ThumbnailHeight;
}


UMovieSceneSection* FThumbnailSection::GetSectionObject() 
{
	return Section;
}


FText FThumbnailSection::GetSectionTitle() const
{
	return FText::GetEmpty();
}

int32 FThumbnailSection::OnPaintSection( FSequencerSectionPainter& InPainter ) const
{
	const ESlateDrawEffect::Type DrawEffects = InPainter.bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	int32 LayerId = InPainter.LayerId;

	if (Thumbnails.Num() != 0)
	{
		// @todo Sequencer: Need a way to visualize the key here
		FVector2D SectionSize = InPainter.SectionGeometry.GetLocalSize();

		int32 ThumbnailIndex = 0;
		for( const TSharedPtr<FTrackEditorThumbnail>& Thumbnail : Thumbnails )
		{
			FVector2D ThumbnailSize = Thumbnail->GetSize();

			FGeometry TruncatedGeometry = InPainter.SectionGeometry.MakeChild(
				ThumbnailSize,
				FSlateLayoutTransform(
					InPainter.SectionGeometry.Scale,
					FVector2D( ThumbnailIndex * ThumbnailWidth, (SectionSize.Y - ThumbnailSize.Y)*.5f)
				)
			);
			
			FSlateDrawElement::MakeViewport(
				InPainter.DrawElements,
				++LayerId,
				TruncatedGeometry.ToPaintGeometry(),
				Thumbnail,
				InPainter.SectionClippingRect,
				false,
				false,
				DrawEffects,
				FLinearColor::White
			);

			if(Thumbnail->GetFadeInCurve() > 0.0f )
			{
				FSlateDrawElement::MakeBox(
					InPainter.DrawElements,
					++LayerId,
					TruncatedGeometry.ToPaintGeometry(),
					WhiteBrush,
					InPainter.SectionClippingRect,
					DrawEffects,
					FLinearColor(1.0f, 1.0f, 1.0f, Thumbnail->GetFadeInCurve())
					);
			}

			++ThumbnailIndex;
		}
	}

	return LayerId;
}


void FThumbnailSection::Tick(const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// The sequencer view range which will be used to determine whether the individual thumbnails should be drawn if the time range intersects the view range
	VisibleTimeRange = SequencerPtr.Pin()->GetViewRange();

	FIntPoint AllocatedSize = AllottedGeometry.GetLocalSize().IntPoint();
	AllocatedSize.X = FMath::Max(AllocatedSize.X, 1);

	float StartTime = Section->GetStartTime();

	if (!FSlateApplication::Get().HasAnyMouseCaptor() && ( AllocatedSize.X != StoredSize.X || !FMath::IsNearlyEqual(StartTime, StoredStartTime) ) )
	{
		RegenerateViewportThumbnails(AllocatedSize);
	}
}

#undef LOCTEXT_NAMESPACE
