// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_Blackboard : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Blackboard", "Blackboard"); }
	virtual FColor GetTypeColor() const override { return FColor(201, 29, 85); }
	virtual UClass* GetSupportedClass() const override;
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<class UBlackboardData>> Objects);
};