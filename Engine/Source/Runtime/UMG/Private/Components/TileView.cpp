// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UTileView

UTileView::UTileView(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = true;

	ItemWidth = 128.0f;
	ItemHeight = 128.0f;
	SelectionMode = ESelectionMode::Single;
}

TSharedRef<SWidget> UTileView::RebuildWidget()
{
	return SNew(STileView< UObject* >)
		.SelectionMode(SelectionMode)
		.ListItemsSource(&Items)
		.ItemWidth(ItemWidth)
		.ItemHeight(ItemHeight)
		.OnGenerateTile(BIND_UOBJECT_DELEGATE(STileView< UObject* >::FOnGenerateRow, HandleOnGenerateTile))
		//.OnSelectionChanged(this, &SSocketManager::SocketSelectionChanged_Execute)
		//.OnContextMenuOpening(this, &SSocketManager::OnContextMenuOpening)
		//.OnItemScrolledIntoView(this, &SSocketManager::OnItemScrolledIntoView)
		//	.HeaderRow
		//	(
		//		SNew(SHeaderRow)
		//		.Visibility(EVisibility::Collapsed)
		//		+ SHeaderRow::Column(TEXT("Socket"))
		//	);
		;
}

TSharedRef<ITableRow> UTileView::HandleOnGenerateTile(UObject* Item, const TSharedRef< STableViewBase >& OwnerTable) const
{
	// Call the user's delegate to see if they want to generate a custom widget bound to the data source.
	if ( OnGenerateTileEvent.IsBound() )
	{
		UWidget* Widget = OnGenerateTileEvent.Execute(Item);
		if ( Widget != NULL )
		{
			return SNew(STableRow< UObject* >, OwnerTable)
			[
				Widget->GetWidget()
			];
		}
	}

	// If a row wasn't generated just create the default one, a simple text block of the item's name.
	return SNew(STableRow< UObject* >, OwnerTable)
		[
			SNew(STextBlock).Text(Item ? FText::FromString(Item->GetName()) : LOCTEXT("null", "null"))
		];
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
