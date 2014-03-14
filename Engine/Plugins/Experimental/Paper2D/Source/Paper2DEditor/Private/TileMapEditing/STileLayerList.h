// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// STileLayerList

class STileLayerList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STileLayerList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPaperTileMapRenderComponent* TileMap);

protected:
	typedef SListView<class UPaperTileLayer*> SPaperLayerListView;

protected:
	TSharedPtr<SPaperLayerListView> ListViewWidget;

protected:
	TSharedRef<ITableRow> OnGenerateRowDefault(class UPaperTileLayer* Item, const TSharedRef<STableViewBase>& OwnerTable);
};
