// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_SkeletalMesh : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_SkeletalMesh", "Skeletal Mesh"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,255); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return USkeletalMesh::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Basic | EAssetTypeCategories::Animation; }
	virtual class UThumbnailInfo* GetThumbnailInfo(UObject* Asset) const OVERRIDE;

protected:
	/** Gets additional actions that do not apply to destructible meshes */
	virtual void GetNonDestructibleActions( const TArray<TWeakObjectPtr<USkeletalMesh>>& Meshes, FMenuBuilder& MenuBuilder);

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when NewAnimBlueprint is selected */
	void ExecuteNewAnimBlueprint(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Creates a new object of type T from factory TFactory */
	template <typename TFactory, typename T>
	void ExecuteNewAnimAsset(TArray<TWeakObjectPtr<USkeletalMesh>> Objects, const FString InSuffix);

	/** Creates animation assets using the BaseName+Suffix */
	void CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeletalMesh>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix);

	/** Handler for when Reimport is selected */
	void ExecuteReimport(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when skeletal mesh LOD import is selected */
	void LODImport(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when skeletal mesh LOD sub menu is opened */
	void GetLODMenu(class FMenuBuilder& MenuBuilder,TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when create asset sub menu is opened */
	void GetCreateMenu(class FMenuBuilder& MenuBuilder,TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for the blend space sub menu */
	void FillBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeletalMesh>> Objects );

	/** Handler for the blend space sub menu */
	void FillAimOffsetBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeletalMesh>> Objects );

	/** Handler for when FindInExplorer is selected */
	void ExecuteFindInExplorer(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when OpenInExternalEditor is selected */
	void ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Returns true to allow execution of source file commands */
	bool CanExecuteSourceCommands(TArray<TWeakObjectPtr<USkeletalMesh>> Objects) const;

	/** Handler for when NewPhysicsAsset is selected */
	void ExecuteNewPhysicsAsset(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);
	
	/** Handler for when NewSkeleton is selected */
	void ExecuteNewSkeleton(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when AssignSkeleton is selected */
	void ExecuteAssignSkeleton(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	/** Handler for when FindSkeleton is selected */
	void ExecuteFindSkeleton(TArray<TWeakObjectPtr<USkeletalMesh>> Objects);

	// Helper functions
private:
	/** Creates a physics asset based on the mesh */
	void CreatePhysicsAssetFromMesh(USkeletalMesh* SkelMesh) const;

	/** Assigns a skeleton to the mesh */
	void AssignSkeletonToMesh(USkeletalMesh* SkelMesh) const;
};
