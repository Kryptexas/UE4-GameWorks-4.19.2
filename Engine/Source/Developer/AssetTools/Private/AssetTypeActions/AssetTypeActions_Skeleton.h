// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Remap Skeleton Asset Data
 */
struct FAssetToRemapSkeleton
{
	FName					PackageName;
	TWeakObjectPtr<UObject> Asset;
	FString					FailureReason;
	bool					bRemapFailed;

	FAssetToRemapSkeleton(FName InPackageName)
		: PackageName(InPackageName)
		, bRemapFailed(false)
	{}

	// report it failed
	void ReportFailed(FString InReason)
	{
		FailureReason = InReason;
		bRemapFailed = true;
	}
};

class FAssetTypeActions_Skeleton : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_Skeleton", "Skeleton"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(105,181,205); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return USkeleton::StaticClass(); }
	virtual bool HasActions ( const TArray<UObject*>& InObjects ) const OVERRIDE { return true; }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) OVERRIDE;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Animation; }

	/** Handler for the blend space sub menu */
	void FillBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons );

	/** Handler for the blend space sub menu */
	void FillAimOffsetBlendSpaceMenu( FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons );

private:
	/** Handler for when Edit is selected */
	void ExecuteEdit(TArray<TWeakObjectPtr<USkeleton>> Objects);

	/** Creates a new object of type T from factory TFactory */
	template <typename TFactory, typename T>
	void ExecuteNewAnimAsset(TArray<TWeakObjectPtr<USkeleton>> Objects, const FString InSuffix);

	/** Handler for when NewAnimBlueprint is selected */
	void ExecuteNewAnimBlueprint(TArray<TWeakObjectPtr<USkeleton>> Objects);

	/** Handler for when Skeleton Retarget is selected */
	void ExecuteRetargetSkeleton(TArray<TWeakObjectPtr<USkeleton>> Skeletons);

private: // Helper functions
	/** Creates animation assets using the BaseName+Suffix */
	void CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeleton>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix);

 	/** 
	 * Main function for handling retargeting old Skeleton to new Skeleton
	 * 
	 * @param OldSkeleton : Old Skeleton that it changes from
	 * @param NewSkeleton : New Skeleton that it would change to
	 * @param Referencers : List of asset names that reference Old Skeleton
	 */
 	void PerformRetarget(USkeleton *OldSkeleton, USkeleton *NewSkeleton, TArray<FName> Packages) const;

	// utility functions for performing retargeting
	// these codes are from AssetRenameManager workflow
	void LoadPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& OutPackagesToSave) const;
	bool CheckOutPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const;
	void ReportFailures(const TArray<FAssetToRemapSkeleton>& AssetsToRemap) const;
	void RetargetSkeleton(TArray<FAssetToRemapSkeleton>& AssetsToRemap, USkeleton* OldSkeleton, USkeleton* NewSkeleton) const;
	void SavePackages(const TArray<UPackage*> PackagesToSave) const;
	void DetectReadOnlyPackages(TArray<FAssetToRemapSkeleton>& AssetsToRemap, TArray<UPackage*>& InOutPackagesToSave) const;
};