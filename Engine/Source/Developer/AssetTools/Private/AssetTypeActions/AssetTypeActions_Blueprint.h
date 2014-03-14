// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_Blueprint : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Blueprint", "Blueprint"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor( 63, 126, 255 ); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UBlueprint::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Basic; }
	virtual void PerformAssetDiff(UObject* Asset1, UObject* Asset2, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const OVERRIDE;
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const OVERRIDE;
	virtual FText GetAssetDescription(const FAssetData& AssetData) const OVERRIDE;

protected:
	/** Handler for when Edit is selected */
	virtual void ExecuteEdit(TArray<TWeakObjectPtr<UBlueprint>> Objects);

	/** Whether or not this asset can create derived blueprints */
	virtual bool CanCreateNewDerivedBlueprint() const;

private:

	/* Called to open the blueprint defaults view, this opens whatever text diff tool the user has */
	void OpenInDefaults(class UBlueprint* OldBlueprint, class UBlueprint* NewBlueprint ) const;

	/** Handler for when EditDefaults is selected */
	void ExecuteEditDefaults(TArray<TWeakObjectPtr<UBlueprint>> Objects);

	/** Handler for when NewDerivedBlueprint is selected */
	void ExecuteNewDerivedBlueprint(TWeakObjectPtr<UBlueprint> InObject);

	/** Returns true if the blueprint is data only */
	bool ShouldUseDataOnlyEditor( const UBlueprint* Blueprint ) const;
};
