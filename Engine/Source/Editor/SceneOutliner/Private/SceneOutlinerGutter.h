// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISceneOutliner.h"
#include "ISceneOutlinerColumn.h"
#include "SceneOutlinerInitializationOptions.h"

/**
 * A gutter for the SceneOutliner which is capable of displaying a variety of Actor details
 */
class FSceneOutlinerGutter : public ISceneOutlinerColumn
{

public:

	/**	Constructor */
	FSceneOutlinerGutter();

	// -----------------------------------------
	// ISceneOutlinerColumn Implementation
	virtual FName GetColumnID() OVERRIDE;

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() OVERRIDE;

	virtual const TSharedRef<SWidget> ConstructRowWidget(const TWeakObjectPtr<AActor>& Actor) OVERRIDE;

	virtual bool ProvidesSearchStrings() OVERRIDE { return false; }

	virtual void PopulateActorSearchStrings(const AActor* const Actor, OUT TArray< FString >& OutSearchStrings) const OVERRIDE {}

	virtual bool SupportsSorting() const OVERRIDE { return true; }

	virtual void SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const OVERRIDE;
	// -----------------------------------------

};