// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// STileLayerList

class STileLayerList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STileLayerList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPaperTileMap* TileMap);

protected:
	typedef SListView<class UPaperTileLayer*> SPaperLayerListView;

protected:
	TSharedPtr<SPaperLayerListView> ListViewWidget;
	TSharedPtr<class FUICommandList> CommandList;
	TWeakObjectPtr<class UPaperTileMap> TileMapPtr;

protected:
	TSharedRef<ITableRow> OnGenerateRowDefault(class UPaperTileLayer* Item, const TSharedRef<STableViewBase>& OwnerTable);

	class UPaperTileLayer* GetSelectedLayer() const;

	// Returns the selected index if anything is selected, or the top item otherwise (only returns INDEX_NONE if there are no layers)
	int32 GetSelectionIndex() const;

	class UPaperTileLayer* AddLayer(bool bCollisionLayer);

	void AddNewLayerAbove();
	void AddNewLayerBelow();
	void DeleteLayer();
	void DuplicateLayer();
	void MergeLayerDown();
	void MoveLayerUp();
	void MoveLayerDown();
};
