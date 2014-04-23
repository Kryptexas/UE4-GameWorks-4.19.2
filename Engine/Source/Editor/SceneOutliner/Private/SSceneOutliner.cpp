// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"
#include "SSceneOutliner.h"
#include "SceneOutlinerTreeItems.h"

#include "ScopedTransaction.h"

#include "ClassIconFinder.h"

#include "SSocketChooser.h"

#include "LevelUtils.h"
#include "ActorEditorUtils.h"
#include "SceneOutlinerFilters.h"

#include "EditorActorFolders.h"

DEFINE_LOG_CATEGORY_STATIC(LogSceneOutliner, Log, All);

#define LOCTEXT_NAMESPACE "SSceneOutliner"

// The amount of time that must past before the Scene Outliner will attempt a sort when in PIE/SIE.
#define SCENE_OUTLINER_RESORT_TIMER 1.0f

namespace SceneOutliner
{

	/** Recursively set the selection of all actor descendants of the specified folder */
	void SelectDescedantActors(TSharedRef<TOutlinerFolderTreeItem> FolderItem, bool bSelected)
	{
		for (const auto& WeakChild : FolderItem->ChildItems)
		{
			auto ChildItem = WeakChild.Pin();
			if (ChildItem.IsValid())
			{
				if (ChildItem->Type == TOutlinerTreeItem::Actor)
				{
					if (AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(ChildItem)->Actor.Get())
					{
						GEditor->SelectActor(Actor, true, /*bNotify=*/false);
					}
				}
				else
				{
					SelectDescedantActors(StaticCastSharedPtr<TOutlinerFolderTreeItem>(ChildItem).ToSharedRef(), bSelected);
				}
			}
		}
	}

	/** Gets the label for a tree item */
	FText GetLabelForItem( const TSharedRef<TOutlinerTreeItem> TreeItem )
	{
		if (TreeItem->Type == SceneOutliner::TOutlinerTreeItem::Actor)
		{
			const AActor* Actor = StaticCastSharedRef<const SceneOutliner::TOutlinerActorTreeItem>(TreeItem)->Actor.Get();
			return Actor ? FText::FromString( Actor->GetActorLabel() ) : LOCTEXT("ActorLabelForMissingActor", "(Deleted Actor)");
		}
		else
		{
			return FText::FromName(StaticCastSharedRef<const SceneOutliner::TOutlinerFolderTreeItem>(TreeItem)->LeafName);
		}
	}

	namespace Helpers
	{
		/** Sorts tree items alphabetically */
		struct FSortByActorAscending
		{
			bool operator()( const FOutlinerTreeItemPtr& A, const FOutlinerTreeItemPtr& B ) const
			{
				if ((A->Type == TOutlinerTreeItem::Folder) != (B->Type == TOutlinerTreeItem::Folder))
				{
					return A->Type == TOutlinerTreeItem::Folder;
				}
				return GetLabelForItem(A.ToSharedRef()).ToString() < GetLabelForItem(B.ToSharedRef()).ToString();
			}
		};
		struct FSortByActorDescending
		{
			bool operator()( const FOutlinerTreeItemPtr& A, const FOutlinerTreeItemPtr& B ) const
			{
				if ((A->Type == TOutlinerTreeItem::Folder) != (B->Type == TOutlinerTreeItem::Folder))
				{
					return A->Type != TOutlinerTreeItem::Folder;
				}
				return GetLabelForItem(A.ToSharedRef()).ToString() > GetLabelForItem(B.ToSharedRef()).ToString();
			}
		};
	}

	void PerformActionOnItem(TSharedPtr<SOutlinerTreeView> OutlinerTreeView, TSharedRef<TOutlinerTreeItem> NewItem, uint8 ActionMask)
	{
		if (ActionMask & NewItemActuator::Select)
		{
			OutlinerTreeView->ClearSelection();
			OutlinerTreeView->SetItemSelection(NewItem, true);
		}

		if (ActionMask & NewItemActuator::Rename)
		{
			NewItem->Flags.RenameWhenInView = true;
		}

		if (ActionMask & (NewItemActuator::ScrollIntoView | NewItemActuator::Rename))
		{
			OutlinerTreeView->RequestScrollIntoView(NewItem);
		}
	}

	void SSceneOutliner::Construct( const FArguments& InArgs, const FSceneOutlinerInitializationOptions& InInitOptions )
	{
		InitOptions = InInitOptions;

		OnItemPicked = InArgs._OnItemPickedDelegate;

		if (InInitOptions.OnSelectionChanged.IsBound())
		{
			SelectionChanged.Add(InInitOptions.OnSelectionChanged);
		}

		bFullRefresh = true;
		bNeedsRefresh = true;
		bIsReentrant = false;
		TotalActorCount = 0;
		FilteredActorCount = 0;
		SortOutlinerTimer = 0.0f;
		bPendingFocusNextFrame = InInitOptions.bFocusSearchBoxWhenOpened;

		SortByColumn = ColumnID_ActorLabel;
		SortMode = EColumnSortMode::Ascending;

		// @todo outliner: Should probably save this in layout!
		// @todo outliner: Should save spacing for list view in layout

		NoBorder = FEditorStyle::GetBrush( "LevelViewport.NoViewportBorder" );
		PlayInEditorBorder = FEditorStyle::GetBrush( "LevelViewport.StartingPlayInEditorBorder" );
		SimulateBorder = FEditorStyle::GetBrush( "LevelViewport.StartingSimulateBorder" );
		MobilityStaticBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityStaticPip" );
		MobilityStationaryBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityStationaryPip" );
		MobilityMovableBrush = FEditorStyle::GetBrush( "ClassIcon.ComponentMobilityMovablePip" );

		//Setup the SearchBox filter
		{
			auto Delegate = TreeItemTextFilter::FItemToStringArray::CreateSP( this, &SSceneOutliner::PopulateSearchStrings );
			SearchBoxFilter = MakeShareable( new TreeItemTextFilter( Delegate ) );
		}

		TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

		//Setup the FilterCollection
		//We use the filter collection provide, otherwise we create our own
		if (InInitOptions.ActorFilters.IsValid())
		{
			CustomFilters = InInitOptions.ActorFilters;
		}
		else
		{
			CustomFilters = MakeShareable( new ActorFilterCollection() );
		}
	
		SearchBoxFilter->OnChanged().AddSP( this, &SSceneOutliner::FullRefresh );
		CustomFilters->OnChanged().AddSP( this, &SSceneOutliner::FullRefresh );

		//Apply custom filters based on global preferences
		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			ApplyShowOnlySelectedFilter( IsShowingOnlySelected() );
			ApplyHideTemporaryActorsFilter( IsHidingTemporaryActors() );
		}


		if (InInitOptions.CustomColumnFactory.IsBound())
		{
			CustomColumn = InInitOptions.CustomColumnFactory.Execute(SharedThis(this));
		}
		else
		{
			CustomColumn = MakeShareable( new FSceneOutlinerActorInfoColumn( SharedThis( this ), ECustomColumnMode::None ) );
		}

		TSharedRef< SHeaderRow > HeaderRowWidget =
			SNew( SHeaderRow )
				// Only show the list header if the user configured the outliner for that
				.Visibility(InInitOptions.bShowHeaderRow ? EVisibility::Visible : EVisibility::Collapsed);

		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			// Set up the gutter if we're in actor browsing mode

			Gutter = MakeShareable(new FSceneOutlinerGutter(FOnSetItemVisibility::CreateSP(this, &SSceneOutliner::OnSetItemVisibility)));
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

		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing || InInitOptions.CustomColumnFactory.IsBound())
		{
			// Customizable actor data column is only viable when browsing OR when it has been bound specifically

			auto CustomColumnHeaderRow = CustomColumn->ConstructHeaderRowColumn();

			CustomColumnHeaderRow
				.SortMode(this, &SSceneOutliner::GetColumnSortMode, CustomColumn->GetColumnID())
				.OnSort(this, &SSceneOutliner::OnColumnSortModeChanged);

			if (InInitOptions.CustomColumnFixedWidth > 0.0f)
			{
				CustomColumnHeaderRow.FixedWidth(InInitOptions.CustomColumnFixedWidth);
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

		auto Toolbar = SNew(SHorizontalBox);

		Toolbar->AddSlot()
		.VAlign(VAlign_Center)
		[
			SAssignNew( FilterTextBoxWidget, SSearchBox )
			.Visibility( this, &SSceneOutliner::GetSearchBoxVisibility )
			.HintText( LOCTEXT( "FilterSearch", "Search..." ) )
			.ToolTipText( LOCTEXT("FilterSearchHint", "Type here to search (pressing enter selects the results)").ToString() )
			.OnTextChanged( this, &SSceneOutliner::OnFilterTextChanged )
			.OnTextCommitted( this, &SSceneOutliner::OnFilterTextCommitted )
		];

		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			Toolbar->AddSlot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(4.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.ToolTipText(LOCTEXT("CreateFolderToolTip", "Create a new folder containing the current actor selection"))
					.OnClicked(this, &SSceneOutliner::OnCreateFolderClicked)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("SceneOutliner.NewFolderIcon"))
					]
				];
		}

		VerticalBox->AddSlot()
		.AutoHeight()
		.Padding( 0.0f, 0.0f, 0.0f, 4.0f )
		[
			Toolbar
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
				SAssignNew( OutlinerTreeView, SOutlinerTreeView, StaticCastSharedRef<SSceneOutliner>(AsShared()) )

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
				.OnContextMenuOpening(this, &SSceneOutliner::OnOpenContextMenu)

				// Header for the tree
				.HeaderRow( HeaderRowWidget )
			]
		];

		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
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
		if (InInitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			// Populate and register to find out when the level's selection changes
			OnLevelSelectionChanged( NULL );
			USelection::SelectionChangedEvent.AddRaw(this, &SSceneOutliner::OnLevelSelectionChanged);
			USelection::SelectObjectEvent.AddRaw(this, &SSceneOutliner::OnLevelSelectionChanged);
		}

		// Register to find out when actors are added or removed
		// @todo outliner: Might not catch some cases (see: CALLBACK_ActorPropertiesChange, CALLBACK_LayerChange, CALLBACK_LevelDirtied, CALLBACK_OnActorMoved, CALLBACK_UpdateLevelsForAllActors)
		FEditorDelegates::MapChange.AddSP( this, &SSceneOutliner::OnMapChange );
		GEngine->OnLevelActorListChanged().AddSP( this, &SSceneOutliner::FullRefresh );
		FWorldDelegates::LevelAddedToWorld.AddSP( this, &SSceneOutliner::OnLevelAdded );
		FWorldDelegates::LevelRemovedFromWorld.AddSP( this, &SSceneOutliner::OnLevelRemoved );

		GEngine->OnLevelActorAdded().AddSP( this, &SSceneOutliner::OnLevelActorsAdded );
		GEngine->OnLevelActorDetached().AddSP( this, &SSceneOutliner::OnLevelActorsDetached );
		GEngine->OnLevelActorFolderChanged().AddSP( this, &SSceneOutliner::OnLevelActorFolderChanged );

		GEngine->OnLevelActorDeleted().AddSP( this, &SSceneOutliner::OnLevelActorsRemoved );
		GEngine->OnLevelActorAttached().AddSP( this, &SSceneOutliner::OnLevelActorsAttached );

		GEngine->OnLevelActorRequestRename().AddSP( this, &SSceneOutliner::OnLevelActorsRequestRename );

		// Register to update when an undo/redo operation has been called to update our list of actors
		GEditor->RegisterForUndo( this );

		// Register to be notified when properties are edited
		FCoreDelegates::OnActorLabelChanged.Add( FCoreDelegates::FOnActorLabelChanged::FDelegate::CreateRaw(this, &SSceneOutliner::OnActorLabelChanged) );

		auto& Folders = FActorFolders::Get();
		Folders.OnFolderCreate.AddSP(this, &SSceneOutliner::OnBroadcastFolderCreate);
		Folders.OnFolderDelete.AddSP(this, &SSceneOutliner::OnBroadcastFolderDelete);
	}

	SSceneOutliner::~SSceneOutliner()
	{
		// We only synchronize selection when in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			USelection::SelectionChangedEvent.RemoveAll(this);
			USelection::SelectObjectEvent.RemoveAll(this);
		}
		FEditorDelegates::MapChange.RemoveAll( this );
		GEngine->OnLevelActorListChanged().RemoveAll( this );
		GEditor->UnregisterForUndo( this );

		SearchBoxFilter->OnChanged().RemoveAll( this );
		CustomFilters->OnChanged().RemoveAll( this );
		
		FWorldDelegates::LevelAddedToWorld.RemoveAll( this );
		FWorldDelegates::LevelRemovedFromWorld.RemoveAll( this );

		FCoreDelegates::OnActorLabelChanged.Remove( FCoreDelegates::FOnActorLabelChanged::FDelegate::CreateRaw(this, &SSceneOutliner::OnActorLabelChanged) );

		auto& Folders = FActorFolders::Get();
		Folders.OnFolderCreate.RemoveAll(this);
		Folders.OnFolderDelete.RemoveAll(this);
	}

	TArray<FOutlinerTreeItemPtr> SSceneOutliner::GetSelectedItems()
	{
		return OutlinerTreeView->GetSelectedItems();
	}

	const TSharedPtr<TOutlinerFolderTreeItem> SSceneOutliner::FindFolderByPath(FName Path) const
	{
		auto* FolderPtr = FolderToTreeItemMap.Find(Path);
		if (FolderPtr)
		{
			return *FolderPtr;
		}
		return nullptr;
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

			EmptyTreeItems();
			UpdateActorMapWithWorld( RepresentingWorld );

			bMadeAnySignificantChanges = true;
			bFullRefresh = false;
		}
		else
		{
			TSet< AActor* > ParentActors;

			// Add actors that have been added to the world
			for(int32 ItemIdx = 0; ItemIdx < AddedItemsList.Num(); ++ItemIdx)
			{
				bMadeAnySignificantChanges |= AddItemToTree(AddedItemsList[ItemIdx], ParentActors);
			}

			// Attach actors to other actors
			for(int32 ItemIdx = 0; ItemIdx < AttachedActorsList.Num(); ++ItemIdx)
			{
				const int32 RootIndex = RootTreeItems.Find(AttachedActorsList[ItemIdx]);

				auto ActorItem = AttachedActorsList[ItemIdx];
				auto* Actor = ActorItem->Actor.Get();
				if (!Actor)
				{
					continue;
				}

				if (RootIndex != INDEX_NONE)
				{
					bMadeAnySignificantChanges = true;

					// Item currently exists at the root of the tree, remove it then enumerate its parents
					RootTreeItems.RemoveAt(RootIndex);


					// Since the item is being removed from the root, whatever parent (or hierarchy of parents) it has needs to be added to the tree (and likely filtered out).
					AActor* Parent = Actor->GetAttachParentActor();
					while(Parent)
					{
						ParentActors.Add(Parent);
						Parent = Parent->GetAttachParentActor();
					}
				}
				else
				{
					// Item currently exists inside a folder - remove this item from our parent, and set the folder name
					TSharedRef<TOutlinerFolderTreeItem>* ParentFolderItem = FolderToTreeItemMap.Find(Actor->GetFolderPath());


					if (ParentFolderItem)
					{
						bMadeAnySignificantChanges = true;
						(*ParentFolderItem)->ChildItems.Remove(AttachedActorsList[ItemIdx]);
					}
				}

				// We only synchronize selection when in actor browsing mode
				if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
				{
					// Synchronize selection
					if( GEditor->GetSelectedActors()->IsSelected( Actor ) )
					{
						OutlinerTreeView->SetItemSelection( AttachedActorsList[ItemIdx], true );
					}
				}
			}

			// Refresh items
			for(int32 ItemIdx = 0; ItemIdx < RefreshItemsList.Num(); ++ItemIdx)
			{
				bMadeAnySignificantChanges = true;

				if (RefreshItemsList[ItemIdx]->Type == TOutlinerTreeItem::Folder)
				{
					// Remove any dead actors from the folder
					auto FolderItem = StaticCastSharedRef<TOutlinerFolderTreeItem>(RefreshItemsList[ItemIdx]);
					for (int32 Index = FolderItem->ChildItems.Num() - 1; Index >= 0; --Index)
					{
						FOutlinerTreeItemPtr Child = FolderItem->ChildItems[Index].Pin();
						if (!Child.IsValid())
						{
							FolderItem->ChildItems.RemoveAt(Index);
						}
						else if (Child->Type == TOutlinerTreeItem::Actor && !StaticCastSharedPtr<TOutlinerActorTreeItem>(Child)->Actor.Get())
						{
							FolderItem->ChildItems.RemoveAt(Index);
						}
					}
				}

				// Refresh items by removing and re-adding them
				RemoveItemFromTree(RefreshItemsList[ItemIdx]);
				AddItemToTree(RefreshItemsList[ItemIdx], ParentActors);
			}

			// Remove items from the tree
			for(int32 ItemIdx = 0; ItemIdx < RemovedItemsList.Num(); ++ItemIdx)
			{
				bMadeAnySignificantChanges = true;
				RemoveItemFromTree(RemovedItemsList[ItemIdx]);
			}

			if( InitOptions.bShowParentTree )
			{
				for( auto ActorIt = ParentActors.CreateConstIterator(); ActorIt; ++ActorIt )
				{
					AActor* Actor = *ActorIt;

					AddFilteredParentActorToTree(Actor);
				}
			}
		}
		
		AddedItemsList.Empty();
		RefreshItemsList.Empty();
		RemovedItemsList.Empty();
		AttachedActorsList.Empty();

		if (bMadeAnySignificantChanges)
		{
			RequestSort();
		}

		NewItemActions.Empty();

		// We're fully refreshed now.
		bNeedsRefresh = false;
	}

	bool SSceneOutliner::ShouldShowFolders() const
	{
		return InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing || InitOptions.bOnlyShowFolders;
	}

	void SSceneOutliner::EmptyTreeItems()
	{
		TotalActorCount = 0;
		FilteredActorCount = 0;

		ActorToTreeItemMap.Reset();
		
		FolderToTreeItemMap.Reset();
		RootTreeItems.Empty();
	}

	void SSceneOutliner::UpdateActorMapWithWorld( UWorld* World )
	{
		if ( World == NULL )
		{
			return;
		}

		EmptyTreeItems();

		TSet< AActor* > ParentActors;

		// We only synchronize selection when in actor browsing mode
		const bool bSyncSelection = ( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing );
		FOutlinerTreeItemPtr LastSelectedItem;

		// Iterate over every actor in memory.  WARNING: This is potentially very expensive!
		for( FActorIterator ActorIt(World); ActorIt; ++ActorIt )
		{
			AActor* Actor = *ActorIt;

			TSharedRef<TOutlinerActorTreeItem> NewItem = MakeShareable( new TOutlinerActorTreeItem(Actor) );

			const bool bAddedToTree = AddActorToTree(NewItem, ParentActors);

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
			// Add any parents for matched actors
			for( auto ActorIt = ParentActors.CreateConstIterator(); ActorIt; ++ActorIt )
			{
				AddFilteredParentActorToTree(*ActorIt);
			}

			if (!IsShowingOnlySelected() && ShouldShowFolders())
			{
				// Add any folders which might match the current search terms
				for (const auto& Path : FActorFolders::Get().GetFoldersForWorld(*World))
				{
					if (!FolderToTreeItemMap.Find(Path))
					{
						TSharedRef<TOutlinerFolderTreeItem> NewFolderItem = MakeShareable(new TOutlinerFolderTreeItem(Path));
						if (!AddFolderToTree(NewFolderItem))
						{
							// If we didn't add that folder, attempt to add its parents too
							TSet<FName> Parents;
							NewFolderItem->GetParentPaths(Parents);
							for (const auto& ParentPath : Parents)
							{
								AddFolderToTree(MakeShareable(new TOutlinerFolderTreeItem(ParentPath)));
							}
						}
					}
				}
			}
		}

		// If we have a selection we may need to scroll to it
		if( bSyncSelection && LastSelectedItem.IsValid() )
		{
			// Scroll last item into view - this means if we are multi-selecting, we show newest selection. @TODO Not perfect though
			OutlinerTreeView->RequestScrollIntoView(LastSelectedItem);
		}
	}

	void SSceneOutliner::RemoveItemFromTree(TSharedRef<TOutlinerTreeItem> InItem)
	{
		TSharedRef<TOutlinerFolderTreeItem>* ParentFolderItem = nullptr;
		if (InItem->Type == TOutlinerTreeItem::Actor)
		{
			auto ActorItemPtr = StaticCastSharedRef<TOutlinerActorTreeItem>(InItem);
			auto* Actor = ActorItemPtr->Actor.Get();
			// We only synchronize selection when in actor browsing mode
			if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				// Synchronize selection since we no longer clear the selection list!
				if( GEditor->GetSelectedActors()->IsSelected( ActorItemPtr->Actor.Get() ) )
				{
					OutlinerTreeView->SetItemSelection( InItem, false );
				}
			}

			if (Actor)
			{
				FName Path = Actor->GetFolderPath();
				if (!Path.IsNone())
				{
					ParentFolderItem = FolderToTreeItemMap.Find(Path);
				}
			}
			ActorToTreeItemMap.Remove(ActorItemPtr->Actor);

			--TotalActorCount;

			if(!InItem->Flags.IsFilteredOut)
			{
				--FilteredActorCount;
			}
		}
		else
		{
			auto FolderItemPtr = StaticCastSharedRef<TOutlinerFolderTreeItem>(InItem);
			ParentFolderItem = FolderToTreeItemMap.Find(FolderItemPtr->GetParentPath());
			FolderToTreeItemMap.Remove(FolderItemPtr->Path);
		}

		if (ParentFolderItem)
		{
			auto& Parent = (*ParentFolderItem);
			Parent->ChildItems.Remove(InItem);
		}
		else
		{
			RootTreeItems.Remove(InItem);
		}
	}

	bool SSceneOutliner::AddItemToTree(TSharedRef<TOutlinerTreeItem> InItem, TSet<AActor*>& ParentActors)
	{
		if (InItem->Type == TOutlinerTreeItem::Actor)
		{
			return AddActorToTree(StaticCastSharedRef<TOutlinerActorTreeItem>(InItem), ParentActors);
		}
		else
		{
			return AddFolderToTree(StaticCastSharedRef<TOutlinerFolderTreeItem>(InItem));
		}
	}

	bool SSceneOutliner::AddActorToTree(TSharedRef<TOutlinerActorTreeItem> InActorItem, TSet< AActor* >& ParentActors)
	{
		if (!InActorItem->Actor.IsValid())
		{
			return false;
		}

		AActor* Actor = InActorItem->Actor.Get();
		if (!IsActorDisplayable(Actor) || !CustomFilters->PassesAllFilters(Actor))
		{
			return false;
		}

		++TotalActorCount;

		// Apply text filter
		if (!SearchBoxFilter->PassesFilter(*InActorItem))
		{
			return false;
		}

		if( InitOptions.bShowParentTree )
		{
			// OK so we know that this actor needs to be visible.  Let's gather any parent actors
			// and make sure they're visible as well, so the user can see the hierarchy.
			AActor* ParentActor = Actor->GetAttachParentActor();
			if( ParentActor != NULL )
			{
				for( auto NextParent = ParentActor; NextParent != NULL; NextParent = NextParent->GetAttachParentActor() )
				{
					ParentActors.Add( NextParent );
				}					
			}
			else
			{
				const FName ActorFolder = Actor->GetFolderPath();
				if (ActorFolder.IsNone() || !ShouldShowFolders())
				{
					// Setup our root-level item list
					RootTreeItems.Add( InActorItem );
				}
				else
				{
					// Ignore search text since we know we pass the filter, and will need to show parents
					const bool bIgnoreSearch = true;
					AddChildToParent(InActorItem, ActorFolder, bIgnoreSearch);
				}
			}
		}
		else
		{
			RootTreeItems.Add( InActorItem );
		}

		// Add the item to the map, making it available for display in the tree when its parent is looking for its children.
		ActorToTreeItemMap.Add(Actor, InActorItem);

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

		NewItemActions.ItemHasBeenCreated(OutlinerTreeView.ToSharedRef(), InActorItem);

		return true;
	}

	bool SSceneOutliner::AddFolderToTree(TSharedRef<TOutlinerFolderTreeItem> InFolderItem, bool bIgnoreSearchFilter)
	{
		if (!InitOptions.bShowParentTree)
		{
			return false;
		}

		if (FolderToTreeItemMap.Find(InFolderItem->Path))
		{
			return false;
		}

		// Apply text filter
		if (!bIgnoreSearchFilter && !SearchBoxFilter->PassesFilter(*InFolderItem))
		{
			return false;
		}

		// Ensure our parent is added first
		FName ParentPath = InFolderItem->GetParentPath();

		if (ParentPath.IsNone())
		{
			RootTreeItems.Add(InFolderItem);
		}
		else
		{
			// Ignore search text since we know we pass the filter, and will need to show parents
			const bool bIgnoreSearch = true;
			AddChildToParent(InFolderItem, ParentPath, bIgnoreSearch);
		}

		// Add the item to the map, making it available for display in the tree when its parent is looking for its children.
		FolderToTreeItemMap.Add(InFolderItem->Path, InFolderItem);

		// Folders are always expanded by default
		OutlinerTreeView->SetItemExpansion(InFolderItem, true);

		NewItemActions.ItemHasBeenCreated(OutlinerTreeView.ToSharedRef(), InFolderItem);

		return true;
	}

	void SSceneOutliner::AddFilteredParentActorToTree(AActor* InActor)
	{
		/* Parents that are filtered out (or otherwise found and added to the list) will not appear in the actor map at this point
		   and should be added as requested. If they have no parent, they will be added to the root of the tree, otherwise they will
		   simply be added to the map, so when displaying the tree, their display item can be found. */
		if( !ActorToTreeItemMap.Contains( InActor ) )
		{
			TSharedRef<TOutlinerActorTreeItem> NewItem = MakeShareable( new TOutlinerActorTreeItem(InActor) );

			ActorToTreeItemMap.Add( InActor, NewItem );

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

			NewItem->Flags.IsFilteredOut = true;
		}


		// Make sure all parent actors are expanded by default
		// @todo Outliner: Really we should be remembering expansion state and trying to restore that
		OutlinerTreeView->SetItemExpansion( ActorToTreeItemMap.FindChecked( InActor ), true );
	}

	void SSceneOutliner::PopulateSearchStrings( const TOutlinerTreeItem& TreeItem, OUT TArray< FString >& OutSearchStrings ) const
	{
		if (TreeItem.Type == TOutlinerTreeItem::Actor)
		{
			AActor* ActorPtr = static_cast<const TOutlinerActorTreeItem&>(TreeItem).Actor.Get();
			if (ActorPtr)
			{
				OutSearchStrings.Add(ActorPtr->GetActorLabel());
				CustomColumn->PopulateActorSearchStrings( ActorPtr, OUT OutSearchStrings );
			}
		}
		else
		{
			OutSearchStrings.Add(static_cast<const TOutlinerFolderTreeItem&>(TreeItem).LeafName.ToString());
			OutSearchStrings.Add(LOCTEXT("FolderClassName", "Folder").ToString());
		}
	}

	EVisibility SSceneOutliner::IsActorClassNameVisible() const
	{
		return CustomColumn->ProvidesSearchStrings() || FilterTextBoxWidget->GetText().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
	}

	FString SSceneOutliner::GetClassNameForItem( FOutlinerTreeItemPtr TreeItem ) const
	{
		FString LabelText;
		const FString& CurrentFilterText = FilterTextBoxWidget->GetText().ToString();

		if( !CustomColumn->ProvidesSearchStrings() && !CurrentFilterText.IsEmpty() )
		{
			// If the user is currently searching, then also display the actor class name.  The class name is
			// searchable, too, and we want to indicate that to the user.
			if (TreeItem->Type == TOutlinerTreeItem::Actor)
			{
				AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem)->Actor.Get();
				if( Actor )
				{
					FString ActorClassName;
					Actor->GetClass()->GetName( ActorClassName );
					LabelText = FString::Printf( TEXT( " (%s)" ), *ActorClassName );
				}
			}
			else
			{
				LabelText = FString::Printf(TEXT(" (%s)"), *LOCTEXT("FolderClassName", "Folder").ToString());
			}
		}

		return LabelText;
	}

	FSlateColor SSceneOutliner::GetColorAndOpacityForItem( FOutlinerTreeItemPtr TreeItem ) const
	{
		FString LabelText;
		bool bDarkenItem = false;
		if (TreeItem->Type == TOutlinerTreeItem::Actor)
		{
			auto ActorTreeItem = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem);
			AActor* Actor = ActorTreeItem->Actor.Get();

			if( Actor == NULL )
			{
				return FLinearColor( 0.2f, 0.2f, 0.25f );		// Deleted actor!
			}

			if ( bRepresentingPlayWorld && !ActorTreeItem->bExistsInCurrentWorldAndPIE )
			{
				// Highlight actors that are exclusive to PlayWorld
				return FLinearColor( 0.12f, 0.56f, 1.0f );
			}

			// Darken items that aren't suitable targets for an active drag and drop actor attachment action
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
		}

		if( !InitOptions.bShowParentTree )
		{
			return FSlateColor::UseForeground();
		}

		// Highlight text differently if it doesn't match the search filter (e.g., parent actors to child actors that
		// match search criteria.)
		if( TreeItem->Flags.IsFilteredOut )
		{
			return FLinearColor( 0.30f, 0.30f, 0.30f );
		}

		return (!bDarkenItem) ? FSlateColor::UseForeground() : FLinearColor( 0.30f, 0.30f, 0.30f );
	}

	const FSlateBrush* SSceneOutliner::GetIconForItem(FOutlinerTreeItemRef Item) const
	{
		if (Item->Type == TOutlinerTreeItem::Actor)
		{
			AActor* Actor = StaticCastSharedRef<TOutlinerActorTreeItem>(Item)->Actor.Get();
			return FClassIconFinder::FindIconForActor(Actor);
		}
		else
		{
			auto Folder = StaticCastSharedRef<TOutlinerFolderTreeItem>(Item);
			if (OutlinerTreeView->IsItemExpanded(Item) && Folder->ChildItems.Num())
			{
				return FEditorStyle::Get().GetBrush(TEXT("SceneOutliner.FolderOpen"));
			}
			else
			{
				return FEditorStyle::Get().GetBrush(TEXT("SceneOutliner.FolderClosed"));
			}
		}
	}

	FText SSceneOutliner::GetToolTipTextForItemIcon( FOutlinerTreeItemPtr TreeItem ) const
	{
		FText ToolTipText;
		if (TreeItem->Type == TOutlinerTreeItem::Actor)
		{
			auto ActorTreeItem = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem);
			AActor* Actor = ActorTreeItem->Actor.Get();
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
		}
		else
		{
			ToolTipText = LOCTEXT("FolderClassName", "Folder");
		}

		return ToolTipText;
	}

	const FSlateBrush* SSceneOutliner::GetBrushForComponentMobilityIcon( FOutlinerTreeItemPtr TreeItem ) const
	{
		const FSlateBrush* IconBrush = MobilityStaticBrush;
		

		if (TreeItem->Type == TOutlinerTreeItem::Folder)
		{
			return IconBrush;
		}

		AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem)->Actor.Get();
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
				if (TreeItem->Type == TOutlinerTreeItem::Actor)
				{
					const auto Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem)->Actor;
					if( Actor != NULL )
					{
						FFormatNamedArguments Args;
						Args.Add(TEXT("ID_Name"), LOCTEXT("CustomColumnMode_InternalName", "ID Name"));
						Args.Add(TEXT("Name"), FText::FromString(Actor->GetName()));
						return FText::Format(LOCTEXT("ActorNameTooltip", "{ID_Name}: {Name}"), Args).ToString();
					}
				}
				else
				{
					FFormatNamedArguments Args;
					Args.Add(TEXT("Name"), FText::FromName(StaticCastSharedPtr<TOutlinerFolderTreeItem>(TreeItem)->LeafName));
					return FText::Format(LOCTEXT("FolderNameTooltip", "Folder: {Name}"), Args).ToString();
				}

				return FString();
			}

		};

		TSharedPtr< SWidget > TableRowContent;
		if( ColumnID == ColumnID_ActorLabel )
		{
			TSharedPtr< SInlineEditableTextBlock > InlineTextBlock;
			TSharedPtr< SHorizontalBox > HBox = SNew( SHorizontalBox );

			const FSlateColor IconColor = InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing ? FLinearColor::White : FEditorStyle::GetSlateColor( "DefaultForeground" );

			auto IconOverlay = SNew(SOverlay)
				+SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew( SImage )
					.Image( this, &SSceneOutliner::GetIconForItem, Item.ToSharedRef() )
					.ToolTipText( this, &SSceneOutliner::GetToolTipTextForItemIcon, Item )
					.ColorAndOpacity( IconColor )
				];

			if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				// Add the component mobility icon
				IconOverlay->AddSlot()
					.HAlign(HAlign_Left)
					[
						SNew( SImage )
						.Image( this, &SSceneOutliner::GetBrushForComponentMobilityIcon, Item )
					];
			}

			HBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding( FMargin(0.0f, 0.0f, 6.0f, 0.0f) )
				[
					SNew(SBox)
					.WidthOverride(18.f)
					[
						IconOverlay
					]
				];

			HBox->AddSlot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding( 0.0f, 2.0f, 0.0f, 2.0f )
				[
					SAssignNew( InlineTextBlock, SInlineEditableTextBlock )

						// Bind a delegate for getting the actor's name.  This ensures that the current actor name is
						// shown accurately, even if it gets renamed while the tree is visible. (Without requiring us
						// to refresh anything.)
 						.Text_Static( &GetLabelForItem, Item.ToSharedRef() )

						// Bind our filter text as the highlight string for the text block, so that when the user
						// starts typing search criteria, this text highlights
						.HighlightText( this, &SSceneOutliner::GetFilterHighlightText )

						// Use the actor's actual FName as the tool-tip text
						.ToolTipText_Static( &Local::GetToolTipTextForActor, Item )

						// Actor names that pass the search criteria are shown in a brighter color
						.ColorAndOpacity( this, &SSceneOutliner::GetColorAndOpacityForItem, Item )

						.OnTextCommitted( this, &SSceneOutliner::OnItemLabelCommitted, Item )

						.OnVerifyTextChanged( this, &SSceneOutliner::OnVerifyItemLabelChanged, Item )

						.IsSelected( InIsSelected )
				];

			HBox->AddSlot()
				.AutoWidth()
				.Padding( 0.0f, 3.0f, 3.0f, 0.0f )
				[
					SNew( STextBlock )

						// Bind a delegate for getting the actor's class name
 						.Text( this, &SSceneOutliner::GetClassNameForItem, Item )

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
			TableRowContent = Gutter->ConstructRowWidget( Item.ToSharedRef() );
		}
		else if( ensure( ColumnID == CustomColumn->GetColumnID() ) )
		{
			TableRowContent = CustomColumn->ConstructRowWidget( Item.ToSharedRef() );
		}

		return TableRowContent.ToSharedRef();
	}

	void SSceneOutliner::OnItemLabelCommitted( const FText& InLabel, ETextCommit::Type InCommitInfo, FOutlinerTreeItemPtr TreeItem )
	{
		if (TreeItem->Type == TOutlinerTreeItem::Actor)
		{
			auto ActorTreeItem = StaticCastSharedPtr<TOutlinerActorTreeItem>(TreeItem);
			auto* Actor = ActorTreeItem->Actor.Get();

			if (Actor && InLabel.ToString() != Actor->GetActorLabel() && Actor->IsActorLabelEditable())
			{
				const FScopedTransaction Transaction( LOCTEXT( "SceneOutlinerRenameActorTransaction", "Rename Actor" ) );
				Actor->SetActorLabel( InLabel.ToString() );

				// Set the keyboard focus back to the SceneOutliner, so the user can perform keyboard commands
				FWidgetPath SceneOutlinerWidgetPath;
				FSlateApplication::Get().GeneratePathToWidgetUnchecked( SharedThis( this ), SceneOutlinerWidgetPath );
				FSlateApplication::Get().SetKeyboardFocus( SceneOutlinerWidgetPath, EKeyboardFocusCause::SetDirectly );
			}
		}
		else
		{
			auto FolderTreeItem = StaticCastSharedPtr<TOutlinerFolderTreeItem>(TreeItem);
			if (!InLabel.EqualTo(FText::FromName(FolderTreeItem->LeafName)))
			{
				// Rename the item
				FName NewPath = FolderTreeItem->GetParentPath();
				if (NewPath.IsNone())
				{
					NewPath = FName(*InLabel.ToString());
				}
				else
				{
					NewPath = FName(*(NewPath.ToString() / InLabel.ToString()));
				}

				if (FActorFolders::Get().RenameFolderInWorld(*RepresentingWorld, FolderTreeItem->Path, NewPath))
				{
					NewItemActions.WhenCreated(NewPath, NewItemActuator::Select | NewItemActuator::ScrollIntoView);
				}
			}
		}
		
	}

	bool SSceneOutliner::OnVerifyItemLabelChanged( const FText& InLabel, FText& OutErrorMessage, FOutlinerTreeItemPtr TreeItem )
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
			OutErrorMessage = FText::Format(LOCTEXT("RenameFailed_TooLong", "Names must be less than {CharCount} characters long."), Arguments);
			return false;
		}

		FString LabelString = InLabel.ToString();
		if (TreeItem->Type == TOutlinerTreeItem::Folder)
		{
			auto FolderTreeItem = StaticCastSharedPtr<TOutlinerFolderTreeItem>(TreeItem);
			if (FolderTreeItem->LeafName == FName(*LabelString))
			{
				return true;
			}

			int32 Dummy = 0;
			if (LabelString.FindChar('/', Dummy) || LabelString.FindChar('\\', Dummy))
			{
				OutErrorMessage = LOCTEXT("RenameFailed_InvalidChar", "Folder names cannot contain / or \\.");
				return false;
			}

			// Validate that this folder doesn't exist already
			FName NewPath = FolderTreeItem->GetParentPath();
			if (NewPath.IsNone())
			{
				NewPath = FName(*LabelString);
			}
			else
			{
				NewPath = FName(*(NewPath.ToString() / LabelString));
			}

			if (FolderToTreeItemMap.Find(NewPath))
			{
				OutErrorMessage = LOCTEXT("RenameFailed_AlreadyExists", "This folder already exists");
				return false;
			}
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

	TArray<TSharedRef<TOutlinerFolderTreeItem>> SSceneOutliner::GetSelectedFolders() const
	{
		TArray<TSharedRef<TOutlinerFolderTreeItem>> Folders;
		for (auto& Item : OutlinerTreeView->GetSelectedItems())
		{
			if (Item->Type == TOutlinerTreeItem::Folder)
			{
				Folders.Add(StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item).ToSharedRef());
			}
		}
		return Folders;
	}

	TSharedPtr<SWidget> SSceneOutliner::OnOpenContextMenu() const
	{
		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>( SelectedActors );

		if (SelectedActors.Num() && InitOptions.ContextMenuOverride.IsBound())
		{
			return InitOptions.ContextMenuOverride.Execute();
		}

		if (InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing)
		{
			return BuildDefaultContextMenu();
		}

		return TSharedPtr<SWidget>();
	}

	TSharedPtr<SWidget> SSceneOutliner::BuildDefaultContextMenu() const
	{
		// Build up the menu
		const bool bCloseAfterSelection = true;
		FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>(), InitOptions.DefaultMenuExtender);

		auto SelectedItems = OutlinerTreeView->GetSelectedItems();
		auto SelectedFolders = GetSelectedFolders();

		TSharedPtr<TOutlinerFolderTreeItem> ParentFolder;
		if (SelectedFolders.Num() == 1)
		{
			ParentFolder = SelectedFolders[0];
		}

		const FSlateIcon NewFolderIcon(FEditorStyle::GetStyleSetName(), "SceneOutliner.NewFolderIcon");
		MenuBuilder.BeginSection("FolderSection", LOCTEXT("ThisFolderSection", "This Folder"));
		{
			if (ParentFolder.IsValid())
			{
				MenuBuilder.AddMenuEntry(LOCTEXT("CreateSubFolder", "Create Sub Folder"), FText(), NewFolderIcon, FUIAction(FExecuteAction::CreateSP(this, &SSceneOutliner::CreateSubFolder, ParentFolder->Path)));
				MenuBuilder.AddMenuEntry(LOCTEXT("RenameFolder", "Rename Folder"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SSceneOutliner::RenameFolder, ParentFolder.ToSharedRef())));
				MenuBuilder.AddMenuEntry(LOCTEXT("DeleteFolder", "Delete Folder"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SSceneOutliner::DeleteFolder, ParentFolder.ToSharedRef())));
			}
		}
		MenuBuilder.EndSection();

		if (SelectedItems.Num() == 0)
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("CreateFolder", "Create Folder"), FText(), NewFolderIcon, FUIAction(FExecuteAction::CreateSP(this, &SSceneOutliner::CreateFolder)));
		}
		else
		{
			MenuBuilder.BeginSection("SelectionSection", LOCTEXT("OutlinerSelection", "Current Outliner Selection"));
			{
				MenuBuilder.AddSubMenu(
					LOCTEXT("MoveActorsToFolder", "Move To Folder"),
					LOCTEXT("MoveActorsToFolder_Tooltip", "Move selection to a folder"),
					FNewMenuDelegate::CreateSP(this,  &SSceneOutliner::FillFoldersSubMenu));

				// If we've only got folders selected, show the selection sub menu
				if (SelectedFolders.Num() == SelectedItems.Num())
				{
					MenuBuilder.AddSubMenu(
						LOCTEXT("SelectSubmenu", "Select"),
						LOCTEXT("SelectSubmenu_Tooltip", "Select the contents of the current selection"),
						FNewMenuDelegate::CreateSP(this, &SSceneOutliner::FillSelectionSubMenu));
				}
			}
		}

		return MenuBuilder.MakeWidget();
	}

	void SSceneOutliner::FillFoldersSubMenu(FMenuBuilder& MenuBuilder) const
	{
		// Build up the menu
		FSceneOutlinerInitializationOptions InitOptions;
		InitOptions.bShowHeaderRow = false;
		InitOptions.bFocusSearchBoxWhenOpened = true;
		InitOptions.bOnlyShowFolders = true;

		// Actor selector to allow the user to choose a folder
		FSceneOutlinerModule& SceneOutlinerModule = FModuleManager::LoadModuleChecked<FSceneOutlinerModule>( "SceneOutliner" );
		TSharedRef< SWidget > MiniSceneOutliner =
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.MaxHeight(400.0f)
			[
				SNew( SceneOutliner::SSceneOutliner, InitOptions )
				.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
				.OnItemPickedDelegate(FOnSceneOutlinerItemPicked::CreateSP(this, &SSceneOutliner::MoveSelectionTo))
			];

		MenuBuilder.AddMenuEntry(LOCTEXT( "CreateNew", "Create New" ), LOCTEXT( "MoveToRoot_ToolTip", "Move to a new folder" ),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "SceneOutliner.NewFolderIcon"), FExecuteAction::CreateSP(this, &SSceneOutliner::CreateFolder));

		if (OutlinerTreeView->ValidateMoveSelectionTo(FName()))
		{
			MenuBuilder.AddMenuEntry(LOCTEXT( "MoveToRoot", "Move To Root" ), LOCTEXT( "MoveToRoot_ToolTip", "Move to the root of the scene outliner" ),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "SceneOutliner.MoveToRoot"), FExecuteAction::CreateSP(this, &SSceneOutliner::MoveSelectionTo, FName()));
		}

		// Existing folders - it would be good to filter out the existing selected folders from this mini-outliner, but that's currently not possible
		MenuBuilder.BeginSection(FName(), LOCTEXT("ExistingFolders", "Existing:"));
		MenuBuilder.AddWidget(MiniSceneOutliner, FText::GetEmpty(), false);
		MenuBuilder.EndSection();
	}

	void SSceneOutliner::FillSelectionSubMenu(FMenuBuilder& MenuBuilder) const
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "AddChildrenToSelection", "Immediate Children" ),
			LOCTEXT( "AddChildrenToSelection_ToolTip", "Select all immediate children of the selected folders" ),
			FSlateIcon(),
			FExecuteAction::CreateSP(this, &SSceneOutliner::SelectFoldersImmediateChildren));
		MenuBuilder.AddMenuEntry(
			LOCTEXT( "AddDescendantsToSelection", "All Descendants" ),
			LOCTEXT( "AddDescendantsToSelection_ToolTip", "Select all descendants of the selected folders" ),
			FSlateIcon(),
			FExecuteAction::CreateSP(this, &SSceneOutliner::SelectFoldersDescendants));
	}

	void SSceneOutliner::SelectFoldersImmediateChildren() const
	{
		auto SelectedFolders = GetSelectedFolders();
		if (SelectedFolders.Num())
		{
			// We'll batch selection changes instead by using BeginBatchSelectOperation()
			GEditor->GetSelectedActors()->BeginBatchSelectOperation();

			for (const auto& Folder : SelectedFolders)
			{
				for (const auto& WeakChild : Folder->ChildItems)
				{
					auto ChildItem = WeakChild.Pin();
					if (ChildItem.IsValid() && ChildItem->Type == TOutlinerTreeItem::Actor)
					{
						if (AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(ChildItem)->Actor.Get())
						{
							GEditor->SelectActor(Actor, true, /*bNotify=*/false);
						}
					}
				}
			}

			GEditor->GetSelectedActors()->EndBatchSelectOperation();
			GEditor->NoteSelectionChange();
		}
	}

	void SSceneOutliner::SelectFoldersDescendants() const
	{
		auto SelectedFolders = GetSelectedFolders();
		if (SelectedFolders.Num())
		{
			// We'll batch selection changes instead by using BeginBatchSelectOperation()
			GEditor->GetSelectedActors()->BeginBatchSelectOperation();

			for (const auto& Folder : SelectedFolders)
			{
				SelectDescedantActors(Folder, true);
			}

			GEditor->GetSelectedActors()->EndBatchSelectOperation();
			GEditor->NoteSelectionChange();
		}
	}

	FName SSceneOutliner::MoveFolderTo(TSharedRef<TOutlinerFolderTreeItem> Folder, FName NewParent)
	{
		FName NewPath;
		if (NewParent.IsNone())
		{
			NewPath = Folder->LeafName;
		}
		else
		{
			NewPath = FName(*(NewParent.ToString() / Folder->LeafName.ToString()));
		}

		if (FActorFolders::Get().RenameFolderInWorld(*RepresentingWorld, Folder->Path, NewPath))
		{
			return NewPath;
		}

		return FName();
	}

	void SSceneOutliner::MoveSelectionTo(TSharedRef<TOutlinerTreeItem> NewParent)
	{
		if (NewParent->Type == TOutlinerTreeItem::Folder)
		{
			MoveSelectionTo(StaticCastSharedRef<TOutlinerFolderTreeItem>(NewParent)->Path);
		}
	}

	void SSceneOutliner::MoveSelectionTo(FName NewParent)
	{
		FSlateApplication::Get().DismissAllMenus();

		if (!OutlinerTreeView->ValidateMoveSelectionTo(NewParent))
		{
			return;
		}

		auto SelectedItems = GetSelectedItems();

		const FScopedTransaction Transaction( LOCTEXT("MoveOutlinerItems", "Move Scene Outliner Items") );
		for (const auto& Item : SelectedItems)
		{
			if (Item->Type == TOutlinerTreeItem::Folder)
			{
				auto Folder = StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item).ToSharedRef();
				auto NewPath = MoveFolderTo(Folder, NewParent);
				if (!NewPath.IsNone() && SelectedItems.Num() == 1)
				{
					NewItemActions.WhenCreated(NewPath, NewItemActuator::Select | NewItemActuator::ScrollIntoView);
				}
			}
			else
			{
				auto ActorItem = StaticCastSharedPtr<TOutlinerActorTreeItem>(Item);
				auto* Actor = ActorItem->Actor.Get();
				if (Actor)
				{
					Actor->SetFolderPath(NewParent);

					if (SelectedItems.Num() == 1)
					{
						NewItemActions.WhenCreated(ActorItem.ToSharedRef(), NewItemActuator::Select | NewItemActuator::ScrollIntoView);
					}
				}
			}
		}
	}

	bool SSceneOutliner::AddChildToParent(FOutlinerTreeItemRef TreeItem, FName ParentPath, bool bIgnoreSearchFilter)
	{
		TSharedRef<TOutlinerFolderTreeItem>* ExistingParent = FolderToTreeItemMap.Find(ParentPath);
		if (ExistingParent)
		{
			(*ExistingParent)->ChildItems.Add(TreeItem);
			OutlinerTreeView->SetItemExpansion(*ExistingParent, true);
			return true;
		}
		else
		{
			TSharedRef<TOutlinerFolderTreeItem> NewFolderItem = MakeShareable(new TOutlinerFolderTreeItem(ParentPath));
			if (AddFolderToTree(NewFolderItem, bIgnoreSearchFilter))
			{
				NewFolderItem->ChildItems.Add(TreeItem);
				OutlinerTreeView->SetItemExpansion(NewFolderItem, true);
				return true;
			}
		}

		return false;
	}

	FReply SSceneOutliner::OnCreateFolderClicked()
	{
		if (!RepresentingWorld)
		{
			return FReply::Unhandled();
		}

		CreateFolder();

		return FReply::Handled();
	}

	void SSceneOutliner::CreateFolder()
	{
		if (!RepresentingWorld)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("UndoAction_CreateFolder", "Create Folder"));

		const FName NewFolderName = FActorFolders::Get().GetDefaultFolderNameForSelection(*RepresentingWorld);
		FActorFolders::Get().CreateFolderContainingSelection(*RepresentingWorld, NewFolderName);

		// Clear the current actor selection.
		GEditor->SelectNone(false, true, true);

		// Move any selected folders into the new folder name
		for (auto& Item : GetSelectedItems())
		{
			if (Item->Type == TOutlinerTreeItem::Folder)
			{
				auto Folder = StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item).ToSharedRef();
				MoveFolderTo(Folder, NewFolderName);
			}
		}

		// At this point the new folder will be in our newly added list, so select it and open a rename when it gets refreshed
		NewItemActions.WhenCreated(NewFolderName, NewItemActuator::Select | NewItemActuator::Rename);
	}
	
	void SSceneOutliner::CreateSubFolder(FName Parent)
	{
		if (!RepresentingWorld)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("UndoAction_CreateFolder", "Create Folder"));

		const FName NewFolderName = FActorFolders::Get().GetDefaultFolderName(*RepresentingWorld, Parent);
		FActorFolders::Get().CreateFolder(*RepresentingWorld, NewFolderName);

		// At this point the new folder will be in our newly added list, so select it and open a rename when it gets refreshed
		NewItemActions.WhenCreated(NewFolderName, NewItemActuator::Select | NewItemActuator::Rename);
	}

	void SSceneOutliner::OnBroadcastFolderCreate(const UWorld& InWorld, FName NewPath)
	{
		if (!ShouldShowFolders() || &InWorld != RepresentingWorld)
		{
			return;
		}

		if (!FolderToTreeItemMap.Contains(NewPath))
		{
			TSharedRef<TOutlinerFolderTreeItem> NewFolderItem = MakeShareable(new TOutlinerFolderTreeItem(NewPath));
			AddedItemsList.Add(NewFolderItem);

			// Move the currently selected actors into the new folder
			for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt; ++SelectionIt )
			{
				AActor* Actor = CastChecked<AActor>(*SelectionIt);
				Actor->SetFolderPath(NewPath);
			}

			Refresh();
		}
	}

	void SSceneOutliner::DetachChildFromParent(TSharedRef<TOutlinerTreeItem> Child, FName ParentPath)
	{
		if (ParentPath.IsNone())
		{
			RootTreeItems.Remove(Child);
		}
		else
		{
			auto* ParentFolder = FolderToTreeItemMap.Find(ParentPath);
			if (ParentFolder)
			{
				ParentFolder->Get().ChildItems.Remove(Child);
			}
		}
	}

	void SSceneOutliner::OnBroadcastFolderDelete(const UWorld& InWorld, FName Path)
	{
		if (&InWorld != RepresentingWorld)
		{
			return;
		}

		auto* Folder = FolderToTreeItemMap.Find(Path);
		if (Folder)
		{
			RemovedItemsList.Add(*Folder);
			Refresh();
		}
	}

	void SSceneOutliner::RenameFolder(TSharedRef<TOutlinerFolderTreeItem> Folder)
	{
		Folder->RenameRequestEvent.ExecuteIfBound();
	}

	void SSceneOutliner::DeleteFolder(TSharedRef<TOutlinerFolderTreeItem> Folder)
	{
		const FScopedTransaction Transaction( LOCTEXT("DeleteFolder", "Delete Folder") );

		auto Children = Folder->ChildItems;
		for (auto& WeakChild : Children)
		{
			auto Item = WeakChild.Pin();
			if (!Item.IsValid())
			{
				continue;
			}

			if (Item->Type == TOutlinerTreeItem::Actor)
			{
				AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(Item)->Actor.Get();
				if (Actor)
				{
					Actor->SetFolderPath(FName());
				}
			}
			else
			{
				DeleteFolder(StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item).ToSharedRef());
			}
		}

		if (RepresentingWorld)
		{
			FActorFolders::Get().DeleteFolder(*RepresentingWorld, Folder->Path);
		}

		Refresh();
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

		if (InParent->Type == TOutlinerTreeItem::Actor)
		{
			auto Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(InParent)->Actor;
			AActor* ParentActor = Actor.Get();
			if( !ParentActor )
			{
				return;
			}
			// Does this actor have anything attached?
			static TArray< AActor* > AttachedActors;
			AttachedActors.Reset();
			// @todo outliner: Perf: This call always will allocate heap memory and do a bit of work, even for actors with no children!
			ParentActor->GetAttachedActors( AttachedActors );

			if( AttachedActors.Num() == 0 )
			{
				return;
			}

			// Walk the list of attached actors and add their tree nodes to the output array
			for( TArray< AActor* >::TConstIterator AttachedActorIt( AttachedActors ); AttachedActorIt; ++AttachedActorIt )
			{
				AActor* ChildActor = *AttachedActorIt;
				if( !ensure( ChildActor != NULL ) )
				{
					continue;
				}

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
					auto* ChildTreeItem = ActorToTreeItemMap.Find( ChildActor );

					// There's a chance we won't actually have a tree item for the child actor (e.g. because
					// Outliner wasn't notified when an actor was destroyed or reparented), so we handle that here.
					// In any other case though, we should always have an item for the actor in our list
					if( ChildTreeItem )
					{
						OutChildren.Add( *ChildTreeItem );
					}
				}
			}
		}
		else
		{
			// Folders
			auto& ChildItems = StaticCastSharedPtr<TOutlinerFolderTreeItem>(InParent)->ChildItems;
			for (auto ChildIt = ChildItems.CreateIterator(); ChildIt; ++ChildIt)
			{
				auto Child = ChildIt->Pin();
				if (Child.IsValid())
				{
					OutChildren.Add( Child );
				}
			}

		}

		if (OutChildren.Num())
		{
			if (SortByColumn == ColumnID_ActorLabel)
			{
				if (SortMode == EColumnSortMode::Ascending)
				{
					OutChildren.Sort(Helpers::FSortByActorAscending());
				}
				else if (SortMode == EColumnSortMode::Descending)
				{
					OutChildren.Sort(Helpers::FSortByActorDescending());
				}
			}
			else if (Gutter.IsValid() && SortByColumn == Gutter->GetColumnID())
			{
				Gutter->SortItems(OutChildren, SortMode);
			}
			else if (CustomColumn.IsValid())
			{
				CustomColumn->SortItems(OutChildren, SortMode);
			}
		}
	}


	bool SSceneOutliner::IsActorDisplayable( const AActor* Actor ) const
	{
		return (
			!InitOptions.bOnlyShowFolders && 
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
				auto FirstItem = SelectedItems[0];
				if (FirstItem->Type == TOutlinerTreeItem::Actor)
				{
					AActor* FirstSelectedActor = StaticCastSharedPtr<TOutlinerActorTreeItem>(FirstItem)->Actor.Get();
					if( FirstSelectedActor != NULL )				
					{
						// Only fire the callback if this actor actually passes our required filters.  It's possible that
						// some actors displayed are parent actors which don't actually match the requested criteria.
						if( IsActorDisplayable( FirstSelectedActor ) && 
							CustomFilters->PassesAllFilters( FirstSelectedActor ) &&
							SearchBoxFilter->PassesFilter( *FirstItem ) )
						{
							// Signal that an actor was selected
							OnItemPicked.ExecuteIfBound( FirstItem.ToSharedRef() );
						}
					}
				}
				else
				{
					// A folder was picked
					OnItemPicked.ExecuteIfBound( FirstItem.ToSharedRef() );
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
				TArray<FOutlinerTreeItemRef> FoldersToSelect;
				const FOutlinerData SelectedItems = OutlinerTreeView->GetSelectedItems();
				for( FOutlinerData::TConstIterator SelectedItemIt( SelectedItems ); SelectedItemIt; ++SelectedItemIt )
				{
					FOutlinerTreeItemPtr SelectedTreeItem = *SelectedItemIt;
					if (SelectedTreeItem->Type == TOutlinerTreeItem::Actor)
					{
						AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(SelectedTreeItem)->Actor.Get();
						if( Actor != NULL )
						{
							ActorsToSelect.Add( Actor );
						}
					}
					else
					{
						FoldersToSelect.Add(SelectedTreeItem.ToSharedRef());
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
					auto* SelectedTreeItem = ActorToTreeItemMap.Find( Actor );
					if (SelectedTreeItem)
					{
						OutlinerTreeView->SetItemSelection(*SelectedTreeItem, true);
					}
				}
				for (auto FolderIt = FoldersToSelect.CreateConstIterator(); FolderIt; ++FolderIt)
				{
					OutlinerTreeView->SetItemSelection(*FolderIt, true);
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
						auto* TreeItem = ActorToTreeItemMap.Find( Actor );
						if( TreeItem )
						{
							OutlinerTreeView->SetItemSelection( *TreeItem, true );

							LastSelectedItem = *TreeItem;
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
				FOutlinerTreeItemPtr SelectedTreeItem = *SelectedItemIt;
				if (SelectedTreeItem->Type == TOutlinerTreeItem::Actor)
				{
					AActor* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(SelectedTreeItem)->Actor.Get();
					if( Actor != NULL )
					{
						SelectedActors.Add( Actor );
					}
				}
				else
				{
					OutlinerTreeView->SetItemExpansion(SelectedTreeItem, !OutlinerTreeView->IsItemExpanded(SelectedTreeItem));
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
		auto FolderItem = StaticCastSharedPtr<TOutlinerFolderTreeItem>(TreeItem);
		if (TreeItem->Flags.RenameWhenInView)
		{
			TreeItem->Flags.RenameWhenInView = false;
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
					FOutlinerTreeItemRef NewItem = MakeShareable( new TOutlinerActorTreeItem((AActor*)InActor) );
					AddedItemsList.AddUnique(NewItem);
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
				auto* ItemPtr = ActorToTreeItemMap.Find(InActor);
				if(ItemPtr)
				{
					RemovedItemsList.AddUnique(*ItemPtr);

					// Refresh the parent folder as well if possible
					const FName Path = InActor->GetFolderPath();
					auto* FolderItem = FolderToTreeItemMap.Find(Path);
					if (FolderItem)
					{
						RefreshItemsList.AddUnique(*FolderItem);
					}
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
				auto* ItemPtr = ActorToTreeItemMap.Find(InActor);
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
				auto* ItemPtr = ActorToTreeItemMap.Find(InActor);

				if(ItemPtr)
				{
					RefreshItemsList.AddUnique(*ItemPtr);
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
					auto* ItemPtr = ActorToTreeItemMap.Find(InParent);

					if(ItemPtr)
					{
						RefreshItemsList.AddUnique(*ItemPtr);
						Refresh();
					}
				}
			}
		}
	}

	/** Called by the engine when an actor's folder is changed */
	void SSceneOutliner::OnLevelActorFolderChanged(const AActor* InActor, FName OldPath)
	{
		auto* ActorTreeItem = ActorToTreeItemMap.Find(InActor);
		if (!ShouldShowFolders() || !InActor || !ActorTreeItem)
		{
			return;
		}
		
		// Detach the item from its old parent while we know the old path
		DetachChildFromParent(*ActorTreeItem, OldPath);

		// Refresh the tree item
		RefreshItemsList.AddUnique(*ActorTreeItem);
		Refresh();
	}

	void SSceneOutliner::OnLevelActorsRequestRename(const AActor* InActor)
	{
		TArray< TSharedPtr < TOutlinerTreeItem > > SelectedItems = OutlinerTreeView->GetSelectedItems();
		if( SelectedItems.Num() > 0)
		{
			// Ensure that the item we want to rename is visible in the tree
			TSharedPtr<TOutlinerTreeItem> ItemToRename = SelectedItems[SelectedItems.Num() - 1];
			ItemToRename->Flags.RenameWhenInView = true;
			OutlinerTreeView->RequestScrollIntoView(ItemToRename);
		}
	}

	void SSceneOutliner::OnMapChange(uint32 MapFlags)
	{
		FullRefresh();
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
		
		auto* TreeItem = ActorToTreeItemMap.Find( ChangedActor );
		if (TreeItem)
		{
			OutlinerTreeView->FlashHighlightOnItem(*TreeItem);
		}
	}

	void SSceneOutliner::OnSetItemVisibility(FOutlinerTreeItemRef Item, bool bIsVisible)
	{
		// We operate on all the selected items if the specified item is selected
		if (OutlinerTreeView->IsItemSelected(Item))
		{
			for (auto& TreeItem : OutlinerTreeView->GetSelectedItems())
			{
				TreeItem->SetIsVisible(bIsVisible);
			}
		}
		else
		{
			Item->SetIsVisible(bIsVisible);
		}

		GEditor->RedrawAllViewports();
	}

	void SSceneOutliner::OnFilterTextChanged( const FText& InFilterText )
	{
		SearchBoxFilter->SetRawFilterText( InFilterText );
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
							if( CustomFilters->PassesAllFilters( Actor ) && SearchBoxFilter->PassesFilter( *ActorIt.Value() ) )
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
						auto* ActorItem = ActorToTreeItemMap.Find(ActorsToSelect[0]);
						if( ActorItem != NULL )				
						{
							// Signal that an actor was selected. We assume it is valid as it won't have been added to ActorsToSelect if not.
							OnItemPicked.ExecuteIfBound( *ActorItem );
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
		return SearchBoxFilter->GetRawFilterText();
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

		// We only allow these operations in actor browsing mode
		if( InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			// Delete key: Delete selected actors (not rebindable, because it doesn't make much sense to bind.)
			if( InKeyboardEvent.GetKey() == EKeys::F2 )
			{
				auto SelectedItems = OutlinerTreeView->GetSelectedItems();
				if (SelectedItems.Num() == 1 && SelectedItems[0]->Type == TOutlinerTreeItem::Folder)
				{
					SelectedItems[0]->RenameRequestEvent.ExecuteIfBound();
					return FReply::Handled();
				}
			}
			else if ( InKeyboardEvent.GetKey() == EKeys::Platform_Delete )
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
					const FScopedTransaction Transaction( LOCTEXT("UndoAction_DeleteSelection", "Delete selection") );
//					FActorFolders::Get().Modify(*GWorld);

					// Delete selected folders too
					auto SelectedItems = OutlinerTreeView->GetSelectedItems();
					
					GEditor->SelectNone(true, true);
					for (auto ItemIt = SelectedItems.CreateIterator(); ItemIt; ++ItemIt)
					{
						auto Item = (*ItemIt);
						if (Item->Type == TOutlinerTreeItem::Folder)
						{
							DeleteFolder(StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item).ToSharedRef());
						}
						else
						{
							auto* Actor = StaticCastSharedPtr<TOutlinerActorTreeItem>(Item)->Actor.Get();
							if (Actor)
							{
								GEditor->SelectActor(Actor, true, true);
							}
						}
					}

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
		if ( SortByColumn != ColumnId || (SortByColumn == CustomColumn->GetColumnID() && !CustomColumn->SupportsSorting()) )
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
	
	void NewItemActuator::Empty()
	{
		PendingFolderActions.Empty();
		PendingActorActions.Empty();
	}

	void NewItemActuator::WhenCreated(FName FolderPath, uint8 InActionMask)
	{
		const FolderAction NewAction = { InActionMask, FolderPath };
		PendingFolderActions.Add(NewAction);
	}

	void NewItemActuator::WhenCreated(FOutlinerTreeItemRef Item, uint8 InActionMask)
	{
		if (Item->Type == TOutlinerTreeItem::Actor)
		{
			const ActorAction NewAction = { InActionMask, StaticCastSharedRef<TOutlinerActorTreeItem>(Item) };
			PendingActorActions.Add(NewAction);
		}
		else
		{
			WhenCreated(StaticCastSharedRef<TOutlinerFolderTreeItem>(Item)->Path, InActionMask);
		}
	}

	void NewItemActuator::ItemHasBeenCreated(TSharedRef<SOutlinerTreeView> OutlinerTreeView, TSharedRef<TOutlinerActorTreeItem> NewItem)
	{
		for (int32 Index = 0; Index < PendingActorActions.Num(); )
		{
			auto PinnedPendingItem = PendingActorActions[Index].ActorItem.Pin();
			if (!PinnedPendingItem.IsValid())
			{
				PendingActorActions.RemoveAt(Index);
			}
			else if(PinnedPendingItem == NewItem)
			{
				PerformActionOnItem(OutlinerTreeView, NewItem, PendingActorActions[Index].Actions);

				PendingActorActions.RemoveAt(Index);
			}
			else
			{
				++Index;
			}
		}
	}

	void NewItemActuator::ItemHasBeenCreated(TSharedRef<SOutlinerTreeView> OutlinerTreeView, TSharedRef<TOutlinerFolderTreeItem> NewItem)
	{
		for (int32 Index = 0; Index < PendingFolderActions.Num(); )
		{
			if (PendingFolderActions[Index].FolderPath == NewItem->Path)
			{
				PerformActionOnItem(OutlinerTreeView, NewItem, PendingFolderActions[Index].Actions);
				PendingFolderActions.RemoveAt(Index);
			}
			else
			{
				++Index;
			}
		}
	}

}	// namespace SceneOutliner

#undef LOCTEXT_NAMESPACE
