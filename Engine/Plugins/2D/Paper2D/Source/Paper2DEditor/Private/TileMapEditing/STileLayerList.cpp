// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "STileLayerList.h"
#include "STileLayerItem.h"

#include "ScopedTransaction.h"

#include "TileMapEditorCommands.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// STileLayerList

void STileLayerList::Construct(const FArguments& InArgs, UPaperTileMap* TileMap)
{
	TSharedRef<SHeaderRow> HeaderRowWidget = SNew(SHeaderRow)
		+SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("TileLayerList", "TileLayerNameHeader", "Layer Name")).FillWidth(0.7);
		//+SHeaderRow::Column("TileSet").DefaultLabel(NSLOCTEXT("TileLayerList", "TileLayerTileSetHeader", "Tile Set")).FillWidth(0.7);

	TileMapPtr = TileMap;

	FTileMapEditorCommands::Register();
	const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		Commands.AddNewLayerAbove,
		FExecuteAction::CreateSP(this, &STileLayerList::AddNewLayerAbove));

	CommandList->MapAction(
		Commands.AddNewLayerBelow,
		FExecuteAction::CreateSP(this, &STileLayerList::AddNewLayerBelow));

	CommandList->MapAction(
		Commands.DeleteLayer,
		FExecuteAction::CreateSP(this, &STileLayerList::DeleteLayer));

	CommandList->MapAction(
		Commands.DuplicateLayer,
		FExecuteAction::CreateSP(this, &STileLayerList::DuplicateLayer));

	CommandList->MapAction(
		Commands.MergeLayerDown,
		FExecuteAction::CreateSP(this, &STileLayerList::MergeLayerDown));

	CommandList->MapAction(
		Commands.MoveLayerUp,
		FExecuteAction::CreateSP(this, &STileLayerList::MoveLayerUp));

	CommandList->MapAction(
		Commands.MoveLayerDown,
		FExecuteAction::CreateSP(this, &STileLayerList::MoveLayerDown));

	FToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization("TileLayerBrowserToolbar"), TSharedPtr<FExtender>(), Orient_Horizontal, /*InForceSmallIcons=*/ true);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	ToolbarBuilder.AddToolBarButton(Commands.AddNewLayerAbove);
	ToolbarBuilder.AddToolBarButton(Commands.MoveLayerUp);
	ToolbarBuilder.AddToolBarButton(Commands.MoveLayerDown);
	ToolbarBuilder.AddToolBarButton(Commands.DuplicateLayer);
	ToolbarBuilder.AddToolBarButton(Commands.DeleteLayer);

	TSharedRef<SWidget> Toolbar = ToolbarBuilder.MakeWidget();

	ListViewWidget = SNew(SPaperLayerListView)
		.SelectionMode(ESelectionMode::Single)

		// Point the tree to our array of root-level items.  Whenever this changes, we'll call RequestTreeRefresh()
		.ListItemsSource(&(TileMap->TileLayers))

		// Find out when the user selects something in the tree
		//.OnSelectionChanged( this, &SLevelsView::OnSelectionChanged )

		// Called when the user double-clicks with LMB on an item in the list
		//.OnMouseButtonDoubleClick( this, &SLevelsView::OnListViewMouseButtonDoubleClick )

		// Generates the actual widget for a tree item
		.OnGenerateRow(this, &STileLayerList::OnGenerateRowDefault)

		// Use the level viewport context menu as the right click menu for list items
		//.OnContextMenuOpening( InArgs._ConstructContextMenu )
		.HeaderRow(HeaderRowWidget);

	// Select the top item by default
	if (TileMap->TileLayers.Num() > 0)
	{
		SetSelectedLayer(TileMap->TileLayers[TileMap->TileLayers.Num() - 1]);
	}

	ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			ListViewWidget.ToSharedRef()
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			Toolbar
		]
	];
}

TSharedRef<ITableRow> STileLayerList::OnGenerateRowDefault(class UPaperTileLayer* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STileLayerItem, Item, OwnerTable);		
}

UPaperTileLayer* STileLayerList::GetSelectedLayer() const
{
	return (ListViewWidget->GetNumItemsSelected() > 0) ? ListViewWidget->GetSelectedItems()[0] : nullptr;
}

UPaperTileLayer* STileLayerList::AddLayer(bool bCollisionLayer, int32 InsertionIndex)
{
	UPaperTileLayer* NewLayer = NULL;

	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TileMapAddLayer", "Add New Layer"));
		TileMap->SetFlags(RF_Transactional);
		TileMap->Modify();

		// Create a set of existing names
		TSet<FString> ExistingNames;
		for (UPaperTileLayer* ExistingLayer : TileMap->TileLayers)
		{
			ExistingNames.Add(ExistingLayer->LayerName.ToString());
		}

		// Find a good name
		FText TestLayerName;
		do
		{
			TileMap->LayerNameIndex++;

			FNumberFormattingOptions NoGroupingFormat;
			NoGroupingFormat.SetUseGrouping(false);

			TestLayerName = FText::Format(LOCTEXT("NewLayerNameFormatString", "Layer {0}"), FText::AsNumber(TileMap->LayerNameIndex, &NoGroupingFormat));
		} while (ExistingNames.Contains(TestLayerName.ToString()));

		// Create the new layer
		NewLayer = NewObject<UPaperTileLayer>(TileMap);
		NewLayer->SetFlags(RF_Transactional);

		NewLayer->LayerWidth = TileMap->MapWidth;
		NewLayer->LayerHeight = TileMap->MapHeight;
		NewLayer->DestructiveAllocateMap(NewLayer->LayerWidth, NewLayer->LayerHeight);
		NewLayer->LayerName = TestLayerName;
		NewLayer->bCollisionLayer = bCollisionLayer;

		// Insert the new layer
		if (TileMap->TileLayers.IsValidIndex(InsertionIndex))
		{
			TileMap->TileLayers.Insert(NewLayer, InsertionIndex);
		}
		else
		{
			TileMap->TileLayers.Add(NewLayer);
		}

		ListViewWidget->RequestListRefresh();
		TileMap->PostEditChange();

		// Change the selection set to select it
		SetSelectedLayer(NewLayer);
	}

	return NewLayer;
}

void STileLayerList::ChangeLayerOrdering(int32 OldIndex, int32 NewIndex)
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		if (TileMap->TileLayers.IsValidIndex(OldIndex) && TileMap->TileLayers.IsValidIndex(NewIndex))
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapReorderLayer", "Reorder Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			UPaperTileLayer* LayerToMove = TileMap->TileLayers[OldIndex];
			TileMap->TileLayers.RemoveAt(OldIndex);
			TileMap->TileLayers.Insert(LayerToMove, NewIndex);

			ListViewWidget->RequestListRefresh();
			TileMap->PostEditChange();
		}
	}
}

void STileLayerList::AddNewLayerAbove()
{
	AddLayer(/*bCollisionLayer=*/ false, GetSelectionIndex());
}

void STileLayerList::AddNewLayerBelow()
{
	AddLayer(/*bCollisionLayer=*/ false, GetSelectionIndex() + 1);
}

int32 STileLayerList::GetSelectionIndex() const
{
	int32 SelectionIndex = INDEX_NONE;

	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		if (UPaperTileLayer* SelectedLayer = GetSelectedLayer())
		{
			TileMap->TileLayers.Find(SelectedLayer, /*out*/ SelectionIndex);
		}
		else
		{
			SelectionIndex = TileMap->TileLayers.Num() - 1;
		}
	}

	return SelectionIndex;
}

void STileLayerList::DeleteLayer()
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const int32 DeleteIndex = GetSelectionIndex();
		if (DeleteIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapDeleteLayer", "Delete Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			TileMap->TileLayers.RemoveAt(DeleteIndex);

			TileMap->PostEditChange();
			ListViewWidget->RequestListRefresh();

			// Select the item below the one that just got deleted
			const int32 NewSelectionIndex = FMath::Min<int32>(DeleteIndex, TileMap->TileLayers.Num() - 1);
			if (TileMap->TileLayers.IsValidIndex(NewSelectionIndex))
			{
				SetSelectedLayer(TileMap->TileLayers[NewSelectionIndex]);
			}
		}
	}
}

void STileLayerList::DuplicateLayer()
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const int32 DuplicateIndex = GetSelectionIndex();
		if (DuplicateIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapDuplicateLayer", "Duplicate Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			UPaperTileLayer* NewLayer = DuplicateObject<UPaperTileLayer>(TileMap->TileLayers[DuplicateIndex], TileMap);
			TileMap->TileLayers.Insert(NewLayer, DuplicateIndex);
			//@TODO: Try renaming it to a better name (Photoshop uses "[OldName] copy", but with a prompt to let you rename it immediately)

			TileMap->PostEditChange();
			ListViewWidget->RequestListRefresh();

			// Select the duplicated layer
			SetSelectedLayer(NewLayer);
		}
	}
}

void STileLayerList::MergeLayerDown()
{
	//@TODO: TILEMAPS: Support merging layers down
}

void STileLayerList::MoveLayerUp()
{
	const int32 SelectedIndex = GetSelectionIndex();
	const int32 NewIndex = SelectedIndex - 1;
	ChangeLayerOrdering(SelectedIndex, NewIndex);
}

void STileLayerList::MoveLayerDown()
{
	const int32 SelectedIndex = GetSelectionIndex();
	const int32 NewIndex = SelectedIndex + 1;
	ChangeLayerOrdering(SelectedIndex, NewIndex);
}

void STileLayerList::SetSelectedLayer(UPaperTileLayer* SelectedLayer)
{
	ListViewWidget->SetSelection(SelectedLayer);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE