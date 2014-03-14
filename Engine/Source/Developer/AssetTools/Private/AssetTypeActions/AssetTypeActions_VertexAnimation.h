// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_VertexAnimation : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VertexAnimation", "Vertex Animation"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(87,0,174); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UVertexAnimation::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Animation; }
	
	// Simply returns false. Vertex Animations are currently are unsupported. Rather than remove
	// the files in question; returning false from here disables the filer in the content browser.
	// Remove this when vertex anims are supported again.
	virtual bool CanFilter() OVERRIDE;

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray< TWeakObjectPtr<UVertexAnimation> > Objects);
};