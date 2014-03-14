// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoundDefinitions.h"

class FAssetTypeActions_DialogueWave : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_DialogueWave", "Dialogue Wave"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(0,0,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UDialogueWave::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Sounds; }
	virtual bool CanFilter() OVERRIDE { return true; }
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UDialogueWave>> Objects);
};