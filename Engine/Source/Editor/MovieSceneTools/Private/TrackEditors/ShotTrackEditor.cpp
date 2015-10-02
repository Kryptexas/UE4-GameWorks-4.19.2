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


#define LOCTEXT_NAMESPACE "FEventTrackEditor"


namespace ShotTrackConstants
{
	// @todo Sequencer Perhaps allow this to be customizable
	const uint32 ThumbnailHeight = 90;
	const uint32 TrackHeight = ThumbnailHeight+10; // some extra padding
	const uint32 TrackWidth = 90;
	const double ThumbnailFadeInDuration = 0.25f;
	const float SectionGripSize = 4.0f;
	const FName ShotTrackGripBrushName = FName("Sequencer.ShotTrack.SectionHandle");

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
			// Ensure all buffers are updated
			FlushRenderingCommands();

			// restore the global time
			Sequencer.Pin()->SetGlobalTime(SavedTime);
		}

	}
}


/////////////////////////////////////////////////
// FShotThumbnail

FShotThumbnail::FShotThumbnail(TSharedPtr<FShotSection> InSection, const FIntPoint& InSize, TRange<float> InTimeRange)
	: OwningSection(InSection)
	, Size(InSize)
	, Texture(NULL)
	, TimeRange(InTimeRange)
	, FadeInCurve( 0.0f, ShotTrackConstants::ThumbnailFadeInDuration )
{
	Texture = new FSlateTexture2DRHIRef(GetSize().X, GetSize().Y, PF_B8G8R8A8, nullptr, TexCreate_Dynamic, true);

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

FIntPoint FShotThumbnail::GetSize() const
{
	return Size;
}
bool FShotThumbnail::RequiresVsync() const 
{
	return false;
}

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
		InternalViewportClient->SetAllowCinematicPreview( false );
		InternalViewportClient->SetRealtime( false );

		InternalViewportScene = MakeShareable(new FSceneViewport(InternalViewportClient.Get(), nullptr));
		InternalViewportClient->Viewport = InternalViewportScene.Get();
	}

	WhiteBrush = FEditorStyle::GetBrush("WhiteBrush");
}

FShotSection::~FShotSection()
{
	if( ThumbnailPool.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);
	}
	
	if(InternalViewportClient.IsValid())
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
			.ToolTipText(LOCTEXT("RenameShot", "The name of this shot.  Click or hit F2 to rename"))
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
	return ShotTrackConstants::TrackHeight;
}

float FShotSection::GetSectionGripSize() const
{
	return ShotTrackConstants::SectionGripSize;
}

FName FShotSection::GetSectionGripLeftBrushName() const
{
	return ShotTrackConstants::ShotTrackGripBrushName;
}

FName FShotSection::GetSectionGripRightBrushName() const
{
	return ShotTrackConstants::ShotTrackGripBrushName;
}

FReply FShotSection::OnSectionDoubleClicked( const FGeometry& SectionGeometry, const FPointerEvent& MouseEvent )
{
	if( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance = Sequencer.Pin()->GetInstanceForSubMovieSceneSection( *Section );

		Sequencer.Pin()->FocusSubMovieScene( MovieSceneInstance );
	}

	return FReply::Handled();
}

int32 FShotSection::OnPaintSection( const FGeometry& AllottedGeometry, const FSlateRect& SectionClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, bool bParentEnabled ) const
{
	const ESlateDrawEffect::Type DrawEffects = bParentEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	if (Camera.IsValid())
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
		for( const TSharedPtr<FShotThumbnail>& Thumbnail : Thumbnails )
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

void FShotSection::Tick( const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (!Camera.IsValid())
	{
		// @todo Sequencer: This is called too often and is expensive
		Camera = UpdateCameraObject();
	}

	FTimeToPixel TimeToPixelConverter( AllottedGeometry, TRange<float>( Section->GetStartTime(), Section->GetEndTime() ) );

	// get the visible time range
	VisibleTimeRange = TRange<float>(
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X),
		TimeToPixelConverter.PixelToTime(-AllottedGeometry.Position.X + ParentGeometry.Size.X));

	FIntPoint AllocatedSize = AllottedGeometry.MakeChild( FVector2D( GetSectionGripSize(), 0.0f ), FVector2D( AllottedGeometry.Size.X - (GetSectionGripSize()*2), AllottedGeometry.Size.Y) ).Size.IntPoint();
	AllocatedSize.X = FMath::Max(AllocatedSize.X, 1);

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
	
	if( Camera.IsValid() )
	{
		ThumbnailPool.Pin()->RemoveThumbnailsNeedingRedraw(Thumbnails);
		Thumbnails.Empty();

		if(Size.X == 0 || Size.Y == 0)
		{
			return;
		}

		float AspectRatio = Camera->CameraComponent->AspectRatio;

		int32 NewThumbnailWidth = FMath::TruncToInt(ShotTrackConstants::TrackWidth*AspectRatio);

		int32 ThumbnailCount = FMath::DivideAndRoundUp(StoredSize.X, NewThumbnailWidth);

		if(NewThumbnailWidth != ThumbnailWidth)
		{
			ThumbnailWidth = NewThumbnailWidth;
			InternalViewportScene->UpdateViewportRHI(false, ThumbnailWidth, ShotTrackConstants::ThumbnailHeight, EWindowMode::Windowed);
		}

		float StartTime = Section->GetStartTime();
		float EndTime = Section->GetEndTime();
		float DeltaTime = EndTime - StartTime;

		int32 TotalX = StoredSize.X;

		// @todo sequencer optimize this to use a single viewport and a single thumbnail
		// instead paste the textures into the thumbnail at specific offsets
		for(int32 ThumbnailIndex = 0; ThumbnailIndex < ThumbnailCount; ++ThumbnailIndex)
		{
			int32 StartX = ThumbnailIndex * ThumbnailWidth;
			float FractionThrough = (float)StartX / (float)TotalX;
			float Time = StartTime + DeltaTime * FractionThrough;

			int32 NextStartX = (ThumbnailIndex+1) * ThumbnailWidth;
			float NextFractionThrough = (float)NextStartX / (float)TotalX;
			float NextTime = FMath::Min(StartTime + DeltaTime * NextFractionThrough, EndTime);

			FIntPoint ThumbnailSize(NextStartX-StartX, ShotTrackConstants::ThumbnailHeight);

			check(FractionThrough >= 0.f && FractionThrough <= 1.f && NextFractionThrough >= 0.f);
			TSharedPtr<FShotThumbnail> NewThumbnail = MakeShareable(new FShotThumbnail(SharedThis(this), ThumbnailSize, TRange<float>(Time, NextTime)));

			Thumbnails.Add(NewThumbnail);
		}

		// @todo Sequencer Optimize Only say a thumbnail needs redraw if it is onscreen
		ThumbnailPool.Pin()->AddThumbnailsNeedingRedraw(Thumbnails);
	}
}

void FShotSection::DrawViewportThumbnail(TSharedPtr<FShotThumbnail> ShotThumbnail)
{
	Sequencer.Pin()->SetGlobalTime(ShotThumbnail->GetTime());
	InternalViewportClient->UpdateViewForLockedActor();
	GWorld->SendAllEndOfFrameUpdates();
	InternalViewportScene->Draw(false);
	ShotThumbnail->CopyTextureIn((FSlateRenderTargetRHI*)InternalViewportScene->GetViewportRenderTargetTexture());
}

ACameraActor* FShotSection::UpdateCameraObject() const
{
	TArray<UObject*> OutObjects;
	// @todo Sequencer - Sub-MovieScenes The director track may be able to get cameras from sub-movie scenes
	Sequencer.Pin()->GetRuntimeObjects( Sequencer.Pin()->GetRootMovieSceneSequenceInstance(),  Section->GetCameraGuid(), OutObjects);

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
		Section->SetShotNameAndNumber( NewShotName, Section->GetShotNumber() );

		UMovieSceneSequence* ShotMovieSceneAnim = Section->GetMovieSceneAnimation();
		if( ShotMovieSceneAnim )
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");

			TArray<FAssetRenameData> AssetsAndNames;
			const FString PackagePath = FPackageName::GetLongPackagePath(ShotMovieSceneAnim->GetOutermost()->GetName());

			// We want to prepend the owning movie scene name.  This is not the movie scene inside referenced by the shot but the movie scene containing 
			// the shot track
			UMovieScene* OwningMovieScene = Section->GetTypedOuter<UMovieScene>();
			const FString NewAssetName = OwningMovieScene->GetName() + TEXT("_") + NewShotName.ToString();

			new (AssetsAndNames) FAssetRenameData(ShotMovieSceneAnim, PackagePath, NewAssetName );

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

TSharedRef<ISequencerTrackEditor> FShotTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> InSequencer )
{
	return MakeShareable( new FShotTrackEditor( InSequencer ) );
}

bool FShotTrackEditor::SupportsType( TSubclassOf<UMovieSceneTrack> Type ) const
{
	return Type == UMovieSceneShotTrack::StaticClass();
}

TSharedRef<ISequencerSection> FShotTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack& Track )
{
	check( SupportsType( SectionObject.GetOuter()->GetClass() ) );

	return MakeShareable( new FShotSection( GetSequencer(), ThumbnailPool, SectionObject ) );
}

void FShotTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	TArray<UObject*> OutObjects;
	// @todo Sequencer - Sub-MovieScenes The director track may be able to get cameras from sub-movie scenes
	GetSequencer()->GetRuntimeObjects(  GetSequencer()->GetRootMovieSceneSequenceInstance(), ObjectGuid, OutObjects);
	bool bValidKey = OutObjects.Num() == 1 && OutObjects[0]->IsA<ACameraActor>();
	if (bValidKey)
	{
		AnimatablePropertyChanged( UMovieSceneShotTrack::StaticClass(), FOnKeyProperty::CreateRaw( this, &FShotTrackEditor::AddKeyInternal, ObjectGuid ) );
	}
}

void FShotTrackEditor::Tick(float DeltaTime)
{
	if (FSlateThrottleManager::Get().IsAllowingExpensiveTasks())
	{
		ThumbnailPool->DrawThumbnails();
	}
}

void FShotTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();

	if ((RootMovieSceneSequence == nullptr) || (RootMovieSceneSequence->GetClass()->GetName() != TEXT("LevelSequenceInstance")))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddShotTrack", "Shot Track"),
		LOCTEXT("AddShotTooltip", "Adds a shot track, as well as a new shot at the current scrubber location if a camera is selected."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Tracks.Shot"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FShotTrackEditor::HandleAddShotTrackMenuEntryExecute),
			FCanExecuteAction::CreateRaw(this, &FShotTrackEditor::HandleAddShotTrackMenuEntryCanExecute)
		)
	);
}

void FShotTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding, const UClass* ObjectClass)
{
	if (ObjectClass->IsChildOf(ACameraActor::StaticClass()))
	{
		const TSharedPtr<ISequencer> ParentSequencer = GetSequencer();

		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddShot", "New Shot"),
			LOCTEXT("AddShotTooltip", "Adds a new shot using this camera at the scrubber location."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FShotTrackEditor::HandleAddShotMenuEntryExecute, ObjectBinding))
			);
	}
}

void FShotTrackEditor::AddKeyInternal( float KeyTime, const FGuid ObjectGuid )
{
	UMovieSceneSequence* FocusedSequence = GetSequencer()->GetFocusedMovieSceneSequence();
	UMovieScene* MovieScene = FocusedSequence->GetMovieScene();

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

	// Create the same type as the sequence type we have
	UClass* AssetClass = FocusedSequence->GetClass();

	UFactory* Factory = GetAssetFactoryForNewShot( AssetClass );

	UObject* NewAsset = AssetToolsModule.Get().CreateAsset( AssetName, PackagePath, AssetClass, Factory );
	

	ShotTrack->AddNewShot( ObjectGuid, *CastChecked<UMovieSceneSequence>( NewAsset ), KeyTime, FText::FromString( ShotName ), ShotNumber );
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
	// @todo Sequencer: This could be a binary search. All shots should be sorted 
	int32 SectionIndex = 0;
	for (SectionIndex = 0; SectionIndex < ShotSections.Num() && ShotSections[SectionIndex]->GetStartTime() < NewShotTime; ++SectionIndex);

	return SectionIndex;
}

UFactory* FShotTrackEditor::GetAssetFactoryForNewShot( UClass* SequenceClass )
{
	static TWeakObjectPtr<UFactory> ShotFactory;

	if( !ShotFactory.IsValid() || ShotFactory->SupportedClass != SequenceClass )
	{
		TArray<UFactory*> Factories;
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* Class = *It;
			if (Class->IsChildOf(UFactory::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
			{
				UFactory* Factory = Class->GetDefaultObject<UFactory>();
				if (Factory->CanCreateNew())
				{
					UClass* SupportedClass = Factory->GetSupportedClass();
					if ( SupportedClass == SequenceClass )
					{
						ShotFactory = Factory;
					}
				}
			}
		}
	}

	return ShotFactory.Get();
}


UMovieScene* FShotTrackEditor::GetFocusedMovieScene() const
{
	UMovieSceneSequence* FocusedSequence = GetSequencer()->GetFocusedMovieSceneSequence();

	return FocusedSequence->GetMovieScene();
}


bool FShotTrackEditor::HandleAddShotTrackMenuEntryCanExecute() const
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	return ((FocusedMovieScene != nullptr) && (FocusedMovieScene->GetShotTrack() == nullptr));
}


void FShotTrackEditor::HandleAddShotTrackMenuEntryExecute()
{
	UMovieScene* FocusedMovieScene = GetFocusedMovieScene();

	if (FocusedMovieScene == nullptr)
	{
		return;
	}

	UMovieSceneTrack* ShotTrack = FocusedMovieScene->GetShotTrack();
	
	if (ShotTrack == nullptr)
	{
		ShotTrack = FocusedMovieScene->AddShotTrack(UMovieSceneShotTrack::StaticClass());
	}
}


void FShotTrackEditor::HandleAddShotMenuEntryExecute(FGuid CameraGuid)
{
	AddKey(CameraGuid);
}


#undef LOCTEXT_NAMESPACE
