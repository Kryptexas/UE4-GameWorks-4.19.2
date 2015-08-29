// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "MovieSceneTrack.h"
#include "MovieSceneShotTrack.h"
#include "ScopedTransaction.h"
#include "ISequencerObjectChangeListener.h"
#include "ISectionLayoutBuilder.h"
#include "Runtime/Engine/Public/Slate/SlateTextures.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"
#include "Runtime/MovieSceneTracks/Public/Sections/MovieSceneShotSection.h"
#include "IKeyArea.h"
#include "MovieSceneToolHelpers.h"
#include "MovieSceneTrackEditor.h"
#include "ShotTrackEditor.h"
#include "CommonMovieSceneTools.h"
#include "Camera/CameraActor.h"
#include "AssetToolsModule.h"
#include "SInlineEditableTextBlock.h"

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
		float SavedTime = Sequencer.Pin()->GetGlobalTime();

		uint32 ThumbnailsDrawn = 0;
		for (int32 ThumbnailsIndex = 0; ThumbnailsDrawn < MaxThumbnailsToDrawAtATime && ThumbnailsIndex < ThumbnailsNeedingDraw.Num(); ++ThumbnailsIndex)
		{
			TSharedPtr<FShotThumbnail> Thumbnail = ThumbnailsNeedingDraw[ThumbnailsIndex];
			
			if (Thumbnail->IsVisible())
			{
			
				Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(false);
			
				Thumbnail->DrawThumbnail();

			
				Sequencer.Pin()->SetPerspectiveViewportPossessionEnabled(true);
				++ThumbnailsDrawn;

				ThumbnailsNeedingDraw.Remove(Thumbnail);
			}
			else if (!Thumbnail->IsValid())
			{
				ensure(0);

				ThumbnailsNeedingDraw.Remove(Thumbnail);
			}
		}
		
		if( ThumbnailsDrawn > 0 )
		{
			// restore the global time
			Sequencer.Pin()->SetGlobalTime(SavedTime);
		}

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

FShotSection::FShotSection( TSharedPtr<ISequencer> InSequencer, TSharedPtr<FShotThumbnailPool> InThumbnailPool, UMovieSceneSection& InSection )
	: Section( CastChecked<UMovieSceneShotSection>( &InSection ) )
	, Sequencer( InSequencer )
	, ThumbnailPool( InThumbnailPool )
	, Thumbnails()
	, ThumbnailWidth(0)
	, StoredSize(ForceInit)
	, StoredStartTime(0.f)
	, VisibleTimeRange(TRange<float>::Empty())
	, InternalViewportScene(nullptr)
	, InternalViewportClient(nullptr)
{

	Camera = UpdateCameraObject();

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

TSharedRef<SWidget> FShotSection::GenerateSectionWidget()
{
	return 
		SNew( SBox )
		.HAlign( HAlign_Left )
		.VAlign( VAlign_Top )
		.Padding( FMargin( 15.0f, 7.0f, 0.0f, 0.0f ) )
		[
			SNew( SInlineEditableTextBlock )
			.ToolTipText( NSLOCTEXT("FShotTrackEditor", "RenameShot", "The name of this shot.  Click or hit F2 to rename") )
			.Text( this, &FShotSection::GetShotName )
			.ShadowOffset( FVector2D(1,1) )
			.OnTextCommitted(this, &FShotSection::OnRenameShot )
		];
}

FText FShotSection::GetSectionTitle() const
{
	return FText::GetEmpty();
}

float FShotSection::GetSectionHeight() const
{
	return AnimatableShotToolConstants::TrackHeight+10;
}

FReply FShotSection::OnSectionDoubleClicked( const FGeometry& SectionGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		TSharedRef<FMovieSceneInstance> MovieSceneInstance = Sequencer.Pin()->GetInstanceForSubMovieSceneSection( *Section );

		Sequencer.Pin()->FocusSubMovieScene( MovieSceneInstance );
	}

	return FReply::Handled();
}

void FShotSection::BuildSectionContextMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("FShotTrackEditor", "FilterToShots", "Filter To Shots"),
		NSLOCTEXT("FShotTrackEditor", "FilterToShotsToolTip", "Filters to the selected shot sections"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FShotSection::FilterToSelectedShotSections, true))
		);
}


void FShotSection::FilterToSelectedShotSections(bool bZoomToShotBounds)
{
	Sequencer.Pin()->FilterToSelectedShotSections(bZoomToShotBounds);
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
		Camera = UpdateCameraObject();
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

ACameraActor* FShotSection::UpdateCameraObject() const
{
	TArray<UObject*> OutObjects;
	// @todo Sequencer - Sub-MovieScenes The director track may be able to get cameras from sub-movie scenes
	Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetRootMovieSceneInstance(),  Section->GetCameraGuid(), OutObjects);

	ACameraActor* ReturnCam = nullptr;
	if( OutObjects.Num() > 0 )
	{
		ReturnCam = Cast<ACameraActor>( OutObjects[0] );
	}

	return ReturnCam;
}

FText FShotSection::GetShotName() const
{
	return Section->GetShotDisplayName();
}

void FShotSection::OnRenameShot( const FText& NewShotName, ETextCommit::Type CommitType )
{
	if (CommitType == ETextCommit::OnEnter && !GetShotName().EqualTo( NewShotName ) )
	{
		Section->Modify();
		Section->SetShotNameAndNumber( NewShotName, Section->GetShotNumber() );

		UMovieScene* ShotMovieScene = Section->GetMovieScene();
		if( ShotMovieScene )
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

			TArray<FAssetRenameData> AssetsAndNames;
			const FString PackagePath = FPackageName::GetLongPackagePath(ShotMovieScene->GetOutermost()->GetName());

			// We want to prepend the owning movie scene name.  This is not the movie scene inside referenced by the shot but the movie scene containing 
			// the shot track
			UMovieScene* OwningMovieScene = Section->GetTypedOuter<UMovieScene>();
			const FString NewAssetName = OwningMovieScene->GetName() + TEXT("_") + NewShotName.ToString();

			new (AssetsAndNames) FAssetRenameData(ShotMovieScene, PackagePath, NewAssetName );

			AssetToolsModule.Get().RenameAssets(AssetsAndNames);
		}
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

	TSharedRef<ISequencerSection> NewSection( new FShotSection( GetSequencer(), ThumbnailPool, SectionObject ) );

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
			NSLOCTEXT("FShotTrackEditor", "AddShot", "Add New Shot"),
			NSLOCTEXT("FShotTrackEditor", "AddShotTooltip", "Adds a new shot using this camera at the scrubber location."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(ParentSequencer.Get(), &ISequencer::AddNewShot, ObjectBinding))
			);
	}
}

void FShotTrackEditor::AddKeyInternal( float KeyTime, const FGuid ObjectGuid )
{
	UMovieScene* MovieScene = GetSequencer()->GetFocusedMovieScene();

	UMovieSceneTrack* Track = MovieScene->GetShotTrack();
	if( !Track )
	{
		Track = MovieScene->AddShotTrack( UMovieSceneShotTrack::StaticClass() );
	}

	UMovieSceneShotTrack* ShotTrack = CastChecked<UMovieSceneShotTrack>( Track );

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

	// Get the current path of the movie scene the shot is in
	const FString LongPackagePath = FPackageName::GetLongPackagePath(ShotTrack->GetOutermost()->GetPathName());

	const TArray<UMovieSceneSection*>& AllSections = ShotTrack->GetAllSections();

	// Find where the section will be inserted
	int32 SectionIndex = FindIndexForNewShot( AllSections, KeyTime );

	const int32 ShotNumber = GenerateShotNumber( *ShotTrack, SectionIndex );

	FString ShotName = FString::Printf( TEXT("Shot_%04d"), ShotNumber );

	const FString NewShotPackagePath = LongPackagePath + TEXT("/") + MovieScene->GetName() + TEXT("_") + ShotName;

	const FString EmptySuffix(TEXT(""));

	// Create a package name and asset name for the new movie scene
	FString PackageName;
	FString AssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(NewShotPackagePath, EmptySuffix, PackageName, AssetName);
	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UMovieScene::StaticClass(), nullptr );
	
	float EndTime = KeyTime;
	if( AllSections.IsValidIndex( SectionIndex + 1 ) )
	{
		// The new section ends where the next section begins
		EndTime = AllSections[SectionIndex+1]->GetStartTime();
	}
	else
	{
		// There is no section after this one so the new section should be the length of the movie scene
		EndTime = MovieScene->GetTimeRange().GetUpperBoundValue();
	}


	if (AllSections.IsValidIndex(SectionIndex - 1))
	{
		// Shift the previous shot's end time so that it ends when the new shot starts
		AllSections[SectionIndex - 1]->SetEndTime(KeyTime);
	}

	if (AllSections.IsValidIndex(SectionIndex + 1))
	{
		// Shift the next shot's start time so that it starts when the new shot ends
		AllSections[SectionIndex + 1]->SetStartTime(EndTime);
	}

	TRange<float> TimeRange( KeyTime, EndTime );
	ShotTrack->AddNewShot( ObjectGuid, *CastChecked<UMovieScene>( NewAsset ), TimeRange, FText::FromString( ShotName ), ShotNumber );
}

int32 FShotTrackEditor::GenerateShotNumber( UMovieSceneShotTrack& ShotTrack, int32 SectionIndex ) const
{
	const TArray<UMovieSceneSection*>& AllSections = ShotTrack.GetAllSections();

	const int32 Interval = 10;
	int32 ShotNum = Interval;
	int32 LastKeyIndex = AllSections.Num() - 1;

	int32 PrevShotNum = 0;
	//get the preceding shot number if any
	if (SectionIndex > 0)
	{
		PrevShotNum = CastChecked<UMovieSceneShotSection>( AllSections[SectionIndex - 1] )->GetShotNumber();
	}

	if (SectionIndex < LastKeyIndex)
	{
		//we're inserting before something before the first frame
		int32 NextShotNum = CastChecked<UMovieSceneShotSection>( AllSections[SectionIndex + 1] )->GetShotNumber();
		if (NextShotNum == 0)
		{
			NextShotNum = PrevShotNum + (Interval * 2);
		}

		if (NextShotNum > PrevShotNum)
		{
			//find a midpoint if we're in order

			//try to stick to the nearest interval if possible
			int32 NearestInterval = PrevShotNum - (PrevShotNum % Interval) + Interval;
			if (NearestInterval > PrevShotNum && NearestInterval < NextShotNum)
			{
				ShotNum = NearestInterval;
			}
			//else find the exact mid point
			else
			{
				ShotNum = ((NextShotNum - PrevShotNum) / 2) + PrevShotNum;
			}
		}
		else
		{
			//Just use the previous shot number + 1 with we're out of order
			ShotNum = PrevShotNum + 1;
		}
	}
	else
	{
		//we're adding to the end of the track
		ShotNum = PrevShotNum + Interval;
	}

	return ShotNum;
}

int32 FShotTrackEditor::FindIndexForNewShot( const TArray<UMovieSceneSection*>& ShotSections, float NewShotTime ) const
{
	int32 SectionIndex = 0;
	for (SectionIndex = 0; SectionIndex < ShotSections.Num() && ShotSections[SectionIndex]->GetStartTime() < NewShotTime; ++SectionIndex);

	return SectionIndex;
}