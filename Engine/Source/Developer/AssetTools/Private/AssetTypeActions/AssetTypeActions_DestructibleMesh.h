// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_DestructibleMesh : public FAssetTypeActions_SkeletalMesh
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_DestructibleMesh", "Destructible Mesh"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(200,128,128); }
	virtual UClass* GetSupportedClass() const OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Physics; }
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;

	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<class UDestructibleMesh>> Objects);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<class UDestructibleMesh>> Objects);
};
