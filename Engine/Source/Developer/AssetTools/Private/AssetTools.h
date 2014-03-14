// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTools : public IAssetTools
{
public:
	FAssetTools();
	virtual ~FAssetTools();

	// IAssetTools implementation
	virtual void RegisterAssetTypeActions(const TSharedRef<IAssetTypeActions>& NewActions) OVERRIDE;
	virtual void UnregisterAssetTypeActions(const TSharedRef<IAssetTypeActions>& ActionsToRemove) OVERRIDE;
	virtual void GetAssetTypeActionsList( TArray<TWeakPtr<IAssetTypeActions>>& OutAssetTypeActionsList ) const OVERRIDE;
	virtual TWeakPtr<IAssetTypeActions> GetAssetTypeActionsForClass( UClass* Class ) const OVERRIDE;
	virtual bool GetAssetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder, bool bIncludeHeading = true ) OVERRIDE;
	virtual UObject* CreateAsset(const FString& AssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, FName CallingContext = NAME_None) OVERRIDE;
	virtual UObject* DuplicateAsset(const FString& AssetName, const FString& PackagePath, UObject* OriginalObject) OVERRIDE;
	virtual void RenameAssets(const TArray<FAssetRenameData>& AssetsAndNames) const OVERRIDE;
	virtual TArray<UObject*> ImportAssets(const FString& DestinationPath) OVERRIDE;
	virtual TArray<UObject*> ImportAssets(const TArray<FString>& Files, const FString& DestinationPath) const OVERRIDE;
	virtual void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName) const OVERRIDE;
	virtual bool AssetUsesGenericThumbnail( const FAssetData& AssetData ) const OVERRIDE;
	virtual void DiffAgainstDepot(UObject* InObject, const FString& InPackagePath, const FString& InPackageName) const OVERRIDE;
	virtual void DiffAssets(UObject* OldAsset1, UObject* NewAsset, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const OVERRIDE;
	virtual FString DumpAssetToTempFile(UObject* Asset) const OVERRIDE;
	virtual bool CreateDiffProcess(const FString& DiffCommand, const FString& DiffArgs) const OVERRIDE;
	virtual void MigratePackages(const TArray<FName>& PackageNamesToMigrate) const OVERRIDE;

public:
	/** Gets the asset tools singleton as a FAssetTools for asset tools module use */
	static FAssetTools& Get();

	/** Syncs the primary content browser to the specified assets, whether or not it is locked. Most syncs that come from AssetTools -feel- like they came from the content browser, so this is okay. */
	void SyncBrowserToAssets(const TArray<UObject*>& AssetsToSync);
	void SyncBrowserToAssets(const TArray<FAssetData>& AssetsToSync);

private:
	/** Checks to see if a package is marked for delete then ask the user if he would like to check in the deleted file before he can continue. Returns true when it is safe to proceed. */
	bool CheckForDeletedPackage(const UPackage* Package) const;

	/** Returns true if the supplied Asset name and package are currently valid for creation. */
	bool CanCreateAsset(const FString& AssetName, const FString& PackageName, const FText& OperationText) const;

	/** Begins the package migration, after assets have been discovered */
	void PerformMigratePackages(TArray<FName> PackageNamesToMigrate) const;

	/** Copies files after the final list was confirmed */
	void MigratePackages_ReportConfirmed(TArray<FString> ConfirmedPackageNamesToMigrate) const;

	/** Gets the dependencies of the specified package recursively */
	void RecursiveGetDependencies(const FName& PackageName, TSet<FName>& AllDependencies) const;

	/** Records the time taken for an import and reports it to engine analytics, if available */
	static void OnNewImportRecord(UClass* AssetType, const FString& FileExtension, bool bSucceeded, bool bWasCancelled, const FDateTime& StartTime);

private:
	/** The manager to handle renaming assets */
	TSharedRef<FAssetRenameManager> AssetRenameManager;

	/** The list of all registered AssetTypeActions */
	TArray<TSharedRef<IAssetTypeActions>> AssetTypeActionsList;
};
