// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace SceneOutliner
{
	struct TOutlinerTreeItem;
}

/**
 *	
 */
class ISceneOutlinerColumn : public TSharedFromThis< ISceneOutlinerColumn >
{

public:
	virtual FName GetColumnID() = 0;

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() = 0;

	virtual const TSharedRef< SWidget > ConstructRowWidget( const TWeakObjectPtr< AActor >&  Actor ) = 0;

	virtual bool ProvidesSearchStrings() = 0;

	virtual void PopulateActorSearchStrings( const AActor* const Actor, OUT TArray< FString >& OutSearchStrings ) const = 0;

	virtual bool SupportsSorting() const = 0;

	virtual void SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const = 0;
};