// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"
#include "SSceneOutliner.h"

#include "ScopedTransaction.h"

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "ClassIconFinder.h"

#include "SSocketChooser.h"

#include "LevelUtils.h"
#include "ActorEditorUtils.h"
#include "SceneOutlinerFilters.h"

DEFINE_LOG_CATEGORY_STATIC(LogSceneOutliner, Log, All);

#define LOCTEXT_NAMESPACE "SSceneOutliner"

// The amoung of time that must past before the Scene Outliner will attempt a sort when in PIE/SIE.
#define SCENE_OUTLINER_RESORT_TIMER 1.0f

namespace SceneOutliner
{
	namespace Helpers
	{
		/** Sorts tree items alphabetically */
		struct FSortByActorAscending
		{
			bool operator()( const FOutlinerTreeItemPtr& A, const FOutlinerTreeItemPtr& B ) const
			{
				auto ActorA = A.Get()->Actor;
				auto ActorB = B.Get()->Actor;
				if( ActorA != NULL && ActorB != NULL )
				{
					return ActorA->GetActorLabel() < ActorB->GetActorLabel();
				}

				return false;
			}
		};
		struct FSortByActorDescending
		{
			bool operator()( const FOutlinerTreeItemPtr& A, const FOutlinerTreeItemPtr& B ) const
			{
				auto ActorA = A.Get()->Actor;
				auto ActorB = B.Get()->Actor;
				if( ActorA != NULL && ActorB != NULL )
				{
					return ActorA->GetActorLabel() > ActorB->GetActorLabel();
				}

				return false;
			}
		};
	}

	void SSceneOutliner::Construct( const FArguments& InArgs, const FSceneOutlinerInitializationOptions& InitOptions )
	{
		this->InitOptions = InitOptions;

		OnActorPicked = InArgs._OnActorPickedDelegate;

		if( InitOptions.OnSelectionChanged.IsBound() )
		{
			SelectionChanged.Add( InitOptions.OnSelectionChanged );
		}

		bFullRefresh = true;
		bNeedsRefresh = true;
		bIsReentrant = false;
		TotalActorCount = 0;
		FilteredActorCount = 0;
		SortOutlinerTimer = 0.0f;
		bPendingFocusNextFrame = InitOptions.bFocusSearchBoxWhenOpened;

		// @todo outliner: Should probably save this in layout!
		// @todo outliner: Should save spacing for list view in layout

		NoBorder = FEditorStyle::GetBrush( "LevelViewport.NoViewportBorder" );
		PlayInEditorBorder = FEditorStyle::GetBrush( "LevelViewport.StartingPlayInEditorBorder" );
		SimulateBorder = FEditorStyle::GetBrush( "LevelViewport.StartingSimulateBorder" );
		MobilityStaticBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityStaticPip" );
		MobilityStationaryBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityStationaryPip" );
		MobilityMovableBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityMovablePip" );

		//Setup the SearchBox filter
		SearchBoxActorFilter = MakeShareable( new ActorTextFilter( ActorTextFilter::FItemToStringArray::CreateSP( this, &SSceneOutliner::PopulateActorSearchStrings ) ) );

		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

		//Setup the FilterCollection
		//We use the filter collection provide, otherwise we create our own
		if( InitOptions.ActorFilters.IsValid() )
		{
			CustomFilters = InitOptions.ActorFilters;
		}
		else
		{
			CustomFilters = MakeShareable( new ActorFilterCollection() );
		}
	
		SearchBoxActorFilter->OnChanged().AddSP( this, &SSceneOutliner::FullRefresh );
		CustomFilters->OnChanged().AddSP( this, &SSceneOutliner::FullRefresh );

		//Apply custom filters based on global preferences
		if ( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			ApplyShowOnlySelectedFilter( IsShowingOnlySelected() );
			ApplyHideTemporaryActorsFilter( IsHidingTemporaryActors() );
		}


		if( InitOptions.CustomColumnFactory.IsBound() )
		{
			CustomColumn = InitOptions.CustomColumnFactory.Execute( SharedThis( this ) );
		}
		else
		{
			CustomColumn = MakeShareable( new FSceneOutlinerActorInfoColumn( SharedThis( this ), ECustomColumnMode::None ) );
		}

		TSharedRef< SHeaderRow > HeaderRowWidget =
			SNew( SHeaderRow )
				// Only show the list header if the user configured the outliner for that
				.Visibility( InitOptions.bShowHeaderRow ? EVisibility::Visible : EVisibility::Collapsed );

		if (InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			// Set up the gutter if we're in actor browsing mode
			Gutter = MakeShareable(new FSceneOutlinerGutter());
			HeaderRowWidget->AddColumn(
				Gutter->ConstructHeaderRowColumn()
				.FixedWidth(16.f)
				.SortMode(this, &SSceneOutliner::GetColumnSortMode, Gutter->GetColumnID())
				.OnSort(this, &SSceneOutliner::OnColumnSortModeChanged)
			);
		}
		
		// Actor label column
		HeaderRowWidget->AddColumn(	
			SHeaderRow::Column( ColumnID_ActorLabel )
			.DefaultLabel( LOCTEXT("ActorColumnLabel", "Actor").ToString() )
			.SortMode( this, &SSceneOutliner::GetColumnSortMode, ColumnID_ActorLabel )
			.OnSort( this, &SSceneOutliner::OnColumnSortModeChanged )
			.FillWidth( 5.0f )
		);

		if ( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing || InitOptions.CustomColumnFactory.IsBound() )
		{
			// Customizable actor data column is only viable when browsing OR when it has been bound specifically

			auto CustomColumnHeaderRow = CustomColumn->ConstructHeaderRowColumn();

			CustomColumnHeaderRow
				.SortMode(this, &SSceneOutliner::GetColumnSortMode, CustomColumn->GetColumnID())
				.OnSort(this, &SSceneOutliner::OnColumnSortModeChanged);

			if (InitOptions.CustomColumnFixedWidth > 0.0f)
			{
				CustomColumnHeaderRow.FixedWidth(InitOptions.CustomColumnFixedWidth);
			}
			else
			{
				CustomColumnHeaderRow.FillWidth(2.0f);
			}

			HeaderRowWidget->AddColumn(CustomColumnHeaderRow);
		}

		ChildSlot
		[
			SNew( SBorder )
			.BorderImage( this, &SSceneOutliner::OnGetBorderBrush )
			.BorderBackgroundColor( this, &SSceneOutliner::OnGetBorderColorAndOpacity )
			.ShowEffectWhenDisabled( false )
			[
				VerticalBox
			]
		];

		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding( 0.0f, 4.0f )
		[
			SAssignNew( FilterTextBoxWidget, SSearchBox )
			.Visibility( this, &SSceneOutliner::GetSearchBoxVisibility )
			.HintText( LOCTEXT( "FilterSearchActors", "Search Actors" ) )
			.ToolTipText( LOCTEXT("FilterSearchActorsHint", "Type here to search actors.  (Pressing enter selects actors.)").ToString() )
			.OnTextChanged( this, &SSceneOutliner::OnFilterTextChanged )
			.OnTextCommitted( this, &SSceneOutliner::OnFilterTextCommitted )
		];

		VerticalBox->AddSlot()
		.FillHeight( 1.0f )
		[
			SNew( SOverlay )
			+SOverlay::Slot()
			.HAlign( HAlign_Center )
			[
				SNew( STextBlock )
				.Visibility( this, &SSceneOutliner::GetEmptyLabelVisibility )
				.Text( LOCTEXT( "EmptyLabel", "Empty" ).ToString() )
				.ColorAndOpacity( FLinearColor( 0.4f, 1.0f, 0.4f ) )
			]

			+SOverlay::Slot()
			[
				SAssignNew( OutlinerTreeView, SOutlinerTreeView )

				// multi-select if we're in browsing mode, 
				// single-select if we're in picking mode,
				.SelectionMode( this, &SSceneOutliner::GetSelectionMode )

				// Point the tree to our array of root-level items.  Whenever this changes, we'll call RequestTreeRefresh()
				.TreeItemsSource( &RootTreeItems )

				// Find out when the user selects something in the tree
				.OnSelectionChanged( this, &SSceneOutliner::OnOutlinerTreeSelectionChanged )

				// Called when the user double-clicks with LMB on an item in the list
				.OnMouseButtonDoubleClick( this, &SSceneOutliner::OnOutlinerTreeDoubleClick )

				// Called when an item is scrolled into view
				.OnItemScrolledIntoView( this, &SSceneOutliner::OnOutlinerTreeItemScrolledIntoView )

				// Called to child items for any given parent item
				.OnGetChildren( this, &SSceneOutliner::OnGetChildrenForOutlinerTree )

				// Generates the actual widget for a tree item
				.OnGenerateRow( this, &SSceneOutliner::OnGenerateRowForOutlinerTree ) 

				// Use the level viewport context menu as the right click menu for tree items
				.OnContextMenuOpening( InArgs._MakeContextMenuWidgetDelegate )

				// Header for the tree
				.HeaderRow( HeaderRowWidget )
			]
		];

		if ( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Separator
			VerticalBox->AddSlot()
			.AutoHeight()
			.Padding(0, 0, 0, 1)
			[
				SNew(SSeparator)
			];

			// Bottom panel
			VerticalBox->AddSlot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				// Asset count
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				.Padding(8, 0)
				[
					SNew( STextBlock )
					.Text( this, &SSceneOutliner::GetFilterStatusText )
					.ColorAndOpacity( this, &SSceneOutliner::GetFilterStatusTextColor )
				]

				// View mode combo button
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew( ViewOptionsComboButton, SComboButton )
					.ContentPadding(0)
					.ForegroundColor( this, &SSceneOutliner::GetViewButtonForegroundColor )
					.ButtonStyle( FEditorStyle::Get(), "ToggleButton" ) // Use the tool bar item style for this button
					.OnGetMenuContent( this, &SSceneOutliner::GetViewButtonContent )
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage).Image( FEditorStyle::GetBrush("GenericViewButton") )
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 0, 0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text( LOCTEXT("ViewButton", "View Options").ToString() )
						]
					]
				]
			];
		} //end if (InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)


		// Don't allow tool-tips over the header
		HeaderRowWidget->EnableToolTipForceField( true );


		// Populate our data set
		Populate();


		// We only synchronize selection when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Populate and register to find out when the level's selection changes
			OnLevelSelectionChanged( NULL );
			USelection::SelectionChangedEvent.AddRaw(this, &SSceneOutliner::OnLevelSelectionChanged);
			USelection::SelectObjectEvent.AddRaw(this, &SSceneOutliner::OnLevelSelectionChanged);
		}

		// Register to find out when actors are added or removed
		// @todo outliner: Might not catch some cases (see: CALLBACK_ActorPropertiesChange, CALLBACK_LayerChange, CALLBACK_LevelDirtied, CALLBACK_OnActorMoved, CALLBACK_UpdateLevelsForAllActors)
		GEngine->OnLevelActorsChanged().AddSP( this, &SSceneOutliner::FullRefresh );
		FWorldDelegates::LevelAddedToWorld.AddSP( this, &SSceneOutliner::OnLevelAdded );
		FWorldDelegates::LevelRemovedFromWorld.AddSP( this, &SSceneOutliner::OnLevelRemoved );

		GEngine->OnLevelActorAdded().AddSP( this, &SSceneOutliner::OnLevelActorsAdded );
		GEngine->OnLevelActorDetached().AddSP( this, &SSceneOutliner::OnLevelActorsDetached );

		GEngine->OnLevelActorDeleted().AddSP( this, &SSceneOutliner::OnLevelActorsRemoved );
		GEngine->OnLevelActorAttached().AddSP( this, &SSceneOutliner::OnLevelActorsAttached );

		GEngine->OnLevelActorRequestRename().AddSP( this, &SSceneOutliner::OnLevelActorsRequestRename );

		// Register to update when an undo/redo operation has been called to update our list of actors
		GEditor->RegisterForUndo( this );

		// Register to be notified when properties are edited
		FCoreDelegates::OnActorLabelChanged.Add( FCoreDelegates::FOnActorLabelChanged::FDelegate::CreateRaw(this, &SSceneOutliner::OnActorLabelChanged) );
	}

	SSceneOutliner::~SSceneOutliner()
	{
		// We only synchronize selection when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			USelection::SelectionChangedEvent.RemoveAll(this);
			USelection::SelectObjectEvent.RemoveAll(this);
		}
		GEngine->OnLevelActorsChanged().RemoveAll( this );
		GEditor->UnregisterForUndo( this );

		SearchBoxActorFilter->OnChanged().RemoveAll( this );
		CustomFilters->OnChanged().RemoveAll( this );
		
		FWorldDelegates::LevelAddedToWorld.RemoveAll( this );
		FWorldDelegates::LevelRemovedFromWorld.RemoveAll( this );

		FCoreDelegates::OnActorLabelChanged.Remove( FCoreDelegates::FOnActorLabelChanged::FDelegate::CreateRaw(this, &SSceneOutliner::OnActorLabelChanged) );
	}

	FSlateColor SSceneOutliner::GetViewButtonForegroundColor() const
	{
		return ViewOptionsComboButton->IsHovered() ? FEditorStyle::GetSlateColor("InvertedForeground") : FEditorStyle::GetSlateColor("DefaultForeground");
	}

	TSharedRef<SWidget> SSceneOutliner::GetViewButtonContent()
	{
		FMenuBuilder MenuBuilder(true, NULL);

		MenuBuilder.BeginSection("AssetThumbnails", LOCTEXT("ShowHeading", "Show"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("ToggleShowOnlySelected", "Only Selected"),
				LOCTEXT("ToggleShowOnlySelectedToolTip", "When enabled, only displays actors that are currently selected."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP( this, &SSceneOutliner::ToggleShowOnlySelected ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SSceneOutliner::IsShowingOnlySelected )
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("ToggleHideTemporaryActors", "Hide Temporary Actors"),
				LOCTEXT("ToggleHideTemporaryActorsToolTip", "When enabled, hides temporary/run-time Actors."),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP( this, &SSceneOutliner::ToggleHideTemporaryActors ),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP( this, &SSceneOutliner::IsHidingTemporaryActors )
				),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}


	/** FILTERS */
	void SSceneOutliner::ToggleShowOnlySelected()
	{
		const bool bEnableFlag = !IsShowingOnlySelected();
		GEditor->AccessEditorUserSettings().bShowOnlySelectedActors = bEnableFlag;
		GEditor->AccessEditorUserSettings().PostEditChange();

		ApplyShowOnlySelectedFilter(bEnableFlag);
	}
	void SSceneOutliner::ApplyShowOnlySelectedFilter(bool bShowOnlySelected)
	{
		if ( !SelectedActorFilter.IsValid() )
		{
			SelectedActorFilter = SceneOutlinerFilters::CreateSelectedActorFilter();
		}

		if ( bShowOnlySelected )
		{			
			CustomFilters->Add( SelectedActorFilter );
		}
		else
		{
			CustomFilters->Remove( SelectedActorFilter );
		}
	}
	bool SSceneOutliner::IsShowingOnlySelected() const
	{
		return (GEditor->AccessEditorUserSettings().bShowOnlySelectedActors);
	}

	void SSceneOutliner::ToggleHideTemporaryActors()
	{
		const bool bEnableFlag = !IsHidingTemporaryActors();
		GEditor->AccessEditorUserSettings().bHideTemporaryActors = bEnableFlag;
		GEditor->AccessEditorUserSettings().PostEditChange();

		ApplyHideTemporaryActorsFilter(bEnableFlag);
	}
	void SSceneOutliner::ApplyHideTemporaryActorsFilter(bool bHideTemporaryActors)
	{
		if ( !HideTemporaryActorsFilter.IsValid() )
		{
			HideTemporaryActorsFilter = SceneOutlinerFilters::CreateHideTemporaryActorsFilter();
		}

		if ( bHideTemporaryActors )
		{			
			CustomFilters->Add( HideTemporaryActorsFilter );
		}
		else
		{
			CustomFilters->Remove( HideTemporaryActorsFilter );
		}
	}
	bool SSceneOutliner::IsHidingTemporaryActors() const
	{
		return (GEditor->AccessEditorUserSettings().bHideTemporaryActors);
	}
	/** END FILTERS */


	const FSlateBrush* SSceneOutliner::OnGetBorderBrush() const
	{
		if (bRepresentingPlayWorld)
		{
			return (GEditor->bIsSimulatingInEditor)? SimulateBorder 
												   : PlayInEditorBorder;
		}
		else
		{
			return NoBorder;
		}
	}

	FSlateColor SSceneOutliner::OnGetBorderColorAndOpacity() const
	{
		return (bRepresentingPlayWorld)? FLinearColor(1.0f, 1.0f, 1.0f, 0.6f)
									   : FLinearColor::Transparent;
	}

	ESelectionMode::Type SSceneOutliner::GetSelectionMode() const
	{
		return (InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)? 
			ESelectionMode::Multi : ESelectionMode::Single;
	}


	void SSceneOutliner::Refresh()
	{
		bNeedsRefresh = true;
	}

	void SSceneOutliner::FullRefresh()
	{
		bFullRefresh = true;
		Refresh();
	}


	void SSceneOutliner::Populate()
	{
		// Block events while we clear out the list.  We don't want actors in the level to become deselected
		// while we doing this
		TGuardValue<bool> ReentrantGuard(bIsReentrant, true);
	

		const TArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
		for (int32 WorldIndex=0; WorldIndex < WorldContexts.Num(); ++WorldIndex)
		{
			if (WorldContexts[WorldIndex].WorldType == EWorldType::PIE)
			{
				RepresentingWorld = WorldContexts[WorldIndex].World();
				break;
			}
			else if (WorldContexts[WorldIndex].WorldType == EWorldType::Editor)
			{
				RepresentingWorld = WorldContexts[WorldIndex].World();
			}
		}
		
		if (!RepresentingWorld)
		{
			return;
		}

		bRepresentingPlayWorld = RepresentingWorld->WorldType == EWorldType::PIE;

		bool bMadeAnySignificantChanges = false;
		if(bFullRefresh)
		{
			// Clear the selection here - UpdateActorMapWithWorld will reconstruct it.
			OutlinerTreeView->ClearSelection();

			ActorToTreeItemMap.Reset();
			RootTreeItems.Empty();
			UpdateActorMapWithWorld( ActorToTreeItemMap, RepresentingWorld );

			bMadeAnySignificantChanges = true;
			bFullRefresh = false;
		}
		else
		{
			TSet< AActor* > ParentActors;

			for(int32 ActorIdx = 0; ActorIdx < AddedActorsList.Num(); ++ActorIdx)
			{
				if(AddedActorsList[ActorIdx].IsValid() && AddedActorsList[ActorIdx]->Actor.IsValid() && IsActorDisplayable(AddedActorsList[ActorIdx]->Actor.Get()))
				{
					bMadeAnySignificantChanges |= AddActorToTree(AddedActorsList[ActorIdx], ActorToTreeItemMap, ParentActors);
				}
			}

			for(int32 ActorIdx = 0; ActorIdx < AttachedActorsList.Num(); ++ActorIdx)
			{
				if(AttachedActorsList[ActorIdx].IsValid())
				{
					int32 FoundIndex(INDEX_NONE);
					FoundIndex = RootTreeItems.Find(AttachedActorsList[ActorIdx]);

					if(FoundIndex != INDEX_NONE)
					{
						bMadeAnySignificantChanges = true;

						RootTreeItems.RemoveAt(FoundIndex);

						if( AttachedActorsList[ActorIdx]->Actor.IsValid() )
						{
							// Since the item is being removed from the root, whatever parent (or hierarchy of parents) it has needs to be added to the tree (and likely filtered out).
							AActor* Parent = AttachedActorsList[ActorIdx]->Actor->GetAttachParentActor();
							while(Parent)
							{
								ParentActors.Add(Parent);
								Parent = Parent->GetAttachParentActor();
							}
						}
					}

					// We only synchronize selection when in actor browsing mode
					if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
					{
						// Synchronize selection
						if( GEditor->GetSelectedActors()->IsSelected( AttachedActorsList[ActorIdx]->Actor.Get() ) )
						{
							OutlinerTreeView->SetItemSelection( AttachedActorsList[ActorIdx], true );
						}
					}
				}
			}

			for(int32 ActorIdx = 0; ActorIdx < DetachedActorsList.Num(); ++ActorIdx)
			{
				if(DetachedActorsList[ActorIdx].IsValid())
				{
					bMadeAnySignificantChanges = true;

					--TotalActorCount;

					if(!DetachedActorsList[ActorIdx]->bIsFilteredOut)
					{
						--FilteredActorCount;
					}

					if(!AddActorToTree(DetachedActorsList[ActorIdx], ActorToTreeItemMap, ParentActors))
					{
						// The actor has been filtered out. Remove the item from the map.
						ActorToTreeItemMap.Remove(DetachedActorsList[ActorIdx]->Actor);
					}
				}
			}

			for(int32 ActorIdx = 0; ActorIdx < RemovedActorsList.Num(); ++ActorIdx)
			{
				if(RemovedActorsList[ActorIdx].IsValid())
				{
					int32 FoundIndex(INDEX_NONE);
					FoundIndex = RootTreeItems.Find(RemovedActorsList[ActorIdx]);

					if(FoundIndex != INDEX_NONE)
					{
						bMadeAnySignificantChanges = true;
						--TotalActorCount;

						if(!RemovedActorsList[ActorIdx]->bIsFilteredOut)
						{
							--FilteredActorCount;
						}

						// We only synchronize selection when in actor browsing mode
						if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
						{
							// Synchronize selection since we no longer clear the selection list!
							if( GEditor->GetSelectedActors()->IsSelected( RemovedActorsList[ActorIdx]->Actor.Get() ) )
							{
								OutlinerTreeView->SetItemSelection( RemovedActorsList[ActorIdx], false );
							}
						}

						RootTreeItems.RemoveAt(FoundIndex);
						ActorToTreeItemMap.Remove(RemovedActorsList[ActorIdx]->Actor);
					}
				}
			}

			if( InitOptions.bShowParentTree )
			{
				for( auto ActorIt = ParentActors.CreateConstIterator(); ActorIt; ++ActorIt )
				{
					AActor* Actor = *ActorIt;

					AddFilteredParentActorToTree(Actor, ActorToTreeItemMap);
				}
			}
		}
		
		AddedActorsList.Empty();
		DetachedActorsList.Empty();
		RemovedActorsList.Empty();
		AttachedActorsList.Empty();

		if (bMadeAnySignificantChanges)
		{
			RequestSort();
		}

		// We're fully refreshed now.
		bNeedsRefresh = false;
	}


	void SSceneOutliner::UpdateActorMapWithWorld( FActorToTreeItemMap& ActorMap, UWorld* World )
	{
		if ( World == NULL )
		{
			return;
		}

		TotalActorCount = 0;
		FilteredActorCount = 0;
		ActorMap.Reset();

		TSet< AActor* > ParentActors;

		// We only synchronize selection when in actor browsing mode
		bool bSyncSelection = ( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing );
		FOutlinerTreeItemPtr LastSelectedItem;

		// Iterate over every actor in memory.  WARNING: This is potentially very expensive!
		for( FActorIterator ActorIt(World); ActorIt; ++ActorIt )
		{
			AActor* Actor = *ActorIt;

			FOutlinerTreeItemPtr NewItem = MakeShareable( new TOutlinerTreeItem );
			NewItem->Actor = Actor;

			bool bAddedToTree = AddActorToTree(NewItem, ActorMap, ParentActors);

			//Tag PlayWorld Actors that have counterparts in EditorWorld
			NewItem->bExistsInCurrentWorldAndPIE = GEditor->ObjectsThatExistInEditorWorld.Get( Actor );

			// Synchronize selection - only things actually displayed after filtering are wanted
			if( bAddedToTree && bSyncSelection && GEditor->GetSelectedActors()->IsSelected( Actor ) )
			{
				LastSelectedItem = NewItem;
			}
		}

		/* Any parent actors identified might not have been added to the tree due to filtering. It is important
		   to check these actors and add them to the list so their children can correctly appear during filtering. */
		if( InitOptions.bShowParentTree )
		{
			for( auto ActorIt = ParentActors.CreateConstIterator(); ActorIt; ++ActorIt )
			{
				AddFilteredParentActorToTree(*ActorIt, ActorMap);
			}
		}

		// If we have a selection we may need to scroll to it
		if( bSyncSelection && LastSelectedItem.IsValid() )
		{
			// Scroll last item into view - this means if we are multi-selecting, we show newest selection. @TODO Not perfect though
			OutlinerTreeView->RequestScrollIntoView(LastSelectedItem);
		}
	}

	bool SSceneOutliner::AddActorToTree(FOutlinerTreeItemPtr InActorItem, FActorToTreeItemMap& InActorMap, TSet< AActor* >& InParentActors)
	{
		bool bSuccessful = false;

		if(!InActorItem.IsValid() || !InActorItem->Actor.IsValid())
		{
			return bSuccessful;
		}

		AActor* Actor = InActorItem->Actor.Get();

		if( IsActorDisplayable( Actor ) )
		{
			// Apply custom filters
			if( CustomFilters->PassesAllFilters( Actor ) )
			{
				++TotalActorCount;

				// Apply text filter
				if( SearchBoxActorFilter->PassesFilter( Actor ) )
				{
					if( InitOptions.bShowParentTree )
					{
						// OK so we know that this actor needs to be visible.  Let's gather any parent actors
						// and make sure they're visible as well, so the user can see the hierarchy.
						AActor* ParentActor = Actor->GetAttachParentActor();
						if( ParentActor != NULL )
						{
							for( auto NextParent = ParentActor; NextParent != NULL; NextParent = NextParent->GetAttachParentActor() )
							{
								InParentActors.Add( NextParent );
							}					
						}

						// Setup our root-level item list
						if( ParentActor == NULL )
						{
							RootTreeItems.Add( InActorItem );
						}
					}
					else
					{
						RootTreeItems.Add( InActorItem );
					}

					// Add the item to the map, making it available for display in the tree when its parent is looking for its children.
					InActorMap.Add(Actor, InActorItem);
					bSuccessful = true;

					++FilteredActorCount;

					// We only synchronize selection when in actor browsing mode
					if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
					{
						// Synchronize selection
						if( GEditor->GetSelectedActors()->IsSelected( Actor ) )
						{
							OutlinerTreeView->SetItemSelection( InActorItem, true );
						}
					}
				}
			}
		}
		return bSuccessful;
	}

	void SSceneOutliner::AddFilteredParentActorToTree(AActor* InActor, FActorToTreeItemMap& InActorMap)
	{
		/* Parents that are filtered out (or otherwise found and added to the list) will not appear in the actor map at this point
		   and should be added as requested. If they have no parent, they will be added to the root of the tree, otherwise they will
		   simply be added to the map, so when displaying the tree, their display item can be found. */
		if( !InActorMap.Contains( InActor ) )
		{
			FOutlinerTreeItemPtr NewItem = MakeShareable( new TOutlinerTreeItem );
			NewItem->Actor = InActor;

			InActorMap.Add( InActor, NewItem );

			// Setup our root-level item list
			const auto ParentActor = InActor->GetAttachParentActor();
			if( ParentActor == NULL )
			{
				RootTreeItems.Add( NewItem );
			}

			// We only synchronize selection when in actor browsing mode
			if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				// Synchronize selection
				if( GEditor->GetSelectedActors()->IsSelected( InActor ) )
				{
					OutlinerTreeView->SetItemSelection( NewItem, true );
				}
			}

			NewItem->bIsFilteredOut = true;
		}


		// Make sure all parent actors are expanded by default
		// @todo Outliner: Really we should be remembering expansion state and trying to restore that
		OutlinerTreeView->SetItemExpansion( InActorMap.FindChecked( InActor ), true );
	}

	void SSceneOutliner::PopulateActorSearchStrings( const AActor* const Actor, OUT TArray< FString >& OutSearchStrings ) const
	{
		OutSearchStrings.Add( Actor->GetActorLabel() );
		CustomColumn->PopulateActorSearchStrings( Actor, OUT OutSearchStrings );
	}

	FText SSceneOutliner::GetLabelForActor( FOutlinerTreeItemPtr TreeItem ) const
	{
		const AActor* Actor = TreeItem->Actor.Get();
		return Actor ? FText::FromString( Actor->GetActorLabel() ) : NSLOCTEXT("SceneOutliner", "ActorLabelForMissingActor", "(Deleted Actor)");
	}

	EVisibility SSceneOutliner::IsActorClassNameVisible() const
	{
		return CustomColumn->ProvidesSearchStrings() || FilterTextBoxWidget->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FString SSceneOutliner::GetClassNameForActor( FOutlinerTreeItemPtr TreeItem ) const
	{
		FString LabelText;
		AActor* Actor = TreeItem->Actor.Get();
		if( Actor )
		{
			const FString& CurrentFilterText = FilterTextBoxWidget->GetText().ToString();

			if( !CustomColumn->ProvidesSearchStrings() && !CurrentFilterText.IsEmpty() )
			{
				// If the user is currently searching, then also display the actor class name.  The class name is
				// searchable, too, and we want to indicate that to the user.
				FString ActorClassName;
				Actor->GetClass()->GetName( ActorClassName );
				LabelText = FString::Printf( TEXT( " (%s)" ), *ActorClassName );
			}
		}

		return LabelText;
	}

	FSlateColor SSceneOutliner::GetColorAndOpacityForActor( FOutlinerTreeItemPtr TreeItem ) const
	{
		AActor* Actor = TreeItem->Actor.Get();

		if( Actor == NULL )
		{
			return FLinearColor( 0.2f, 0.2f, 0.25f );		// Deleted actor!
		}

		if ( bRepresentingPlayWorld )
		{
			if( !TreeItem->bExistsInCurrentWorldAndPIE )
			{
				// Highlight actors that are exclusive to PlayWorld
				return FLinearColor( 0.12f, 0.56f, 1.0f );	
			}
		}

		if( !InitOptions.bShowParentTree )
		{
			return FSlateColor::UseForeground();
		}

		// Highlight text differently if it doesn't match the search filter (e.g., parent actors to child actors that
		// match search criteria.)
		if( TreeItem->bIsFilteredOut )
		{
			return FLinearColor( 0.30f, 0.30f, 0.30f );
		}

		// Darken items that aren't suitable targets for an active drag and drop actor attachment action
		bool bDarkenItem = false;
		if ( FSlateApplication::Get().IsDragDropping() )
		{
			TSharedPtr<FDragDropOperation> DragDropOp = FSlateApplication::Get().GetDragDroppingContent();

			if (DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropOp))	
			{
				TSharedPtr<FActorDragDropGraphEdOp> DragDropActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropOp);

				bool CanAttach = true;
				for(int32 i=0; i<DragDropActorOp->Actors.Num(); i++)
				{
					AActor* ChildActor = DragDropActorOp->Actors[i].Get();
					if (ChildActor)
					{
						if (!GEditor->CanParentActors(Actor, ChildActor))
						{
							bDarkenItem = true;
							break;
						}
					}
				}
			}
		}

		// also darken items that are non selectable in the active mode(s)
		const bool bInSelected = true;
		const bool bSelectEvenIfHidden = true;		// @todo outliner: Is this actually OK?
		bDarkenItem |= !GEditor->CanSelectActor( Actor, bInSelected, bSelectEvenIfHidden );

		return (!bDarkenItem) ? FSlateColor::UseForeground() : FLinearColor( 0.30f, 0.30f, 0.30f );
	}

	FText SSceneOutliner::GetToolTipTextForActorIcon( FOutlinerTreeItemPtr TreeItem ) const
	{
		FText ToolTipText;
		AActor* Actor = TreeItem->Actor.Get();

		if( Actor )
		{
			ToolTipText = FText::FromString( Actor->GetClass()->GetName() );
			if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				USceneComponent* RootComponent = Actor->GetRootComponent();
				if( RootComponent )
				{
					FFormatNamedArguments Args;
					Args.Add( TEXT("ActorClassName"), ToolTipText );

					if( RootComponent->Mobility == EComponentMobility::Static )
					{
						ToolTipText = FText::Format( LOCTEXT( "ComponentMobility_Static", "Static {ActorClassName}" ), Args );
					}
					else if( RootComponent->Mobility == EComponentMobility::Stationary )
					{
						ToolTipText = FText::Format( LOCTEXT( "ComponentMobility_Stationary", "Stationary {ActorClassName}" ), Args );
					}
					else if( RootComponent->Mobility == EComponentMobility::Movable )
					{
						ToolTipText = FText::Format( LOCTEXT( "ComponentMobility_Movable", "Movable {ActorClassName}" ), Args );
					}
				}
			}
		}

		return ToolTipText;
	}

	const FSlateBrush* SSceneOutliner::GetBrushForComponentMobilityIcon( FOutlinerTreeItemPtr TreeItem ) const
	{
		const FSlateBrush* IconBrush = MobilityStaticBrush;
		AActor* Actor = TreeItem->Actor.Get();

		if( Actor )
		{
			USceneComponent* RootComponent = Actor->GetRootComponent();
			if( RootComponent )
			{
				if( RootComponent->Mobility == EComponentMobility::Stationary )
				{
					IconBrush = MobilityStationaryBrush;
				}
				else if( RootComponent->Mobility == EComponentMobility::Movable )
				{
					IconBrush = MobilityMovableBrush;
				}
			}
		}

		return IconBrush;
	}

	TSharedRef< SWidget > SSceneOutliner::GenerateWidgetForItemAndColumn( FOutlinerTreeItemPtr Item, const FName ColumnID, FIsSelected InIsSelected ) const
	{
		struct Local
		{
			/** Gets the tool-tip text for an actor */
			static FString GetToolTipTextForActor( FOutlinerTreeItemPtr TreeItem )
			{
				const auto Actor = TreeItem.Get()->Actor;
				if( Actor != NULL )
				{
					return FString::Printf( TEXT( "%s: %s" ), *LOCTEXT("CustomColumnMode_InternalName", "ID Name").ToString(), *Actor->GetName() );
				}

				return FString();
			}

		};

		// Find the icon that goes with this actor's type
		AActor* Actor = Item->Actor.Get();
		const FSlateBrush* IconBrush = FClassIconFinder::FindIconForActor(Actor);		
		TSharedPtr< SWidget > TableRowContent;

		if( ColumnID == ColumnID_ActorLabel )
		{
			TSharedPtr< SInlineEditableTextBlock > InlineTextBlock;
			TSharedPtr< SHorizontalBox > HBox = SNew( SHorizontalBox );

			if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				// Add the component mobility icon
				HBox->AddSlot()
					.AutoWidth()
					.Padding( 0.0f, 1.0f, 1.0f, 0.0f )
					[
						SNew( SImage )
						.Image( this, &SSceneOutliner::GetBrushForComponentMobilityIcon, Item )
					];
			}

			const FSlateColor IconColor = InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing ? FLinearColor::White : FEditorStyle::GetSlateColor( "DefaultForeground" );

			HBox->AddSlot()
				.AutoWidth()
				.Padding( 0.0f, 2.0f, 6.0f, 0.0f )
				[
					SNew( SImage )
						.Image( IconBrush )
						.ToolTipText( this, &SSceneOutliner::GetToolTipTextForActorIcon, Item )
						.ColorAndOpacity( IconColor )
				];

			HBox->AddSlot()
				.FillWidth(1.0f)
				.Padding( 0.0f, 3.0f, 0.0f, 0.0f )
				[
					SAssignNew( InlineTextBlock, SInlineEditableTextBlock )

						// Bind a delegate for getting the actor's name.  This ensures that the current actor name is
						// shown accurately, even if it gets renamed while the tree is visible. (Without requiring us
						// to refresh anything.)
 						.Text( this, &SSceneOutliner::GetLabelForActor, Item ) 

						// Bind our filter text as the highlight string for the text block, so that when the user
						// starts typing search criteria, this text highlights
						.HighlightText( this, &SSceneOutliner::GetFilterHighlightText )

						// Use the actor's actual FName as the tool-tip text
						.ToolTipText_Static( &Local::GetToolTipTextForActor, Item )

						// Actor names that pass the search criteria are shown in a brighter color
						.ColorAndOpacity( this, &SSceneOutliner::GetColorAndOpacityForActor, Item )

						.OnTextCommitted( this, &SSceneOutliner::OnActorLabelCommitted, Item->Actor )

						.OnVerifyTextChanged( this, &SSceneOutliner::OnVerifyActorLabelChanged )

						.IsSelected( InIsSelected )
				];

			HBox->AddSlot()
				.AutoWidth()
				.Padding( 0.0f, 3.0f, 3.0f, 0.0f )
				[
					SNew( STextBlock )

						// Bind a delegate for getting the actor's class name
 						.Text( this, &SSceneOutliner::GetClassNameForActor, Item )

						.Visibility( this, &SSceneOutliner::IsActorClassNameVisible )

						// Bind our filter text as the highlight string for the text block, so that when the user
						// starts typing search criteria, this text highlights
						.HighlightText( this, &SSceneOutliner::GetFilterHighlightText )
				];

			TableRowContent = 
				SNew( SSceneOutlinerItemWidget )
					.Content()
					[
						HBox.ToSharedRef()
					];

			Item->RenameRequestEvent.BindSP( InlineTextBlock.Get(), &SInlineEditableTextBlock::EnterEditingMode );

		}
		else if( Gutter.IsValid() && ColumnID == Gutter->GetColumnID() )
		{
			TableRowContent = Gutter->ConstructRowWidget( Actor );
		}
		else if( ensure( ColumnID == CustomColumn->GetColumnID() ) )
		{
			TableRowContent = CustomColumn->ConstructRowWidget( Actor );
		}

		return TableRowContent.ToSharedRef();
	}

	void SSceneOutliner::OnActorLabelCommitted( const FText& InLabel, ETextCommit::Type InCommitInfo, TWeakObjectPtr<AActor> InActor )
	{
		if(InLabel.ToString() != InActor->GetActorLabel() && InActor->IsActorLabelEditable())
		{
			const FScopedTransaction Transaction( LOCTEXT( "SceneOutlinerRenameActorTransaction", "Rename Actor" ) );
			InActor->SetActorLabel( InLabel.ToString() );

			// Set the keyboard focus back to the SceneOutliner, so the user can perform keyboard commands
			FWidgetPath SceneOutlinerWidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetUnchecked( SharedThis( this ), SceneOutlinerWidgetPath );
			FSlateApplication::Get().SetKeyboardFocus( SceneOutlinerWidgetPath, EKeyboardFocusCause::SetDirectly );
		}
	}

	bool SSceneOutliner::OnVerifyActorLabelChanged( const FText& InLabel, FText& OutErrorMessage )
	{
		if( InLabel.IsEmpty() )
		{
			OutErrorMessage = LOCTEXT( "RenameFailed_LeftBlank", "Names cannot be left blank" );
			return false;
		}

		if(InLabel.ToString().Len() >= NAME_SIZE)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("CharCount"), NAME_SIZE );
			OutErrorMessage = FText::Format(LOCTEXT("RenameFailed_TooLong", "Actor names must be less than {CharCount} characters long."), Arguments);
			return false;
		}

		return true;
	}

	void SSceneOutliner::ExpandActor(AActor* Actor)
	{
		if( !InitOptions.bShowParentTree )
		{
			return;
		}

		OutlinerTreeView->SetItemExpansion(*ActorToTreeItemMap.Find(Actor), true);
	}

	TSharedRef< ITableRow > SSceneOutliner::OnGenerateRowForOutlinerTree( FOutlinerTreeItemPtr Item, const TSharedRef< STableViewBase >& OwnerTable )
	{
		return
			SNew( SSceneOutlinerTreeRow, OwnerTable )
				.SceneOutliner( SharedThis( this ) )
				.Item( Item );
	}


	void SSceneOutliner::OnGetChildrenForOutlinerTree( FOutlinerTreeItemPtr InParent, TArray< FOutlinerTreeItemPtr >& OutChildren )
	{
		if( !InitOptions.bShowParentTree )
		{
			return;
		}

		AActor* ParentActor = InParent->Actor.Get();
		if( ParentActor != NULL )
		{
			// Does this actor have anything attached?
			static TArray< AActor* > AttachedActors;
			AttachedActors.Reset();
			// @todo outliner: Perf: This call always will allocate heap memory and do a bit of work, even for actors with no children!
			ParentActor->GetAttachedActors( AttachedActors );

			if( AttachedActors.Num() > 0 )
			{
				// Walk the list of attached actors and add their tree nodes to the output array
				for( TArray< AActor* >::TConstIterator AttachedActorIt( AttachedActors ); AttachedActorIt; ++AttachedActorIt )
				{
					AActor* ChildActor = *AttachedActorIt;
					if( ensure( ChildActor != NULL ) )
					{
						// NOTE: We don't call Filters->PassesAllFilters() here because we may need to display branches
						// in the tree for actors that don't pass our actor inclusion list, so that we can
						// represent the heirarchy correctly.  We only call IsActorDisplayable() to avoid showing
						// deleted actors and such.

						// We MUST limit child actors displayed to those whose root component is attached to this actor
						// Otherwise, the attached actors could be present elsewhere in the hierarchy, causing a crash
						if( ChildActor->GetAttachParentActor() == ParentActor && IsActorDisplayable( ChildActor ) )
						{
							// NOTE: We don't need to apply filters here because we always repopulate the tree after
							// the filter changes.  Besides, we may need to display child actors that were filtered out
							// simply because they may have their own children that do match the filter.

							// Find this actor in our list of tree items for each actor
							FOutlinerTreeItemPtr ChildTreeItem = ActorToTreeItemMap.FindRef( ChildActor );

							// There's a chance we won't actually have a tree item for the child actor (e.g. because
							// Outliner wasn't notified when an actor was destroyed or reparented), so we handle that here.
							// In any other case though, we should always have an item for the actor in our list
							if( ChildTreeItem.IsValid() )
							{
								OutChildren.Add( ChildTreeItem );
							}
						}
					}
				}

				if (SortByColumn == ColumnID_ActorLabel)
				{
					// Always sort children by their name ascending, regardless of current sort mode
					OutChildren.Sort(Helpers::FSortByActorAscending());
				}
				else if (CustomColumn.IsValid())
				{
					CustomColumn->SortItems(OutChildren, SortMode);
				}
			}
		}
	}


	bool SSceneOutliner::IsActorDisplayable( const AActor* Actor ) const
	{
		return (
			Actor->IsEditable() &&			// Only show actors that are allowed to be selected and drawn in editor
			Actor->IsListedInSceneOutliner() &&
			!Actor->IsTemplate() &&			// Should never happen, but we never want CDOs displayed
			!FActorEditorUtils::IsABuilderBrush(Actor) &&	// Don't show the builder brush
			!Actor->IsA( AWorldSettings::StaticClass() ) &&		// Don't show the WorldSettings actor, even though it is technically editable
			!Actor->IsPendingKill() &&						// We don't want to show actors that are about to go away
			FLevelUtils::IsLevelVisible( Actor->GetLevel() ) &&					// Only show Actors whose level is visible
			!FLevelEditorViewportClient::GetDropPreviewActors().Contains(Actor) );	// Don't display drag and drop preview actors (transient temp actors)
	}


	void SSceneOutliner::OnOutlinerTreeSelectionChanged( FOutlinerTreeItemPtr TreeItem, ESelectInfo::Type /*SelectInfo*/ )
	{
		if( InitOptions.Mode == ESceneOutlinerMode::ActorPicker )
		{
			// In actor picking mode, we check to see if we have a selected actor, and if so, fire
			// off the notification to whoever is listening.  This may often cause the widget itself
			// to be enqueued for destruction
			FOutlinerData SelectedItems = OutlinerTreeView->GetSelectedItems();
			if( SelectedItems.Num() > 0 )
			{
				AActor* FirstSelectedActor = SelectedItems[ 0 ]->Actor.Get();
				if( FirstSelectedActor != NULL )				
				{
					// Only fire the callback if this actor actually passes our required filters.  It's possible that
					// some actors displayed are parent actors which don't actually match the requested criteria.
					if( IsActorDisplayable( FirstSelectedActor ) && 
						CustomFilters->PassesAllFilters( FirstSelectedActor ) &&
						SearchBoxActorFilter->PassesFilter( FirstSelectedActor ) )
					{
						// Signal that an actor was selected
						OnActorPicked.ExecuteIfBound( FirstSelectedActor );
					}
				}
			}
		}

		// We only synchronize selection when in actor browsing mode
		else if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			if( !bIsReentrant )
			{
				TGuardValue<bool> ReentrantGuard(bIsReentrant,true);

				// @todo outliner: Could be called frequently.  Use dirty bit?
				// @todo outliner: Can be called from non-interactive selection

				// The tree let us know that selection has changed, but wasn't able to tell us
				// what changed.  So we'll perform a full difference check and update the editor's
				// selected actors to match the control's selection set.

				// Make a list of all the actors that should now be selected in the world.
				TArray< AActor* > ActorsToSelect;
				const FOutlinerData SelectedItems = OutlinerTreeView->GetSelectedItems();
				for( FOutlinerData::TConstIterator SelectedItemIt( SelectedItems ); SelectedItemIt; ++SelectedItemIt )
				{
					AActor* Actor = ( *SelectedItemIt )->Actor.Get();
					if( Actor != NULL )
					{
						ActorsToSelect.Add( Actor );
					}
				}

				bool bChanged = false;

				for( int32 ActorIndex = 0; ActorIndex < ActorsToSelect.Num() && !bChanged; ActorIndex++ )
				{
					AActor* Actor = ActorsToSelect[ ActorIndex ];
					if ( !GEditor->GetSelectedActors()->IsSelected( Actor ) )
					{
						// Actor has been selected
						bChanged = true;
					}
				}
				for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt && !bChanged; ++SelectionIt )
				{
					AActor* Actor = CastChecked< AActor >( *SelectionIt );
					if ( !ActorsToSelect.Contains( Actor ) )
					{
						// Actor has been deselected
						bChanged = true;

						// If actor was a group actor, remove its members from the ActorsToSelect list
						AGroupActor* DeselectedGroupActor = Cast<AGroupActor>(Actor);
						if ( DeselectedGroupActor )
						{
							TArray<AActor*> GroupActors;
							DeselectedGroupActor->GetGroupActors( GroupActors );

							for ( auto GroupActor : GroupActors )
							{
								ActorsToSelect.RemoveSingle( GroupActor );
							}

						}
					}
				}

				// If there's a discrepancy, update the selected actors to reflect this list.
				if ( bChanged )
				{
					const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "ClickingOnActors", "Clicking on Actors") );
					GEditor->GetSelectedActors()->Modify();

					// Clear the selection.
					GEditor->SelectNone(false, true, true);

					// We'll batch selection changes instead by using BeginBatchSelectOperation()
					GEditor->GetSelectedActors()->BeginBatchSelectOperation();

					const bool bShouldSelect = true;
					const bool bNotifyAfterSelect = false;
					const bool bSelectEvenIfHidden = true;	// @todo outliner: Is this actually OK?
					for( int32 ActorIndex = 0; ActorIndex < ActorsToSelect.Num(); ActorIndex++ )
					{
						AActor* Actor = ActorsToSelect[ ActorIndex ];
						UE_LOG(LogSceneOutliner, Verbose,  TEXT("Clicking on Actor (scene outliner): %s (%s)"), *Actor->GetClass()->GetName(), *Actor->GetActorLabel());
						GEditor->SelectActor( Actor, bShouldSelect, bNotifyAfterSelect, bSelectEvenIfHidden );
					}

					// Commit selection changes
					GEditor->GetSelectedActors()->EndBatchSelectOperation();

					// Fire selection changed event
					GEditor->NoteSelectionChange();
				}

				// Synchronize selection - actors may or may not have been selected depending on 
				// the editor mode we are in (and their selectablity). Grouping may also have selected
				// other actors.
				OutlinerTreeView->ClearSelection();
				for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt; ++SelectionIt )
				{
					AActor* Actor = CastChecked< AActor >( *SelectionIt );
					FOutlinerTreeItemPtr TreeItem = ActorToTreeItemMap.FindRef( Actor );
					if( TreeItem.IsValid() )
					{
						OutlinerTreeView->SetItemSelection( TreeItem, true );
					}
				}

				// Broadcast selection changed delegate
				SelectionChanged.Broadcast();
			}
		}
	}

	
	void SSceneOutliner::OnLevelSelectionChanged(UObject* Obj)
	{
		// We only synchronize selection when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// @todo outliner: Because we are not notified of which items are being added/removed from selection, we have
			// no immediate means to incrementally update the tree when selection changes.

			// Ideally, we can improve the filtering paradigm to better support incremental updates in cases such as these
			if ( IsShowingOnlySelected() )
			{
				FullRefresh();
			}

			if( !bIsReentrant )
			{
				TGuardValue<bool> ReentrantGuard(bIsReentrant, true);

				// @todo outliner: Called twice quite frequently (once for deselect, once for select.)  Change to dirty bit pattern.

				// The selected actors in the level have changed, so update our tree control's selection state
				// to match the level.
				{
					OutlinerTreeView->ClearSelection();

					ValidateOutlinerTreeView();

					// Synchronize selection
					FOutlinerTreeItemPtr LastSelectedItem;
					for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt; ++SelectionIt )
					{
						AActor* Actor = CastChecked< AActor >( *SelectionIt );
						FOutlinerTreeItemPtr TreeItem = ActorToTreeItemMap.FindRef( Actor );
						if( TreeItem.IsValid() )
						{
							OutlinerTreeView->SetItemSelection( TreeItem, true );

							LastSelectedItem = TreeItem;
						}
					}

					// Scroll last item into view - this means if we are multi-selecting, we show newest selection. @TODO Not perfect though
					if(LastSelectedItem.IsValid() && !OutlinerTreeView->IsItemVisible( LastSelectedItem ))
					{
						OutlinerTreeView->RequestScrollIntoView(LastSelectedItem);
					}
				}

			}
		}
	}


	void SSceneOutliner::ValidateOutlinerTreeView()
	{
		bool bRefresh = false;
		for( FActorToTreeItemMap::TConstIterator ActorIt( ActorToTreeItemMap ); ActorIt; ++ActorIt )
		{
			AActor* Actor = ActorIt.Key().Get();
			if( Actor != NULL )
			{
				//Check whether actor is marked for deletion
				if( Actor->HasAnyFlags(RF_PendingKill) )
				{
					bRefresh = true;
					break;
				}
			}
		}
		if( bRefresh )
		{
			Populate();
		}
	}

	void SSceneOutliner::OnOutlinerTreeDoubleClick( FOutlinerTreeItemPtr TreeItem )
	{
		// We only modify viewport cameras when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Move viewport cameras to the actor that was double-clicked on
			const FOutlinerData SelectedItems = OutlinerTreeView->GetSelectedItems();
			TArray< AActor* > SelectedActors;
			for( FOutlinerData::TConstIterator SelectedItemIt( SelectedItems ); SelectedItemIt; ++SelectedItemIt )
			{
				AActor* Actor = ( *SelectedItemIt )->Actor.Get();
				if( Actor != NULL )
				{
					SelectedActors.Add( Actor );
				}
			}

			if( SelectedActors.Num() > 0 )
			{
				const bool bActiveViewportOnly = false;
				GEditor->MoveViewportCamerasToActor( SelectedActors, bActiveViewportOnly );
			}
		}
	}

	void SSceneOutliner::OnOutlinerTreeItemScrolledIntoView( FOutlinerTreeItemPtr TreeItem, const TSharedPtr<ITableRow>& Widget )
	{
		if(TreeItem->bRenameWhenScrolledIntoView)
		{
			TreeItem->bRenameWhenScrolledIntoView = false;

			if(Widget.IsValid() && Widget->GetContent().IsValid())
			{
				TreeItem->RenameRequestEvent.ExecuteIfBound();
			}
		}
	}

	void SSceneOutliner::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
	{
		FullRefresh();
	}

	void SSceneOutliner::OnLevelRemoved(ULevel* InLevel, UWorld* InWorld)
	{
		FullRefresh();
	}

	void SSceneOutliner::OnLevelActorsAdded(AActor* InActor)
	{
		// @todo outliner: Currently, attachment changes aren't causing us to re-populate
		if( !bIsReentrant )
		{
			if( InActor && RepresentingWorld == InActor->GetWorld() )
			{
				if(!ActorToTreeItemMap.Find(InActor))
				{
					FOutlinerTreeItemPtr NewItem = MakeShareable( new TOutlinerTreeItem );
					NewItem->Actor = InActor;

					AddedActorsList.AddUnique(NewItem);
				}

				Refresh();
			}
		}
	}

	void SSceneOutliner::OnLevelActorsRemoved(AActor* InActor)
	{
		if( !bIsReentrant )
		{
			if( InActor && RepresentingWorld == InActor->GetWorld() )
			{
				FOutlinerTreeItemPtr* ItemPtr = ActorToTreeItemMap.Find(InActor);
				if(ItemPtr)
				{
					RemovedActorsList.AddUnique(*ItemPtr);
					Refresh();
				}
			}
		}
	}

	void SSceneOutliner::OnLevelActorsAttached(AActor* InActor, const AActor* InParent)
	{
		// InActor can be equal to InParent in cases of components being attached internally. The Scene Outliner does not need to do anything in this case.
		if( !bIsReentrant && InActor != InParent )
		{
			if( InActor && RepresentingWorld == InActor->GetWorld() )
			{
				FOutlinerTreeItemPtr* ItemPtr = ActorToTreeItemMap.Find(InActor);
				if(ItemPtr)
				{
					AttachedActorsList.AddUnique(*ItemPtr);
					Refresh();
				}
			}
		}
	}

	void SSceneOutliner::OnLevelActorsDetached(AActor* InActor, const AActor* InParent)
	{
		// InActor can be equal to InParent in cases of components being attached internally. The Scene Outliner does not need to do anything in this case.
		if( !bIsReentrant && InActor != InParent)
		{
			if( InActor && RepresentingWorld == InActor->GetWorld() )
			{
				FOutlinerTreeItemPtr* ItemPtr = ActorToTreeItemMap.Find(InActor);

				if(ItemPtr)
				{
					DetachedActorsList.AddUnique(*ItemPtr);
					Refresh();
				}
				else
				{
					// We should find the item, but if we don't, do an add.
					OnLevelActorsAdded(InActor);
				}
			}

			if( InParent && RepresentingWorld == InParent->GetWorld() )
			{
				// See if any of the remaining filtered actors still have this as a parent, 
				// and if not remove it from the list too. If by chance its name matches the 
				// current filter it'll get automatically re-added to the list.
				bool bHasVisibileChild = false;
				TArray<AActor*> AttachedActors;
				InParent->GetAttachedActors(AttachedActors);
				for (TArray< AActor* >::TConstIterator AttachedActorIt(AttachedActors); AttachedActorIt; ++AttachedActorIt)
				{
					const AActor* ChildActor = *AttachedActorIt;

					if (ChildActor != NULL && ActorToTreeItemMap.Contains( ChildActor ) )
					{
						bHasVisibileChild = true;
						break;
					}
				}
				if ( !bHasVisibileChild )
				{
					// No children remaining, make sure this gets re-evaluated for the list too
					FOutlinerTreeItemPtr* ItemPtr = ActorToTreeItemMap.Find(InParent);

					if(ItemPtr)
					{
						RootTreeItems.Remove(*ItemPtr);
						DetachedActorsList.AddUnique(*ItemPtr);
						Refresh();
					}
				}
			}
		}
	}

	void SSceneOutliner::OnLevelActorsRequestRename(const AActor* InActor)
	{
		TArray< TSharedPtr < TOutlinerTreeItem > > SelectedItems = OutlinerTreeView->GetSelectedItems();
		if( SelectedItems.Num() > 0)
		{
			// Ensure that the item we want to rename is visible in the tree
			TSharedPtr<TOutlinerTreeItem> ItemToRename = SelectedItems[SelectedItems.Num() - 1];
			ItemToRename->bRenameWhenScrolledIntoView = true;
			OutlinerTreeView->RequestScrollIntoView(ItemToRename);
		}
	}

	void SSceneOutliner::PostUndo(bool bSuccess)
	{
		// Refresh our tree in case any changes have been made to the scene that might effect our actor list
		if( !bIsReentrant )
		{
			FullRefresh();
		}
	}

	void SSceneOutliner::OnActorLabelChanged(AActor* ChangedActor)
	{
		if ( !ensure(ChangedActor) )
		{
			return;
		}
		
		FOutlinerTreeItemPtr TreeItem = ActorToTreeItemMap.FindRef( ChangedActor );
		OutlinerTreeView->FlashHighlightOnItem(TreeItem);
	}

	void SSceneOutliner::OnFilterTextChanged( const FText& InFilterText )
	{
		SearchBoxActorFilter->SetRawFilterText( InFilterText );
	}

	void SSceneOutliner::OnFilterTextCommitted( const FText& InFilterText, ETextCommit::Type CommitInfo )
	{
		const FString CurrentFilterText = InFilterText.ToString();
		// We'll only select actors if the user actually pressed the enter key.  We don't want to change
		// selection just because focus was lost from the search text field.
		if( CommitInfo == ETextCommit::OnEnter )
		{
			// Any text in the filter?  If not, we won't bother doing anything
			if( !CurrentFilterText.IsEmpty() )
			{
				// Gather all of the actors that match the filter text
				TArray< AActor* > ActorsToSelect;
				for( FActorToTreeItemMap::TConstIterator ActorIt( ActorToTreeItemMap ); ActorIt; ++ActorIt )
				{
					AActor* Actor = ActorIt.Key().Get();
					if( Actor != NULL )
					{
						// NOTE: The actor should have already passed this test earlier when we added it to the tree, but
						// we'll run it again just to be safe, as the data might be stale (e.g. the actor might now be
						// pending kill), in which case we don't want to try to select it
						if( IsActorDisplayable( Actor ) )
						{
							if( CustomFilters->PassesAllFilters( Actor ) && SearchBoxActorFilter->PassesFilter( Actor ) )
							{
								ActorsToSelect.Add( Actor );
							}
						}
					}
				}

				// We only select level actors when in actor browsing mode
				if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
				{
					// Start batching selection changes
					GEditor->GetSelectedActors()->BeginBatchSelectOperation();

					// Select actors (and only the actors) that match the filter text
					const bool bNoteSelectionChange = false;
					const bool bDeselectBSPSurfs = false;
					const bool WarnAboutManyActors = true;
					GEditor->SelectNone( bNoteSelectionChange, bDeselectBSPSurfs, WarnAboutManyActors );
					for( TArray< AActor* >::TConstIterator ActorIt( ActorsToSelect ); ActorIt; ++ActorIt )
					{
						AActor* Actor = *ActorIt;

						const bool bShouldSelect = true;
						const bool bSelectEvenIfHidden = false;
						GEditor->SelectActor( Actor, bShouldSelect, bNoteSelectionChange, bSelectEvenIfHidden );
					}

					// Commit selection changes
					GEditor->GetSelectedActors()->EndBatchSelectOperation();

					// Fire selection changed event
					GEditor->NoteSelectionChange();

					// Set keyboard focus to the SceneOutliner, so the user can perform keyboard commands that interact
					// with selected actors (such as Delete, to delete selected actors.)
					{
						// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
						FWidgetPath SceneOutlinerWidgetPath;
						FSlateApplication::Get().GeneratePathToWidgetUnchecked( SharedThis( this ), SceneOutlinerWidgetPath );

						// Set keyboard focus directly
						FSlateApplication::Get().SetKeyboardFocus( SceneOutlinerWidgetPath, EKeyboardFocusCause::SetDirectly );
					}
				}

				// In 'actor picking' mode, we allow the user to commit their selection by pressing enter
				// in the search window when a single actor is available
				else if( InitOptions.Mode == ESceneOutlinerMode::ActorPicker )
				{
					// In actor picking mode, we check to see if we have a selected actor, and if so, fire
					// off the notification to whoever is listening.  This may often cause the widget itself
					// to be enqueued for destruction
					if( ActorsToSelect.Num() == 1 )
					{
						AActor* FirstSelectedActor = ActorsToSelect[ 0 ];
						if( FirstSelectedActor != NULL )				
						{
							// Only fire the callback if this actor actually passes our required filters.  It's possible that
							// some actors displayed are parent actors which don't actually match the requested criteria.
							if( IsActorDisplayable( FirstSelectedActor ) && 
								CustomFilters->PassesAllFilters( FirstSelectedActor ) &&
								SearchBoxActorFilter->PassesFilter( FirstSelectedActor ) )
							{
								// Signal that an actor was selected
								OnActorPicked.ExecuteIfBound( FirstSelectedActor );
							}
						}
					}
				}
			}
		}
	}

	EVisibility SSceneOutliner::GetSearchBoxVisibility() const
	{
		return InitOptions.bShowSearchBox ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility SSceneOutliner::GetFilterStatusVisibility() const
	{
		return IsFilterActive() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility SSceneOutliner::GetEmptyLabelVisibility() const
	{
		return ( IsFilterActive() || RootTreeItems.Num() > 0 ) ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FString SSceneOutliner::GetFilterStatusText() const
	{
		const int32 SelectedActorCount = OutlinerTreeView->GetNumItemsSelected();

		if ( !IsFilterActive() )
		{
			if (SelectedActorCount == 0)
			{
				return FString::Printf( *LOCTEXT("ShowingAllActors", "%d actors").ToString(), TotalActorCount );
			}
			else
			{
				return FString::Printf( *LOCTEXT("ShowingAllActorsSelected", "%d actors (%d selected)").ToString(), TotalActorCount, SelectedActorCount );
			}
		}
		else if( IsFilterActive() && FilteredActorCount == 0 )
		{
			return FString::Printf( *LOCTEXT("ShowingNoActors", "No matching actors (%d total)").ToString(), TotalActorCount );
		}
		else if (SelectedActorCount != 0)
		{
			return FString::Printf( *LOCTEXT("ShowingOnlySomeActorsSelected", "Showing %d of %d actors (%d selected)").ToString(), FilteredActorCount, TotalActorCount, SelectedActorCount );
		}
		else
		{
			return FString::Printf( *LOCTEXT("ShowingOnlySomeActors", "Showing %d of %d actors").ToString(), FilteredActorCount, TotalActorCount );
		}
	}

	FSlateColor SSceneOutliner::GetFilterStatusTextColor() const
	{
		if ( !IsFilterActive() )
		{
			// White = no text filter
			return FLinearColor( 1.0f, 1.0f, 1.0f );
		}
		else if( FilteredActorCount == 0 )
		{
			// Red = no matching actors
			return FLinearColor( 1.0f, 0.4f, 0.4f );
		}
		else
		{
			// Green = found at least one match!
			return FLinearColor( 0.4f, 1.0f, 0.4f );
		}
	}

	bool SSceneOutliner::IsFilterActive() const
	{
		return FilterTextBoxWidget->GetText().ToString().Len() > 0 && TotalActorCount != FilteredActorCount;
	}

	const FSlateBrush* SSceneOutliner::GetFilterButtonGlyph() const
	{
		if( IsFilterActive() )
		{
			return FEditorStyle::GetBrush(TEXT("SceneOutliner.FilterCancel"));
		}
		else
		{
			return FEditorStyle::GetBrush(TEXT("SceneOutliner.FilterSearch"));
		}
	}

	FString SSceneOutliner::GetFilterButtonToolTip() const
	{
		return IsFilterActive() ? LOCTEXT("ClearSearchFilter", "Clear search filter").ToString() : LOCTEXT("StartSearching", "Search").ToString();

	}

	FText SSceneOutliner::GetFilterHighlightText() const
	{
		return SearchBoxActorFilter->GetRawFilterText();
	}


	bool SSceneOutliner::SupportsKeyboardFocus() const
	{
		// We only need to support keyboard focus if we're in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Scene outliner needs keyboard focus so the user can press keys to activate commands, such as the Delete
			// key to delete selected actors
			return true;
		}

		return false;
	}

	FReply SSceneOutliner::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
	{
		// @todo outliner: Use command system for these for discoverability? (allow bindings?)

		// We only allow users to delete actors when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Delete key: Delete selected actors (not rebindable, because it doesn't make much sense to bind.)
			if( InKeyboardEvent.GetKey() == EKeys::Delete )
			{
				if( InitOptions.CustomDelete.IsBound() )
				{
					TArray< TWeakObjectPtr< AActor > > SelectedActors;

					for ( FSelectionIterator It( GEditor->GetSelectedActorIterator() ) ; It ; ++It )
					{
						UObject* Object = *It;
						if( Object->IsA( AActor::StaticClass() ) )
						{
							AActor* Actor = static_cast<AActor*>( *It );
							SelectedActors.Add( Actor );
						}
					}

					InitOptions.CustomDelete.Execute( SelectedActors );
				}
				else
				{
					const FScopedTransaction Transaction( LOCTEXT("UndoAction_DeleteSelectedActors", "Delete selected actors") );

					// Code from FLevelEditorActionCallbacks::Delete_CanExecute()
					// Should this be just return FReply::Unhandled()?
					TArray<FEdMode*> ActiveModes; 
					GEditorModeTools().GetActiveModes( ActiveModes );
					for( int32 ModeIndex = 0; ModeIndex < ActiveModes.Num(); ++ModeIndex )
					{
						const EEditAction::Type CanProcess = ActiveModes[ModeIndex]->GetActionEditDelete();
						if (CanProcess == EEditAction::Process)
						{
							return FReply::Handled();
						}
						else if (CanProcess == EEditAction::Halt)
						{
							return FReply::Unhandled();
						}
					}

					if (GUnrealEd->CanDeleteSelectedActors( GWorld, true, false ))
					{
						GEditor->edactDeleteSelected( GWorld );
					}
				}

				return FReply::Handled();
			}
		}

		return FReply::Unhandled();
	}


	void SSceneOutliner::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
	{
		// Call parent implementation
		SCompoundWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

		if ( bPendingFocusNextFrame && FilterTextBoxWidget->GetVisibility() == EVisibility::Visible )
		{
			FWidgetPath WidgetToFocusPath;
			FSlateApplication::Get().GeneratePathToWidgetUnchecked( FilterTextBoxWidget.ToSharedRef(), WidgetToFocusPath );
			FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
			bPendingFocusNextFrame = false;
		}

		if( bNeedsRefresh )
		{
			if( !bIsReentrant )
			{
				Populate();
			}
		}

		if( SortOutlinerTimer >= SCENE_OUTLINER_RESORT_TIMER )
		{
			SortTree();
		}
		SortOutlinerTimer -= InDeltaTime;
	}

	EColumnSortMode::Type SSceneOutliner::GetColumnSortMode( const FName ColumnId ) const
	{
		if ( SortByColumn != ColumnId )
		{
			return EColumnSortMode::None;
		}

		return SortMode;
	}

	void SSceneOutliner::OnColumnSortModeChanged( const FName& ColumnId, EColumnSortMode::Type InSortMode )
	{
		if (CustomColumn.IsValid() && ColumnId == CustomColumn->GetColumnID() && !CustomColumn->SupportsSorting())
		{
			return;
		}

		SortByColumn = ColumnId;
		SortMode = InSortMode;

		RequestSort();
	}

	void SSceneOutliner::RequestSort()
	{
		if (bRepresentingPlayWorld)
		{
			if (SortOutlinerTimer < 0.0f || OutlinerTreeView->HasKeyboardFocus())
			{
				SortOutlinerTimer = SCENE_OUTLINER_RESORT_TIMER;
			}
		}
		else
		{
			// Sort the list of root items
			SortTree();
		}

		OutlinerTreeView->RequestTreeRefresh();
	}

	void SSceneOutliner::SortTree()
	{
		if (SortByColumn == ColumnID_ActorLabel)
		{
			if (SortMode == EColumnSortMode::Ascending)
			{
				RootTreeItems.Sort(Helpers::FSortByActorAscending());
			}
			else if (SortMode == EColumnSortMode::Descending)
			{
				RootTreeItems.Sort(Helpers::FSortByActorDescending());
			}
		}
		else if (Gutter.IsValid() && SortByColumn == Gutter->GetColumnID())
		{
			Gutter->SortItems(RootTreeItems, SortMode);
		}
		else if (CustomColumn.IsValid())
		{
			CustomColumn->SortItems(RootTreeItems, SortMode);
		}
	}


}	// namespace SceneOutliner

#undef LOCTEXT_NAMESPACE
