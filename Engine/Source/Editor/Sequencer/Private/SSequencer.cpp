// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"

#include "SSequencer.h"
#include "Editor/SequencerWidgets/Public/SequencerWidgetsModule.h"
#include "ISequencerInternals.h"
#include "MovieScene.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "ScopedTransaction.h"
#include "MovieSceneTrack.h"
#include "MovieSceneTrackEditor.h"
#include "SSequencerSectionOverlay.h"
#include "SSequencerTrackArea.h"
#include "SequencerNodeTree.h"
#include "TimeSliderController.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "DragAndDrop/ActorDragDropGraphEdOp.h"
#include "DragAndDrop/ClassDragDropOp.h"
#include "AssetSelection.h"
#include "K2Node_PlayMovieScene.h"
#include "MovieSceneShotSection.h"
#include "CommonMovieSceneTools.h"


#define LOCTEXT_NAMESPACE "Sequencer"


/**
 * The shot filter overlay displays the overlay needed to filter out widgets based on
 * which shots are actively in use.
 */
class SShotFilterOverlay : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SShotFilterOverlay) {}
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<ISequencerInternals> InSequencer)
	{
		ViewRange = InArgs._ViewRange;
		Sequencer = InSequencer;
	}

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
	{
		if (Sequencer.Pin()->IsShotFilteringOn())
		{
			TArray< TWeakObjectPtr<UMovieSceneSection> > FilterShots = Sequencer.Pin()->GetFilteringShotSections();
			// if there are filtering shots, continuously update the cached filter zones
			// we do this in tick, and cache it in order to make animation work properly
			CachedFilteredRanges.Empty();
			for (int32 i = 0; i < FilterShots.Num(); ++i)
			{
				UMovieSceneSection* Filter = FilterShots[i].Get();
				CachedFilteredRanges.Add(Filter->GetRange());
			}
		}
	}
	
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE
	{
		float Alpha = Sequencer.Pin()->GetOverlayFadeCurve();
		
		if (Alpha > 0.f)
		{
			FTimeToPixel TimeToPixelConverter = FTimeToPixel(AllottedGeometry, ViewRange.Get());
		
			TRange<float> TimeBounds = TRange<float>(TimeToPixelConverter.PixelToTime(0),
				TimeToPixelConverter.PixelToTime(AllottedGeometry.Size.X));

			TArray< TRange<float> > OverlayRanges = ComputeOverlayRanges(TimeBounds, CachedFilteredRanges);

			for (int32 i = 0; i < OverlayRanges.Num(); ++i)
			{
				float LowerBound = TimeToPixelConverter.TimeToPixel(OverlayRanges[i].GetLowerBoundValue());
				float UpperBound = TimeToPixelConverter.TimeToPixel(OverlayRanges[i].GetUpperBoundValue());
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(FVector2D(LowerBound, 0), FVector2D(UpperBound - LowerBound, AllottedGeometry.Size.Y)),
					FEditorStyle::GetBrush("Sequencer.ShotFilter"),
					MyClippingRect,
					ESlateDrawEffect::None,
					FLinearColor(1.f, 1.f, 1.f, Alpha)
				);
			}
		}

		return LayerId;
	}

	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		return FVector2D(100, 100);
	}

private:
	/**
	 * Given a range of time bounds, find what ranges still should be
	 * filtered based on shot filters
	 */
	TArray< TRange<float> > ComputeOverlayRanges(TRange<float> TimeBounds, TArray< TRange<float> > RangesToSubtract) const
	{
		TArray< TRange<float> > FilteredRanges;
		FilteredRanges.Add(TimeBounds);
		// @todo Sequencer Optimize - This is O(n^2)
		// However, n is likely to stay very low, and the average case is likely O(n)
		// Also, the swapping of TArrays in this loop could use some heavy optimization as well
		for (int32 i = 0; i < RangesToSubtract.Num(); ++i)
		{
			TRange<float>& CurrentRange = RangesToSubtract[i];

			// ignore ranges that don't overlap with the time bounds
			if (CurrentRange.Overlaps(TimeBounds))
			{
				TArray< TRange<float> > NewFilteredRanges;
				for (int32 j = 0; j < FilteredRanges.Num(); ++j)
				{
					TArray< TRange<float> > SubtractedRanges = TRange<float>::Difference(FilteredRanges[j], CurrentRange);
					NewFilteredRanges.Append(SubtractedRanges);
				}
				FilteredRanges = NewFilteredRanges;
			}
		}
		return FilteredRanges;
	}

private:
	/** The current minimum view range */
	TAttribute< TRange<float> > ViewRange;
	/** The main sequencer interface */
	TWeakPtr<ISequencerInternals> Sequencer;
	/** Cached set of ranges that are being filtering currently */
	TArray< TRange<float> > CachedFilteredRanges;
};


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSequencer::Construct( const FArguments& InArgs, TSharedRef< class ISequencerInternals > InSequencer )
{
	Sequencer = InSequencer;

	// Create a node tree which contains a tree of movie scene data to display in the sequence
	SequencerNodeTree = MakeShareable( new FSequencerNodeTree( InSequencer.Get() ) );

	AllowAutoKey = InArgs._AllowAutoKey;
	OnToggleAutoKey = InArgs._OnToggleAutoKey;
	CleanViewEnabled = InArgs._CleanViewEnabled;
	OnToggleCleanView = InArgs._OnToggleCleanView;

	FSequencerWidgetsModule& SequencerWidgets = FModuleManager::Get().LoadModuleChecked<FSequencerWidgetsModule>( "SequencerWidgets" );

	FTimeSliderArgs TimeSliderArgs;
	TimeSliderArgs.ViewRange = InArgs._ViewRange;
	TimeSliderArgs.OnViewRangeChanged = InArgs._OnViewRangeChanged;
	TimeSliderArgs.ScrubPosition = InArgs._ScrubPosition;
	TimeSliderArgs.OnScrubPositionChanged = InArgs._OnScrubPositionChanged;

	TSharedRef<FSequencerTimeSliderController> TimeSliderController( new FSequencerTimeSliderController( TimeSliderArgs ) );
	
	bool bMirrorLabels = false;
	// Create the top and bottom sliders
	TSharedRef<ITimeSlider> TopTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, bMirrorLabels );

	bMirrorLabels = true;
	TSharedRef<ITimeSlider> BottomTimeSlider = SequencerWidgets.CreateTimeSlider( TimeSliderController, bMirrorLabels);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.Padding( FMargin( 0.0f, 2.0f, 0.0f, 0.0f ) )
				.FillWidth( TAttribute<float>( this, &SSequencer::GetAnimationOutlinerFillPercentage ) )
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						// @todo Sequencer - Temp auto-key button
						SNew( SCheckBox )
						.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
						.IsChecked( this, &SSequencer::OnGetAutoKeyCheckState )
						.OnCheckStateChanged( this, &SSequencer::OnAutoKeyChecked )
						[
							SNew( STextBlock )
							.Text( LOCTEXT("ToggleAutoKey", "Auto Key").ToString() )
						]
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.FillWidth(1.f)
					[
						// @todo Sequencer - Temp clean view button
						SNew( SCheckBox )
						.Style(FEditorStyle::Get(), "ToggleButtonCheckbox")
						.IsChecked( this, &SSequencer::OnGetCleanViewCheckState )
						.OnCheckStateChanged( this, &SSequencer::OnCleanViewChecked )
						[
							SNew( STextBlock )
							.Text( LOCTEXT("ToggleCleanView", "Clean View").ToString() )
						]
					]
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(BreadcrumbTrail, SBreadcrumbTrail<FSequencerBreadcrumb>)
					.OnCrumbClicked(this, &SSequencer::OnCrumbClicked)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.Padding( FMargin( 0.0f, 2.0f, 0.0f, 0.0f ) )
				.FillWidth( TAttribute<float>( this, &SSequencer::GetAnimationOutlinerFillPercentage ) )
				.VAlign(VAlign_Center)
				[
					// Search box for searching through the outliner
					SNew( SSearchBox )
					.OnTextChanged( this, &SSequencer::OnOutlinerSearchChanged )
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew( SBorder )
					// @todo Sequencer Do not change the paddings or the sliders scrub widgets wont line up
					.Padding( FMargin( 0.0f, 2.0f, 0.0f, 0.0f ) )
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					.BorderBackgroundColor( FLinearColor(.50f, .50f, .50f, 1.0f ) )
					[
						TopTimeSlider
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew( SOverlay )
				+ SOverlay::Slot()
				[
					MakeSectionOverlay( TimeSliderController, InArgs._ViewRange, InArgs._ScrubPosition, false )
				]
				+ SOverlay::Slot()
				[
					SAssignNew( TrackArea, SSequencerTrackArea, Sequencer.Pin().ToSharedRef() )
					.ViewRange( InArgs._ViewRange )
					.OutlinerFillPercent( this, &SSequencer::GetAnimationOutlinerFillPercentage )
				]
				+ SOverlay::Slot()
				[
					SNew( SHorizontalBox )
					.Visibility( EVisibility::HitTestInvisible )
					+ SHorizontalBox::Slot()
					.FillWidth( TAttribute<float>( this, &SSequencer::GetAnimationOutlinerFillPercentage ) )
					[
						// Take up space but display nothing. This is required so that all areas dependent on time align correctly
						SNullWidget::NullWidget
					]
					+ SHorizontalBox::Slot()
					.FillWidth( 1.0f )
					[
						SNew(SShotFilterOverlay, Sequencer)
						.ViewRange( InArgs._ViewRange )
					]
				]
				+ SOverlay::Slot()
				[
					MakeSectionOverlay( TimeSliderController, InArgs._ViewRange, InArgs._ScrubPosition, true )
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.FillWidth( TAttribute<float>( this, &SSequencer::GetAnimationOutlinerFillPercentage ) )
				[
					SNew( SSpacer )
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew( SBorder )
					// @todo Sequencer Do not change the paddings or the sliders scrub widgets wont line up
					.Padding( FMargin( 0.0f, 0.0f, 0.0f, 2.0f ) )
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					.BorderBackgroundColor( FLinearColor(.50f, .50f, .50f, 1.0f ) )
					[
						BottomTimeSlider
					]
				]
			]
		]
	];

	BreadcrumbTrail->PushCrumb(TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateSP(this, &SSequencer::GetRootMovieSceneName)), FSequencerBreadcrumb( Sequencer.Pin()->GetRootMovieSceneInstance() ));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSequencer::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	Sequencer.Pin()->Tick(InDeltaTime);

	const TArray< TSharedPtr<FMovieSceneTrackEditor> >& TrackEditors = Sequencer.Pin()->GetTrackEditors();
	for (int32 EditorIndex = 0; EditorIndex < TrackEditors.Num(); ++EditorIndex)
	{
		TrackEditors[EditorIndex]->Tick(InCurrentTime);
	}
}

void SSequencer::UpdateLayoutTree()
{	
	// Update the node tree
	SequencerNodeTree->Update();
	
	TrackArea->Update( *SequencerNodeTree );

	SequencerNodeTree->UpdateCachedVisibilityBasedOnShotFiltersChanged();
}

void SSequencer::UpdateBreadcrumbs(const TArray< TWeakObjectPtr<class UMovieSceneSection> >& FilteringShots)
{
	TSharedRef<FMovieSceneInstance> FocusedMovieSceneInstance = Sequencer.Pin()->GetFocusedMovieSceneInstance();

	if (BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::ShotType)
	{
		BreadcrumbTrail->PopCrumb();
	}

	if( BreadcrumbTrail->PeekCrumb().BreadcrumbType == FSequencerBreadcrumb::MovieSceneType && BreadcrumbTrail->PeekCrumb().MovieSceneInstance.Pin() != FocusedMovieSceneInstance )
	{
		// The current breadcrumb is not a moviescene so we need to make a new breadcrumb in order return to the parent moviescene later
		BreadcrumbTrail->PushCrumb( FocusedMovieSceneInstance->GetMovieScene()->GetName(), FSequencerBreadcrumb( FocusedMovieSceneInstance ) );
	}

	if (Sequencer.Pin()->IsShotFilteringOn())
	{
		TAttribute<FString> CrumbString;
		if (FilteringShots.Num() == 1)
		{
			CrumbString = TAttribute<FString>::Create(TAttribute<FString>::FGetter::CreateSP(this, &SSequencer::GetShotSectionTitle, FilteringShots[0].Get()));
		}
		else {CrumbString = TEXT("Multiple Shots");}
		BreadcrumbTrail->PushCrumb(CrumbString, FSequencerBreadcrumb());
	}

	SequencerNodeTree->UpdateCachedVisibilityBasedOnShotFiltersChanged();
}

void SSequencer::OnOutlinerSearchChanged( const FText& Filter )
{
	SequencerNodeTree->FilterNodes( Filter.ToString() );
}

TSharedRef<SWidget> SSequencer::MakeSectionOverlay( TSharedRef<FSequencerTimeSliderController> TimeSliderController, const TAttribute< TRange<float> >& ViewRange, const TAttribute<float>& ScrubPosition, bool bTopOverlay )
{
	return
		SNew( SHorizontalBox )
		.Visibility( EVisibility::HitTestInvisible )
		+ SHorizontalBox::Slot()
		.FillWidth( TAttribute<float>( this, &SSequencer::GetAnimationOutlinerFillPercentage ) )
		[
			// Take up space but display nothing. This is required so that all areas dependent on time align correctly
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot()
		.FillWidth( 1.0f )
		[
			SNew( SSequencerSectionOverlay, TimeSliderController )
			.DisplayScrubPosition( bTopOverlay )
			.DisplayTickLines( !bTopOverlay )
		];
}

ESlateCheckBoxState::Type SSequencer::OnGetAutoKeyCheckState() const
{
	return AllowAutoKey.Get() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SSequencer::OnAutoKeyChecked( ESlateCheckBoxState::Type InState ) 
{
	OnToggleAutoKey.ExecuteIfBound( InState == ESlateCheckBoxState::Checked ? true : false );
}

ESlateCheckBoxState::Type SSequencer::OnGetCleanViewCheckState() const
{
	return CleanViewEnabled.Get() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SSequencer::OnCleanViewChecked( ESlateCheckBoxState::Type InState ) 
{
	OnToggleCleanView.ExecuteIfBound( InState == ESlateCheckBoxState::Checked ? true : false );
}


void SSequencer::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// @todo sequencer: Add drop validity cue
}


void SSequencer::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	// @todo sequencer: Clear drop validity cue
}


FReply SSequencer::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bool bIsDragSupported = false;

	if( DragDrop::IsTypeMatch<FAssetDragDropOp>( DragDropEvent.GetOperation() ) ||
		DragDrop::IsTypeMatch<FClassDragDropOp>( DragDropEvent.GetOperation() ) ||
		DragDrop::IsTypeMatch<FUnloadedClassDragDropOp>( DragDropEvent.GetOperation() ) ||
		DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
	{
		bIsDragSupported = true;
	}

	return bIsDragSupported ? FReply::Handled() : FReply::Unhandled();
}


FReply SSequencer::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bool bWasDropHandled = false;

	// @todo sequencer: Get rid of hard-code assumptions about dealing with ACTORS at this level?

	// @todo sequencer: We may not want any actor-specific code here actually.  We need systems to be able to
	// register with sequencer to support dropping assets/classes/actors, or OTHER types!

	// @todo sequencer: Handle drag and drop from other FDragDropOperations, including unloaded classes/asset and external drags!

	// @todo sequencer: Consider allowing drops into the level viewport to add to the MovieScene as well.
	//		- Basically, when Sequencer is open it would take over drops into the level and auto-add puppets for these instead of regular actors
	//		- This would let people drag smoothly and precisely into the view to drop assets/classes into the scene

	if( DragDrop::IsTypeMatch<FAssetDragDropOp>( DragDropEvent.GetOperation() ) )
	{
		const auto& DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>( DragDropEvent.GetOperation() );

		OnAssetsDropped( *DragDropOp );

		bWasDropHandled = true;
	}
	else if( DragDrop::IsTypeMatch<FClassDragDropOp>( DragDropEvent.GetOperation() ) )
	{
		const auto& DragDropOp = StaticCastSharedPtr<FClassDragDropOp>( DragDropEvent.GetOperation() );

		OnClassesDropped( *DragDropOp );

		bWasDropHandled = true;
	}
	else if( DragDrop::IsTypeMatch<FUnloadedClassDragDropOp>( DragDropEvent.GetOperation() ) )
	{
		const auto& DragDropOp = StaticCastSharedPtr<FUnloadedClassDragDropOp>( DragDropEvent.GetOperation() );

		OnUnloadedClassesDropped( *DragDropOp );

		bWasDropHandled = true;
	}
	else if( DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() ) )
	{
		const auto& DragDropOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() );

		OnActorsDropped( *DragDropOp );

		bWasDropHandled = true;
	}

	return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
}

FReply SSequencer::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) 
{
	// A toolkit tab is active, so direct all command processing to it
	if( Sequencer.Pin()->GetCommandBindings().ProcessCommandBindings( InKeyboardEvent ) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SSequencer::OnAssetsDropped( const FAssetDragDropOp& DragDropOp )
{
	ISequencerInternals& SequencerRef = *Sequencer.Pin();

	bool bObjectAdded = false;
	bool bSpawnableAdded = false;
	TArray< UObject* > DroppedObjects;
	bool bAllAssetsWereLoaded = true;
	for( auto CurAssetData = DragDropOp.AssetData.CreateConstIterator(); CurAssetData; ++CurAssetData )
	{
		const FAssetData& AssetData = *CurAssetData;

		UObject* Object = AssetData.GetAsset();
		if ( Object != NULL )
		{
			DroppedObjects.Add( Object );
		}
		else
		{
			bAllAssetsWereLoaded = false;
		}
	}

	const TSet< TSharedRef<const FSequencerDisplayNode> >& SelectedNodes = SequencerNodeTree->GetSelectedNodes();
	FGuid TargetObjectGuid;
	// if exactly one object node is selected, we have a target object guid
	TSharedPtr<const FSequencerDisplayNode> DisplayNode;
	if (SelectedNodes.Num() == 1)
	{
		for (TSet< TSharedRef<const FSequencerDisplayNode> >::TConstIterator It(SelectedNodes); It; ++It)
		{
			DisplayNode = *It;
		}
		if (DisplayNode.IsValid() && DisplayNode->GetType() == ESequencerNode::Object)
		{
			TSharedPtr<const FObjectBindingNode> ObjectBindingNode = StaticCastSharedPtr<const FObjectBindingNode>(DisplayNode);
			TargetObjectGuid = ObjectBindingNode->GetObjectBinding();
		}
	}

	for( auto CurObjectIter = DroppedObjects.CreateConstIterator(); CurObjectIter; ++CurObjectIter )
	{
		UObject* CurObject = *CurObjectIter;

		if (!SequencerRef.OnHandleAssetDropped(CurObject, TargetObjectGuid))
		{
			SequencerRef.AddSpawnableForAssetOrClass( CurObject, NULL );
			bSpawnableAdded = true;
		}
		bObjectAdded = true;
	}


	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );
	}

	if( bObjectAdded )
	{
		// Update the sequencers view of the movie scene data when any object is added
		SequencerRef.NotifyMovieSceneDataChanged();
	}
}


void SSequencer::OnClassesDropped( const FClassDragDropOp& DragDropOp )
{
	ISequencerInternals& SequencerRef = *Sequencer.Pin();

	bool bSpawnableAdded = false;
	for( auto ClassIter = DragDropOp.ClassesToDrop.CreateConstIterator(); ClassIter; ++ClassIter )
	{
		UClass* Class = ( *ClassIter ).Get();
		if( Class != NULL )
		{
			UObject* Object = Class->GetDefaultObject();

			SequencerRef.AddSpawnableForAssetOrClass( Object, NULL );

			bSpawnableAdded = true;

		}
	}

	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );

		// Update the sequencers view of the movie scene data
		SequencerRef.NotifyMovieSceneDataChanged();
	}
}

void SSequencer::OnUnloadedClassesDropped( const FUnloadedClassDragDropOp& DragDropOp )
{
	ISequencerInternals& SequencerRef = *Sequencer.Pin();
	bool bSpawnableAdded = false;
	for( auto ClassDataIter = DragDropOp.AssetsToDrop->CreateConstIterator(); ClassDataIter; ++ClassDataIter )
	{
		auto& ClassData = *ClassDataIter;

		// Check to see if the asset can be found, otherwise load it.
		UObject* Object = FindObject<UObject>( NULL, *ClassData.AssetName );
		if( Object == NULL )
		{
			Object = FindObject<UObject>(NULL, *FString::Printf(TEXT("%s.%s"), *ClassData.GeneratedPackageName, *ClassData.AssetName));
		}

		if( Object == NULL )
		{
			// Load the package.
			GWarn->BeginSlowTask( LOCTEXT("OnDrop_FullyLoadPackage", "Fully Loading Package For Drop"), true, false );
			UPackage* Package = LoadPackage(NULL, *ClassData.GeneratedPackageName, LOAD_NoRedirects );
			if( Package != NULL )
			{
				Package->FullyLoad();
			}
			GWarn->EndSlowTask();

			Object = FindObject<UObject>(Package, *ClassData.AssetName);
		}

		if( Object != NULL )
		{
			// Check to see if the dropped asset was a blueprint
			if(Object->IsA(UBlueprint::StaticClass()))
			{
				// Get the default object from the generated class.
				Object = Cast<UBlueprint>(Object)->GeneratedClass->GetDefaultObject();
			}
		}

		if( Object != NULL )
		{
			SequencerRef.AddSpawnableForAssetOrClass( Object, NULL );
			bSpawnableAdded = true;
		}
	}

	if( bSpawnableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );

		SequencerRef.NotifyMovieSceneDataChanged();
	}
}


void SSequencer::OnActorsDropped( FActorDragDropGraphEdOp& DragDropOp )
{
	ISequencerInternals& SequencerRef = *Sequencer.Pin();
	bool bPossessableAdded = false;
	for( auto ActorIter = DragDropOp.Actors.CreateIterator(); ActorIter; ++ActorIter )
	{
		AActor* Actor = ( *ActorIter ).Get();
		if( Actor != NULL )
		{
			// Grab the MovieScene that is currently focused.  We'll add our Blueprint as an inner of the
			// MovieScene asset.
			UMovieScene* OwnerMovieScene = SequencerRef.GetFocusedMovieScene();

			// @todo sequencer: Undo doesn't seem to be working at all
			const FScopedTransaction Transaction( LOCTEXT("UndoPossessingObject", "Possess Object with MovieScene") );

			// Possess the object!
			{
				// Update level script
				const bool bCreateIfNotFound = true;
				UK2Node_PlayMovieScene* PlayMovieSceneNode = SequencerRef.BindToPlayMovieSceneNode( bCreateIfNotFound );

				// Create a new possessable
				OwnerMovieScene->Modify();
				const FString PossessableName = Actor->GetActorLabel();
				const FGuid PossessableGuid = OwnerMovieScene->AddPossessable( PossessableName, Actor->GetClass() );

				if (Sequencer.Pin()->IsShotFilteringOn()) {Sequencer.Pin()->AddUnfilterableObject(PossessableGuid);}

				// Bind the actor to the new possessable
				// @todo sequencer: Handle Undo properly here (new UObjects being created and modified)
				TArray< UObject* > Actors;
				Actors.Add( Actor );
				PlayMovieSceneNode->BindPossessableToObjects( PossessableGuid, Actors );

				bPossessableAdded = true;
			}
		}
	}

	if( bPossessableAdded )
	{
		SequencerRef.SpawnOrDestroyPuppetObjects( SequencerRef.GetFocusedMovieSceneInstance() );

		SequencerRef.NotifyMovieSceneDataChanged();
	}
}

void SSequencer::OnCrumbClicked(const FSequencerBreadcrumb& Item)
{
	if (Item.BreadcrumbType != FSequencerBreadcrumb::ShotType)
	{
		if( Sequencer.Pin()->GetFocusedMovieSceneInstance() == Item.MovieSceneInstance.Pin() ) 
		{
			// then do zooming
		}
		else
		{
			Sequencer.Pin()->PopToMovieScene( Item.MovieSceneInstance.Pin().ToSharedRef() );
		}

		Sequencer.Pin()->FilterToShotSections( TArray< TWeakObjectPtr<UMovieSceneSection> >() );
	}
	else
	{
		// refilter what we already have
		Sequencer.Pin()->FilterToShotSections(Sequencer.Pin()->GetFilteringShotSections());
	}
}

FString SSequencer::GetRootMovieSceneName() const
{
	return Sequencer.Pin()->GetRootMovieScene()->GetName();
}

FString SSequencer::GetShotSectionTitle(UMovieSceneSection* ShotSection) const
{
	return Cast<UMovieSceneShotSection>(ShotSection)->GetTitle().ToString();
}

void SSequencer::DeleteSelectedNodes()
{
	const TSet< TSharedRef<const FSequencerDisplayNode> >& SelectedNodes = SequencerNodeTree->GetSelectedNodes();

	if( SelectedNodes.Num() > 0 )
	{
		const FScopedTransaction Transaction( LOCTEXT("UndoDeletingObject", "Delete Object from MovieScene") );

		ISequencerInternals& SequencerRef = *Sequencer.Pin();

		for( auto It = SelectedNodes.CreateConstIterator(); It; ++It )
		{
			TSharedRef<const FSequencerDisplayNode> Node = *It;

			if( Node->IsVisible() )
			{
				// Delete everything in the entire node
				SequencerRef.OnRequestNodeDeleted( Node );
			}

		}
	}
}

#undef LOCTEXT_NAMESPACE
