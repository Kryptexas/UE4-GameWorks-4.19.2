// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextureStats.h"

/** Stats page representing texture stats */
class FTextureStatsPage : public FStatsPage<UTextureStats>
{
public:
	/** Singleton accessor */
	static FTextureStatsPage& Get();

	/** Begin IStatsPage interface */
	virtual void Generate( TArray< TWeakObjectPtr<UObject> >& OutObjects ) const OVERRIDE;
	virtual void GenerateTotals( const TArray< TWeakObjectPtr<UObject> >& InObjects, TMap<FString, FText>& OutTotals ) const OVERRIDE;
	virtual void OnShow( TWeakPtr< class IStatsViewer > InParentStatsViewer ) OVERRIDE;
	virtual void OnHide() OVERRIDE;
	/** End IStatsPage interface */

private:

	/** Delegate to allow is to trigger a refresh on actor selection */
	void OnEditorSelectionChanged( UObject* NewSelection, TWeakPtr< class IStatsViewer > InParentStatsViewer );

	/** Delegate to allow is to trigger a refresh on new level */
	void OnEditorNewCurrentLevel( TWeakPtr< class IStatsViewer > InParentStatsViewer );
};

