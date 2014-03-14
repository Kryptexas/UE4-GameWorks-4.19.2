// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "STileLayerList.h"
#include "STileLayerItem.h"

//////////////////////////////////////////////////////////////////////////
// STileLayerList

void STileLayerList::Construct(const FArguments& InArgs, UPaperTileMapRenderComponent* TileMap)
{
	TSharedRef<SHeaderRow> HeaderRowWidget = SNew(SHeaderRow)
		+SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("TileLayerList", "TileLayerNameHeader", "Layer Name").ToString()).FillWidth(0.7)
		+SHeaderRow::Column("TileSet").DefaultLabel(NSLOCTEXT("TileLayerList", "TileLayerTileSetHeader", "Tile Set").ToString()).FillWidth(0.7);

	ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			SAssignNew(ListViewWidget, SPaperLayerListView)
			.SelectionMode( ESelectionMode::Single )

			// Point the tree to our array of root-level items.  Whenever this changes, we'll call RequestTreeRefresh()
			.ListItemsSource( &(TileMap->TileLayers) )

			// Find out when the user selects something in the tree
			//.OnSelectionChanged( this, &SLevelsView::OnSelectionChanged )

			// Called when the user double-clicks with LMB on an item in the list
			//.OnMouseButtonDoubleClick( this, &SLevelsView::OnListViewMouseButtonDoubleClick )

			// Generates the actual widget for a tree item
			.OnGenerateRow( this, &STileLayerList::OnGenerateRowDefault ) 

			// Use the level viewport context menu as the right click menu for list items
			//.OnContextMenuOpening( InArgs._ConstructContextMenu )
			.HeaderRow( HeaderRowWidget )			
		]
	];
}



TSharedRef<ITableRow> STileLayerList::OnGenerateRowDefault(class UPaperTileLayer* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STileLayerItem, Item, OwnerTable);		
}
