// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/SceneOutliner/Public/ISceneOutlinerColumn.h"

/**
 * A custom column for the SceneOutliner which allows the user to remove actors from
 * the contents of a layer with a single click
 */
class FSceneOutlinerLayerContentsColumn : public ISceneOutlinerColumn
{

public:

	/**
	 *	Constructor
	 */
	FSceneOutlinerLayerContentsColumn( const TSharedRef< class FLayerViewModel >& InViewModel );

	//////////////////////////////////////////////////////////////////////////
	// Begin ISceneOutlinerColumn Implementation

	virtual FName GetColumnID() OVERRIDE;

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() OVERRIDE;

	virtual const TSharedRef< SWidget > ConstructRowWidget( const TWeakObjectPtr< AActor >&  InActor ) OVERRIDE;

	virtual bool ProvidesSearchStrings();

	virtual void PopulateActorSearchStrings( const AActor* const InActor, OUT TArray< FString >& OutSearchStrings ) const OVERRIDE;
	
	virtual bool SupportsSorting() const OVERRIDE { return false; }

	virtual void SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const OVERRIDE {}

	// End ISceneOutlinerColumn Implementation
	//////////////////////////////////////////////////////////////////////////

private:

	FReply OnRemoveFromLayerClicked( const TWeakObjectPtr< AActor > Actor );


private:

	/**	 */
	const TSharedRef< FLayerViewModel > ViewModel;
};