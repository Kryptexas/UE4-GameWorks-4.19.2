// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_BehaviorTree : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_BehaviorTree", "Behavior Tree"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(129,50,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<class UBehaviorTree>> Objects);
};