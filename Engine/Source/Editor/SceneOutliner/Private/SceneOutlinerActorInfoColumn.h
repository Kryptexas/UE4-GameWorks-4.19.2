// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISceneOutliner.h"
#include "ISceneOutlinerColumn.h"
#include "SceneOutlinerInitializationOptions.h"

/**
 * A custom column for the SceneOutliner which is capable of displaying a variety of Actor details
 */
class FSceneOutlinerActorInfoColumn : public ISceneOutlinerColumn
{

public:

	/**
	 *	Constructor
	 */
	FSceneOutlinerActorInfoColumn( const TWeakPtr< ISceneOutliner >& InSceneOutlinerWeak, ECustomColumnMode::Type InDefaultCustomColumnMode );

	//////////////////////////////////////////////////////////////////////////
	// Begin ISceneOutlinerColumn Implementation

	virtual FName GetColumnID() override;

	virtual SHeaderRow::FColumn::FArguments ConstructHeaderRowColumn() override;

	virtual const TSharedRef< SWidget > ConstructRowWidget( const TSharedRef<SceneOutliner::TOutlinerTreeItem> TreeItem ) override;

	virtual bool ProvidesSearchStrings() override;

	virtual void PopulateActorSearchStrings( const AActor* const Actor, OUT TArray< FString >& OutSearchStrings ) const override;

	virtual bool SupportsSorting() const override;

	virtual void SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const override;
	
	// End ISceneOutlinerColumn Implementation
	//////////////////////////////////////////////////////////////////////////

private:

	TSharedRef< SWidget > ConstructClassHyperlink( const TWeakObjectPtr< AActor >&  Actor );

	void OnModeChanged( TSharedPtr< ECustomColumnMode::Type > NewSelection, ESelectInfo::Type SelectInfo );

	EVisibility GetColumnDataVisibility( bool bIsClassHyperlink ) const;

	FString GetTextForItem( const TSharedRef<SceneOutliner::TOutlinerTreeItem> TreeItem ) const;

	FString GetTextForActor( const TWeakObjectPtr< AActor > TreeItem ) const;

	FText MakeComboText( const ECustomColumnMode::Type& Mode ) const;

	FText MakeComboToolTipText( const ECustomColumnMode::Type& Mode );

	TSharedRef< ITableRow > MakeComboButtonItemWidget( TSharedPtr< ECustomColumnMode::Type > Mode, const TSharedRef<STableViewBase> & );

	FText GetSelectedMode() const;

private:

	/** 
	 * Current custom column mode.  This is used for displaying a bit of extra data about the actors, as well as
	 * allowing the user to search by additional criteria 
	 */
	mutable ECustomColumnMode::Type CurrentMode;

	/** A list of available custom column modes for Slate */
	static TArray< TSharedPtr< ECustomColumnMode::Type > > ModeOptions;

	/** Weak reference to the outliner widget that owns our list */
	TWeakPtr< ISceneOutliner > SceneOutlinerWeak;
};
