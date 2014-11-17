// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_StaticMesh : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_StaticMesh", "Static Mesh"); }
	virtual FColor GetTypeColor() const override { return FColor(0, 255, 255); }
	virtual UClass* GetSupportedClass() const override { return UStaticMesh::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const override { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Basic; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const override;

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UStaticMesh>> Objects);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UStaticMesh>> Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<UStaticMesh>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UStaticMesh>> Objects);

	/** Handler for when CreateDestructibleMesh is selected */
	void ExecuteCreateDestructibleMesh(TArray<TWeakObjectPtr<UStaticMesh>> Objects);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<UStaticMesh>> Objects) const;

	/** Handler to provide the list of LODs that can be imported or reimported */
	void GetImportLODMenu(class FMenuBuilder& MenuBuilder,TArray<TWeakObjectPtr<UStaticMesh>> Objects);
};