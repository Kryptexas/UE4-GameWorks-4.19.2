// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_MovieScene : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MovieScene", "Movie Scene"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(200, 80, 80); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Animation; }

	virtual bool ShouldForceWorldCentric() OVERRIDE
	{
		// @todo sequencer: Hack to force world-centric mode for Sequencer
		return true;
	}

protected:
	/** Handler for when Edit is selected */
	virtual void ExecuteEdit(TArray<TWeakObjectPtr<class UMovieScene>> Objects);

};