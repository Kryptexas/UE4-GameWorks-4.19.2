// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "TextFilter.h"

typedef STreeView<TSharedPtr<FLevelModel>> SLevelsTreeWidget;
typedef TTextFilter<const TSharedPtr<FLevelModel>& > LevelTextFilter;

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldLevelsTreeView 
	: public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWorldLevelsTreeView) {}
		SLATE_ARGUMENT(TSharedPtr<FLevelCollectionModel>, InWorldModel)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	SWorldLevelsTreeView();
	~SWorldLevelsTreeView();

	/** Regenerates current items */
	void RefreshView();

private:
	/** Creates a list item for the tree view */
	TSharedRef<ITableRow> GenerateTreeRow(TSharedPtr<FLevelModel> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Handler for returning a list of children associated with a particular tree node */
	void GetChildrenForTree(TSharedPtr<FLevelModel> Item, FLevelModelList& OutChildren);

	/** @return the SWidget containing the context menu */
	TSharedPtr<SWidget> ConstructLevelContextMenu() const;

	/** Called by TreeView widget whenever tree item expanded or collapsed */
	void OnExpansionChanged(TSharedPtr<FLevelModel> Item, bool bIsItemExpanded);

	/** Called by TreeView widget whenever selection is changed */
	void OnSelectionChanged(const TSharedPtr<FLevelModel> Item, ESelectInfo::Type SelectInfo);

	/** Handles selection changes in data source */
	void OnUpdateSelection();

	/** 
	 *	Called by STreeView when the user double-clicks on an item 
	 *
	 *	@param	Item	The item that was double clicked
	 */
	void OnTreeViewMouseButtonDoubleClick(TSharedPtr<FLevelModel> Item);

	/**
	 * Checks to see if this widget supports keyboard focus.  Override this in derived classes.
	 *
	 * @return  True if this widget can take keyboard focus
	 */
	bool SupportsKeyboardFocus() const OVERRIDE;

	/**
	 * Called after a key is pressed when this widget has keyboard focus
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) OVERRIDE;

private:
	/** @returns Whether specified item should be expanded */
	bool IsTreeItemExpanded(TSharedPtr<FLevelModel> Item) const;

	/** Appends the Level's name to the OutSearchStrings array if the Level is valid */
	void TransformLevelToString(const TSharedPtr<FLevelModel>& Level, TArray<FString>& OutSearchStrings) const;
	
	/** @return Text entered in search box */
	FText GetSearchBoxText() const;

private:
	/**	Whether the view is currently updating the viewmodel selection */
	bool								bUpdatingSelection;
	/** Our list view widget */
	TSharedPtr<SLevelsTreeWidget>		TreeWidget;
	
	/** Items collection to display */
	TSharedPtr<FLevelCollectionModel>	WorldModel;

	/** The Header Row for the hierarchy */
	TSharedPtr<SHeaderRow>				HeaderRowWidget;

	/**	The LevelTextFilter that constrains which Levels appear in the hierarchy */
	TSharedPtr<LevelTextFilter>			SearchBoxLevelFilter;
};
