// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "MovieSceneTrack.h"
#include "MovieSceneShotTrack.h"
#include "ScopedTransaction.h"
#include "ISequencerObjectChangeListener.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/Engine/Public/Slate/SlateTextures.h"
#include "ObjectTools.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "Runtime/MovieSceneTracks/Public/Sections/MovieSceneShotSection.h"
#include "IKeyArea.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneTrackEditor.h"
#include "ShotTrackEditor.h"
#include "CommonMovieSceneTools.h"
#include "Camera/CameraActor.h"

namespace AnimatableShotToolConstants
{
	// @todo Sequencer Perhaps allow this to be customizable
	const uint32 TrackHeight = 90;
	const uint32 TrackWidth = 90;
	const double ThumbnailFadeInDuration = 0.25f;

}

/////////////////////////////////////////////////
// FShotThumbnailPool

FShotThumbnailPool::FShotThumbnailPool(TSharedPtr<ISequencer> InSequencer, uint32 InMaxThumbnailsToDrawAtATime)
	: Sequencer(InSequencer)
	, ThumbnailsNeedingDraw()
	, MaxThumbnailsToDrawAtATime(InMaxThumbnailsToDrawAtATime)
{
}

void FShotThumbnailPool::AddThumbnailsNeedingRedraw(const TArray< TSharedPtr<FShotThumbnail> >& InThumbnails)
{
	ThumbnailsNeedingDraw.Append(InThumbnails);
}

void FShotThumbnailPool::RemoveThumbnailsNeedingRedraw(const TArray< TSharedPtr<FShotThumbnail> >& InThumbnails)
{
	for (int32 i = 0; i < InThumbnails.Num(); ++i)
	{
		ThumbnailsNeedingDraw.Remove(InThumbnails[i]);
	}
}

void FShotThumbnailPool::DrawThumbnails()
{
	if (ThumbnailsNeedingDraw.Num() > 0)
	{
		// save the global time
		Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(false);
		float SavedTime = Sequencer.Pin()->GetGlobalTime();

		uint32 ThumbnailsDrawn = 0;
		for (int32 ThumbnailsIndex = 0; ThumbnailsDrawn < MaxThumbnailsToDrawAtATime && ThumbnailsIndex < ThumbnailsNeedingDraw.Num(); ++ThumbnailsIndex)
		{
			TSharedPtr<FShotThumbnail> Thumbnail = ThumbnailsNeedingDraw[ThumbnailsIndex];
			
			if (Thumbnail->IsVisible())
			{
				Thumbnail->DrawThumbnail();
				++ThumbnailsDrawn;

				ThumbnailsNeedingDraw.Remove(Thumbnail);
			}
			else if (!Thumbnail->IsValid())
			{
				ensure(0);

				ThumbnailsNeedingDraw.Remove(Thumbnail);
			}
		}

		// restore the global time
		Sequencer.Pin()->SetGlobalTime(SavedTime);
		Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(true);
	}
}


/////////////////////////////////////////////////
// FShotThumbnail

FShotThumbnail::FShotThumbnail(TSharedPtr<FShotSection> InSection, TRange<float> InTimeRange)
	: OwningSection(InSection)
	, Texture(NULL)
	, TimeRange(InTimeRange)
	, FadeInCurve( 0.0f, AnimatableShotToolConstants::ThumbnailFadeInDuration )
{
	Texture = new FSlateTexture2DRHIRef(GetSize().X, GetSize().Y, PF_B8G8R8A8, NULL, TexCreate_Dynamic, true);

	BeginInitResource( Texture );

}

FShotThumbnail::~FShotThumbnail()
{
	BeginReleaseResource( Texture );

	FlushRenderingCommands();

	if (Texture) 
	{
		delete Texture;
	}
}

FIntPoint FShotThumbnail::GetSize() const {return FIntPoint(OwningSection.Pin()->GetThumbnailWidth(), AnimatableShotToolConstants::TrackHeight);}
bool FShotThumbnail::RequiresVsync() const {return false;}

FSlateShaderResource* FShotThumbnail::GetViewportRenderTargetTexture() const
{
	return Texture;
}

void FShotThumbnail::DrawThumbnail()
{
	OwningSection.Pin()->DrawViewportThumbnail(SharedThis(this));
	FadeInCurve.PlayReverse( OwningSection.Pin()->GetSequencerWidget() );
}

float FShotThumbnail::GetTime() const {return TimeRange.GetLowerBoundValue();}

void FShotThumbnail::CopyTextureIn(FSlateRenderTargetRHI* InTexture)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER( ReadTexture,
		FSlateRenderTargetRHI*, RenderTarget, InTexture,
		FSlateTexture2DRHIRef*, TargetTexture, Texture,
	{
		RHICmdList.CopyToResolveTarget(RenderTarget->GetRHIRef(), TargetTexture->GetTypedResource(), false, FResolveParams());
	});
}

float FShotThumbnail::GetFadeInCurve() const 
{
	return FadeInCurve.GetLerp();
}

bool FShotThumbnail::IsVisible() const
{
	return OwningSection.IsValid() ? TimeRange.Overlaps(OwningSection.Pin()->GetVisibleTimeRange()) : false;
}

bool FShotThumbnail::IsValid() const
{
	return OwningSection.IsValid();
}

/////////////////////////////////////////////////
// FShotSection

FShotSection::FShotSection( TSharedPtr<ISequencer> InSequencer, TSharedPtr<FShotThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection, UObject* InTargetObject )
	: Section( &InSection )
	, Sequencer( InSequencer )
	, Camera(Cast<ACameraActor>(InTargetObject))
	, ThumbnailPool( InThumbnailPool )
	, Thumbnails()
	, ThumbnailWidth(0)
	, StoredSize(ForceInit)
	, StoredStartTime(0.f)
	, VisibleTimeRange(TRange<float>::Empty())
	, InternalViewportScene(nullptr)
	, InternalViewportClient(nullptr)
{
	if (Camera.IsValid())
	{
		// @todo Sequencer optimize This code shouldn't even be called if this is offscreen
		// the following code could be generated on demand when the section is scrolled onscreen
		InternalViewportClient = MakeShareable(new FLevelEditorViewportClient(nullptr));
		InternalViewportClient->ViewportType = LVT_Perspective;
		InternalViewportClient->bDisableInput = true;
		InternalViewportClient->bDrawAxes = false;
		InternalViewportClient->EngineShowFlags = FEngineShowFlags(ESFIM_Game);
		InternalViewportClient->EngineShowFlags.DisableAdvancedFeatures();
		InternalViewportClient->SetActorLock(Camera.Get());

		InternalViewportScene = MakeShareable(new FSceneViewport(InternalViewportClient.Get(), nullptr));
		InternalViewportClient->Viewport = InternalViewportScene.Get();

		CalculateThumbnailWidthAndResize();
	}

	WhiteBrush = FEditorStyle::GetBrush("WhiteBrush");
}

FShotSection::~FShotSection()
{
	ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);

	if (InternalViewportClient.IsValid())
	{
		InternalViewportClient->Viewport = nullptr;
	}
}

UMovieSceneSection* FShotSection::GetSectionObject() 
{
	return Section;
}

FText FShotSection::GetSectionTitle() const
{
	return Cast<UMovieSceneShotSection>(Section)->GetTitle();
}

float FShotSection::GetSectionHeight() const
{
	return AnimatableShotToolConstants::TrackHeight+10;
}

FReply FShotSection::OnSectionDoubleClicked( const FGeometry& SectionGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		check( Section->IsA<UMovieSceneSection>() );

		Sequencer.Pin()->FilterToSelectedShotSections();
	}

	return FReply::Handled();
}

int32 FShotSection::OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const
{
	if (Camera.IsValid())
	{
		FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				WhiteBrush,
				SectionClippingRect,
				ESlateDrawEffect::None,
				FLinearColor::Black
			);

		int32 ThumbnailIndex = 0;
		for( const TSharedPtr<FShotThumbnail>& Thumbnail : Thumbnails )
		{
			FGeometry TruncatedGeometry = AllottedGeometry.MakeChild(
				Thumbnail->GetSize(),
				FSlateLayoutTransform(AllottedGeometry.Scale, FVector2D(ThumbnailIndex * ThumbnailWidth, 5.0f))
				);
			
			FSlateDrawElement::MakeViewport(
				OutDrawElements,
				LayerId,
				TruncatedGeometry.ToPaintGeometry(),
				Thumbnail,
				SectionClippingRect,
				false,
				false,
				ESlateDrawEffect::None,
				FLinearColor::White
			);

			if(Thumbnail->GetFadeInCurve() > 0.0f )
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					TruncatedGeometry.ToPaintGeometry(),
					WhiteBrush,
					SectionClippingRect,
					ESlateDrawEffect::None,
					FColor(255, 255, 255, Thumbnail->GetFadeInCurve() * 255)
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
			SectionClippingRect
		); 
	}

	return LayerId + 1;
}

void FShotSection::Tick( const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (!Camera.IsValid())
	{
		return;
	}

	FTimeToPixel TimeToPixelConverter( AllottedGeometry, TRange<float>( Section->GetStartTime(), Section->GetEndTime() ) );

	// get the visible time range
	VisibleTimeRange = TRange<float>(
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X),
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X + ParentGeometry.Size.X));

	FIntPoint AllocatedSize = AllottedGeometry.Size.IntPoint();
	float StartTime = Section->GetStartTime();

	if (!FSlateApplication::Get().HasAnyMouseCaptor() && ( AllocatedSize.X != StoredSize.X || !FMath::IsNearlyEqual(StartTime, StoredStartTime) ) )
	{
		RegenerateViewportThumbnails(AllocatedSize);
	}
}

uint32 FShotSection::GetThumbnailWidth() const
{
	return ThumbnailWidth;
}

void FShotSection::RegenerateViewportThumbnails(const FIntPoint& Size)
{
	StoredSize = Size;
	StoredStartTime = Section->GetStartTime();
	
	ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);
	Thumbnails.Empty();

	if (Size.X == 0 || Size.Y == 0)
	{
		return;
	}

	CalculateThumbnailWidthAndResize();

	int32 ThumbnailCount = FMath::DivideAndRoundUp(StoredSize.X, (int32)ThumbnailWidth);
	
	float StartTime = Section->GetStartTime();
	float EndTime = Section->GetEndTime();
	float DeltaTime = EndTime - StartTime;

	int32 TotalX = StoredSize.X;
	
	// @todo sequencer optimize this to use a single viewport and a single thumbnail
	// instead paste the textures into the thumbnail at specific offsets
	for (int32 i = 0; i < ThumbnailCount; ++i)
	{
		int32 StartX = i * ThumbnailWidth;
		float FractionThrough = (float)StartX / (float)TotalX;
		float Time = StartTime + DeltaTime * FractionThrough;

		int32 NextStartX = (i+1) * ThumbnailWidth;
		float NextFractionThrough = (float)NextStartX / (float)TotalX;
		float NextTime = FMath::Min(StartTime + DeltaTime * NextFractionThrough, EndTime);
	
		check(FractionThrough >= 0.f && FractionThrough <= 1.f && NextFractionThrough >= 0.f);
		TSharedPtr<FShotThumbnail> NewThumbnail = MakeShareable(new FShotThumbnail(SharedThis(this), TRange<float>(Time, NextTime)));

		Thumbnails.Add(NewThumbnail);
	}

	// @todo Sequencer Optimize Only say a thumbnail needs redraw if it is onscreen
	ThumbnailPool.Pin()->AddThumbnailsNeedingRedraw(Thumbnails);
}

void FShotSection::DrawViewportThumbnail(TSharedPtr<FShotThumbnail> ShotThumbnail)
{
	Sequencer.Pin()->SetGlobalTime(ShotThumbnail->GetTime());
	InternalViewportClient->UpdateViewForLockedActor();
			
	GWorld->SendAllEndOfFrameUpdates();
	InternalViewportScene->Draw(false);
	ShotThumbnail->CopyTextureIn((FSlateRenderTargetRHI*)InternalViewportScene->GetViewportRenderTargetTexture());

	FlushRenderingCommands();
}

void FShotSection::CalculateThumbnailWidthAndResize()
{
	// @todo Sequencer We also should detect when the property is changed or keyframed to update

	// get the aspect ratio at the first frame
	// if it changes over time, we ignore this
	Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(false);
	float SavedTime = Sequencer.Pin()->GetGlobalTime();
	
	Sequencer.Pin()->SetGlobalTime(Section->GetStartTime());
	uint32 OutThumbnailWidth = AnimatableShotToolConstants::TrackWidth * Camera->GetCameraComponent()->AspectRatio;

	// restore the global time
	Sequencer.Pin()->SetGlobalTime(SavedTime);
	Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(true);

	if (OutThumbnailWidth != ThumbnailWidth)
	{
		ThumbnailWidth = OutThumbnailWidth;
		InternalViewportScene->UpdateViewportRHI( false, ThumbnailWidth, AnimatableShotToolConstants::TrackHeight, EWindowMode::Windowed );
	}
}

/////////////////////////////////////////////////
// FShotTrackEditor

FShotTrackEditor::FShotTrackEditor( TSharedRef<ISequencer> InSequencer )
	: FMovieSceneTrackEditor( InSequencer ) 
{
	ThumbnailPool = MakeShareable(new FShotThumbnailPool(InSequencer));
}

FShotTrackEditor::~FShotTrackEditor()
{
}

TSharedRef<FMovieSceneTrackEditor> FShotTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FShotTrackEditor( InSequencer ) );
}

bool FShotTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneShotTrack::StaticClass();
}

TSharedRef<ISequencerSection> FShotTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );
	
	TArray<UObject*> OutObjects;
	// @todo Sequencer - Sub-MovieScenes The director track may be able to get cameras from sub-movie scenes
	GetSequencer()->GetRuntimeObjects( GetSequencer()->GetRootMovieSceneInstance(), Cast<const UMovieSceneShotSection>(&SectionObject)->GetCameraGuid(), OutObjects);
	check(OutObjects.Num() <= 1);
	TSharedRef<ISequencerSection> NewSection( new FShotSection( GetSequencer(), ThumbnailPool, SectionObject, OutObjects.Num() ? OutObjects[0] : NULL ) );

	return NewSection;
}

void FShotTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	TArray<UObject*> OutObjects;
	// @todo Sequencer - Sub-MovieScenes The director track may be able to get cameras from sub-movie scenes
	GetSequencer()->GetRuntimeObjects(  GetSequencer()->GetRootMovieSceneInstance(), ObjectGuid, OutObjects);
	bool bValidKey = OutObjects.Num() == 1 && OutObjects[0]->IsA<ACameraActor>();
	if (bValidKey)
	{
		AnimatablePropertyChanged( UMovieSceneShotTrack::StaticClass(), false, FOnKeyProperty::CreateRaw( this, &FShotTrackEditor::AddKeyInternal, ObjectGuid ) );
	}
}

void FShotTrackEditor::Tick(float DeltaTime)
{
	if (FSlateThrottleManager::Get().IsAllowingExpensiveTasks())
	{
		ThumbnailPool->DrawThumbnails();
	}
}

void FShotTrackEditor::BuildObjectBindingContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	if (ObjectClass->IsChildOf(ACameraActor::StaticClass()))
	{
		const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("Sequencer", "AddShot", "Add New Shot"),
			NSLOCTEXT("Sequencer", "AddShotTooltip", "Adds a new shot using this camera at the scrubber location."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(ParentSequencer.Get(), &ISequencer::AddNewShot, ObjectBinding))
			);
	}
}

void FShotTrackEditor::AddKeyInternal( float KeyTime, const FGuid ObjectGuid )
{
	UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieScene();

	UMovieSceneTrack* ShotTrack = MovieScene->GetShotTrack();

	if( !ShotTrack )
	{
		ShotTrack = MovieScene->AddShotTrack( UMovieSceneShotTrack::StaticClass() );
	}

	Cast<UMovieSceneShotTrack>(ShotTrack)->AddNewShot(ObjectGuid, KeyTime);
}
