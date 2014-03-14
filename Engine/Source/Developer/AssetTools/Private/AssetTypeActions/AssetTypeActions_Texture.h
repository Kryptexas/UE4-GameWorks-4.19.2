// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Texture : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Texture", "BaseTexture"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UTexture::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return false; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::MaterialsAndTextures; }

private:
	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<UTexture>> Objects);

	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<UTexture>> Objects);

	/** Handler for when CreateMaterial is selected */
	void ExecuteCreateMaterial(TArray<TWeakObjectPtr<UTexture>> Objects);

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<UTexture>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UTexture>> Objects);

	/** Handler for when FindMaterials is selected */
	void ExecuteFindMaterials(TWeakObjectPtr<UTexture> Object);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<UTexture>> Objects) const;
};