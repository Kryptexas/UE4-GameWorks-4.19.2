// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetData.h"
#include "AssetRegistryInterface.h"
#include "StreamableManager.h"
#include "AssetBundleData.h"
#include "AssetManager.generated.h"

/** Defined in C++ file */
struct FPrimaryAssetTypeData;
struct FPrimaryAssetData;

/** Structure with publicly exposed information about an asset type */
struct FPrimaryAssetTypeInfo
{
	/** Asset type name */
	FName PrimaryAssetType;

	/** Base Class of all assets of this type */
	UClass* AssetBaseClass;

	/** True if the objects loaded are blueprints classes, false if they are normal UObjects */
	bool bHasBlueprintClasses;

	/** True if this is an asset created at runtime that has no on disk representation */
	bool bIsDynamicAsset;

	/** True if this type is editor only */
	bool bIsEditorOnly;

	/** Number of tracked assets of that type */
	int32 NumberOfAssets;

	/** Paths to search for this asset type */
	TArray<FString> AssetDataPaths;

	FPrimaryAssetTypeInfo() : AssetBaseClass(UObject::StaticClass()), bHasBlueprintClasses(false), bIsDynamicAsset(false), bIsEditorOnly(false), NumberOfAssets(0) {}

	FPrimaryAssetTypeInfo(FName InPrimaryAssetType, UClass* InAssetBaseClass, bool bInHasBlueprintClasses, bool bInIsEditorOnly) 
		: PrimaryAssetType(InPrimaryAssetType), AssetBaseClass(InAssetBaseClass), bHasBlueprintClasses(bInHasBlueprintClasses), bIsDynamicAsset(false), bIsEditorOnly(bInIsEditorOnly), NumberOfAssets(0)
	{
	}
};

/** 
 * A singleton UObject that is responsible for loading and unloading PrimaryAssets, and maintaining game-specific asset references
 * Games should override this class and change the class reference
 */
UCLASS()
class ENGINE_API UAssetManager : public UObject
{
	GENERATED_BODY()

public:
	/** Constructor */
	UAssetManager();

	/** Returns true if there is a current asset manager */
	static bool IsValid();

	/** Returns the current AssetManager object */
	static UAssetManager& Get();

	/** Accesses the StreamableManager used by this Asset Manager. Static for easy access */
	static FStreamableManager& GetStreamableManager() { return Get().StreamableManager; }

	// ADDING TO ASSET DIRECTORY

	/** 
	 * Scans a list of paths and reads asset data for all primary assets of a specific type.
	 * If done in the editor it will load the data off disk, in cooked games it will load out of the asset registry cache
	 *
	 * @param PrimaryAssetType	Type of asset to look for. If the scanned asset matches GetPrimaryAssetType with this it will be added to directory
	 * @param Paths				List of file system paths to scan
	 * @param BaseClass			Base class of all loaded assets, if the scanned asset isn't a child of this class it will be skipped
	 * @param bHasBlueprintClasses	If true, the assets are blueprints that subclass BaseClass. If false they are base UObject assets
	 * @param bIsEditorOnly		If true, assets will only be scanned in editor builds, and will not be stored into the asset registry
	 * @param bForceSynchronousScan If true will scan the disk synchronously, otherwise will wait for asset registry scan to complete
	 * @return					Number of primary assets found
	 */
	virtual int32 ScanPathsForPrimaryAssets(FName PrimaryAssetType, const TArray<FString>& Paths, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly = false, bool bForceSynchronousScan = true);

	/** Single path wrapper */
	virtual int32 ScanPathForPrimaryAssets(FName PrimaryAssetType, const FString& Path, UClass* BaseClass, bool bHasBlueprintClasses, bool bIsEditorOnly = false, bool bForceSynchronousScan = true);
	
	/** Call this before many calls to ScanPaths to improve load performance. It will defer expensive updates until StopBulkScanning is called */
	virtual void StartBulkScanning();
	virtual void StopBulkScanning();

	/** 
	 * Adds or updates a Dynamic asset, which is a runtime-specified asset that has no on disk representation, so has no FAssetData. But it can have bundle state and a path.
	 *
	 * @param FPrimaryAssetId	Type/Name of the asset. The type info will be created if it doesn't already exist
	 * @param AssetPath			Path to the object representing this asset, this is optional for dynamic assets
	 * @param BundleData		List of Name->asset paths that represent the possible bundle states for this asset
	 * @return					True if added
	 */
	virtual bool AddDynamicAsset(const FPrimaryAssetId& PrimaryAssetId, const FStringAssetReference& AssetPath, const FAssetBundleData& BundleData);

	/** This will expand out references in the passed in AssetBundleData that are pointing to other primary assets with bundles. This is useful to preload entire webs of assets */
	virtual void RecursivelyExpandBundleData(FAssetBundleData& BundleData);

	// ACCESSING ASSET DIRECTORY

	/** Gets the FAssetData for a primary asset with the specified type/name, will only work for once that have been scanned for already. Returns true if it found a valid data */
	virtual bool GetPrimaryAssetData(const FPrimaryAssetId& PrimaryAssetId, FAssetData& AssetData) const;

	/** Gets list of all FAssetData for a primary asset type, returns true if any were found */
	virtual bool GetPrimaryAssetDataList(FName PrimaryAssetType, TArray<FAssetData>& AssetDataList) const;

	/** Gets the in-memory UObject for a primary asset id, returning nullptr if it's not in memory. Will return blueprint class for blueprint assets. This works even if the asset wasn't loaded explicitly */
	virtual UObject* GetPrimaryAssetObject(const FPrimaryAssetId& PrimaryAssetId) const;

	/** Templated versions of above */
	template<class AssetType> 
	FORCEINLINE AssetType* GetPrimaryAssetObject(const FPrimaryAssetId& PrimaryAssetId) const
	{
		UObject* ObjectReturn = GetPrimaryAssetObject(PrimaryAssetId);
		return Cast<AssetType>(ObjectReturn);
	}

	template<class AssetType>
	FORCEINLINE TSubclassOf<AssetType> GetPrimaryAssetObjectClass(const FPrimaryAssetId& PrimaryAssetId) const
	{
		TSubclassOf<AssetType> ReturnClass;
		ReturnClass = Cast<UClass>(GetPrimaryAssetObject(PrimaryAssetId));
		return ReturnClass;
	}

	/** Gets list of all loaded objects for a primary asset type, returns true if any were found. Will return blueprint class for blueprint assets. This works even if the asset wasn't loaded explicitly */
	virtual bool GetPrimaryAssetObjectList(FName PrimaryAssetType, TArray<UObject*>& ObjectList) const;

	/** Gets the FStringAssetReference for a primary asset type and name, returns invalid if not found */
	virtual FStringAssetReference GetPrimaryAssetPath(const FPrimaryAssetId& PrimaryAssetId) const;

	/** Gets the list of all FStringAssetReferences for a given type, returns true if any found */
	virtual bool GetPrimaryAssetPathList(FName PrimaryAssetType, TArray<FStringAssetReference>& AssetPathList) const;

	/** Sees if the passed in object path is a registered primary asset, if so return it. Returns invalid Identifier if not found */
	virtual FPrimaryAssetId GetPrimaryAssetIdForPath(const FStringAssetReference& ObjectPath) const;

	/** Parses AssetData to extract the primary type/name from it. This works even if it isn't in the directory yet */
	virtual FPrimaryAssetId GetPrimaryAssetIdFromData(const FAssetData& AssetData, FName SuggestedType = NAME_None) const;

	/** Gets list of all FPrimaryAssetId for a primary asset type, returns true if any were found */
	virtual bool GetPrimaryAssetIdList(FName PrimaryAssetType, TArray<FPrimaryAssetId>& PrimaryAssetIdList) const;

	/** Gets metadata for a specific asset type, returns false if not found */
	virtual bool GetPrimaryAssetTypeInfo(FName PrimaryAssetType, FPrimaryAssetTypeInfo& AssetTypeInfo) const;

	/** Gets list of all primary asset types infos */
	virtual void GetPrimaryAssetTypeInfoList(TArray<FPrimaryAssetTypeInfo>& AssetTypeInfoList) const;

	// ASYNC LOADING PRIMARY ASSETS

	/** 
	 * Loads a list of Primary Assets. This will start an async load of those assets, calling callback on completion.
	 * These assets will stay in memory until explicitly unloaded.
	 * You can wait on the returned streamable request or poll as needed.
	 * If there is no work to do, returned handle will be null and delegate will get called before function returns.
	 *
	 * @param AssetsToLoad		List of primary assets to load
	 * @param LoadBundles		List of bundles to load for those assets
	 * @param DelegateToCall	Delegate that will be called on completion, may be called before function returns if assets are already loaded
	 * @param Priority			Async loading priority for this request
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToLoad, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** Single asset wrapper */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAsset(const FPrimaryAssetId& AssetToLoad, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** Loads all assets of a given type, useful for cooking */
	virtual TSharedPtr<FStreamableHandle> LoadPrimaryAssetsWithType(FName PrimaryAssetType, const TArray<FName>& LoadBundles = TArray<FName>(), FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** 
	 * Unloads a list of Primary Assets. This will drop hard references to these assets, but they may be in memory due to other references or GC delay
	 *
	 * @param AssetsToUnload	List of primary assets to load
	 * @return					Number of assets unloaded
	 */
	virtual int32 UnloadPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToUnload);

	/** Single asset wrapper */
	virtual int32 UnloadPrimaryAsset(const FPrimaryAssetId& AssetToUnload);

	/** Loads all assets of a given type, useful for cooking */
	virtual int32 UnloadPrimaryAssetsWithType(FName PrimaryAssetType);

	/** 
	 * Changes the bundle state of a set of loaded primary assets.
	 * You can wait on the returned streamable request or poll as needed.
	 * If there is no work to do, returned handle will be null and delegate will get called before function returns.
	 *
	 * @param AssetsToChange	List of primary assets to change state of
	 * @param AddBundles		List of bundles to add
	 * @param RemoveBundles		Explicit list of bundles to remove
	 * @param RemoveAllBundles	If true, remove all existing bundles even if not in remove list
	 * @param DelegateToCall	Delegate that will be called on completion, may be called before function returns if assets are already loaded
	 * @param Priority			Async loading priority for this request
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	virtual TSharedPtr<FStreamableHandle> ChangeBundleStateForPrimaryAssets(const TArray<FPrimaryAssetId>& AssetsToChange, const TArray<FName>& AddBundles, const TArray<FName>& RemoveBundles, bool bRemoveAllBundles = false, FStreamableDelegate DelegateToCall = FStreamableDelegate(), TAsyncLoadPriority Priority = FStreamableManager::DefaultAsyncLoadPriority);

	/** 
	 * Returns the loading handle associated with the primary asset, it can then be checked for progress or waited on
	 *
	 * @param PrimaryAssetId	Asset to get handle for
	 * @param bForceCurrent		If true, returns the current handle. If false, will return pending if active, or current if not
	 * @param Bundles			If not null, will fill in with a list of the requested bundle state
	 * @return					Streamable Handle that can be used to poll or wait
	 */
	TSharedPtr<FStreamableHandle> GetPrimaryAssetHandle(const FPrimaryAssetId& PrimaryAssetId, bool bForceCurrent = false, TArray<FName>* Bundles = nullptr);

	/** 
	 * Returns a list of primary assets that are in the given bundle state. Only assets that are loaded or being loaded are valid
	 *
	 * @param PrimaryAssetList	Any valid assets are added to this list
	 * @param ValidTypes		List of types that are allowed. If empty, all types allowed
	 * @param RequiredBundles	Adds to list if the bundle state has all of these bundles. If empty will return all loaded
	 * @param ExcludedBundles	Doesn't add if the bundle state has any of these bundles
	 * @param bForceCurrent		If true, only use the current state. If false, use the current or pending
	 * @return					True if any found
	 */
	bool GetPrimaryAssetsWithBundleState(TArray<FPrimaryAssetId>& PrimaryAssetList, const TArray<FName>& ValidTypes, const TArray<FName>& RequiredBundles, const TArray<FName>& ExcludedBundles = TArray<FName>(), bool bForceCurrent = false);

	/** Fills in a TMap with the pending/active loading state of every asset */
	void GetPrimaryAssetBundleStateMap(TMap<FPrimaryAssetId, TArray<FName>>& BundleStateMap, bool bForceCurrent = false);

	/** Quick wrapper to async load some non primary assets with the primary streamable manager. This will not auto release the handle, release it if needed */
	virtual TSharedPtr<FStreamableHandle> LoadAssetList(const TArray<FStringAssetReference>& AssetList);

	/** Returns a single AssetBundleInfo, matching Scope and Name */
	virtual FAssetBundleEntry GetAssetBundleEntry(const FPrimaryAssetId& BundleScope, FName BundleName) const;

	/** Appends all AssetBundleInfos inside a given scope */
	virtual bool GetAssetBundleEntries(const FPrimaryAssetId& BundleScope, TArray<FAssetBundleEntry>& OutEntries) const;

	// GENERAL ASSET UTILITY FUNCTIONS

	/** Gets the FAssetData at a specific path, handles redirectors and blueprint classes correctly. Returns true if it found a valid data */
	virtual bool GetAssetDataForPath(const FStringAssetReference& ObjectPath, FAssetData& AssetData) const;

	/** Turns an FAssetData into FStringAssetReference, handles adding _C as necessary */
	virtual FStringAssetReference GetAssetPathForData(const FAssetData& AssetData, bool bIsBlueprint) const;

	/** Tries to redirect a Primary Asset Id, using list in AssetManagerSettings */
	virtual FPrimaryAssetId GetRedirectedPrimaryAssetId(const FPrimaryAssetId& OldId) const;

	/** Reads redirectory list and gets a list of the redirected previous names for a Primary Asset Id */
	virtual void GetPreviousPrimaryAssetIds(const FPrimaryAssetId& NewId, TArray<FPrimaryAssetId>& OutOldIds) const;

	/** Reads AssetManagerSEttings for specifically redirected asset paths. This is useful if you need to convert older saved data */
	virtual FString GetRedirectedAssetPath(const FString& OldPath) const;

	/** Extracts all FStringAssetReferences from a Class/Struct */
	virtual void ExtractStringAssetReferences(const UStruct* Struct, const void* StructValue, TArray<FStringAssetReference>& FoundAssetReferences, const TArray<FName>& PropertiesToSkip = TArray<FName>()) const;

	/** Dumps out summary of managed types to log */
	static void DumpAssetSummary();

	/** Starts initial load, gets called from InitializeObjectReferences */
	virtual void StartInitialLoading();

	/** Finishes initial loading, gets called from end of Engine::Init() */
	virtual void FinishInitialLoading();

	// Overrides
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	// EDITOR ONLY FUNCTIONALITY

	/** Gets package names to add to the cook. Do this instead of loading assets so RAM can be properly managed on build machines */
	virtual void ModifyCook(TArray<FString>& PackageNames);

	/** Handler to assign a given package to a streaming chunk */
	virtual void AssignStreamingChunk(const FString& PackageToAdd, const FString& LastLoadedMapName, const TArray<int32>& AssetRegistryChunkIDs, const TArray<int32>& ExistingChunkIds, TArray<int32>& OutChunkIndexList);

	/** Allows setting extra dependencies for the manifest */
	virtual bool GetPackageDependenciesForManifestGenerator(FName PackageName, TArray<FName>& DependentPackageNames, EAssetRegistryDependencyType::Type DependencyType);

	/** Refresh the entire set of asset data, can call from editor when things have changed dramatically */
	virtual void RefreshPrimaryAssetDirectory();

	/** Refreshes cache of asset data for in memory object */
	virtual void RefreshAssetData(UObject* ChangedObject);

	/** 
	 * Initializes asset bundle data from a passed in struct or class, this will read the AssetBundles metadata off the UProperties. As an example this property definition:
	 *
	 * UPROPERTY(EditDefaultsOnly, Category=Data, meta = (AssetBundles = "Client,Server"))
	 * TAssetPtr<class UCurveTable> CurveTableReference;
	 *
	 * Would add the value of CurveTableReference to both the Client and Server asset bundles
	 *
	 * @param Struct		UScriptStruct or UClass representing the property hierarchy
	 * @param StructValue	Location in memory of Struct or Object
	 * @param AssetBundle	Bundle that will be filled out
	 */
	virtual void InitializeAssetBundlesFromMetadata(const UStruct* Struct, const void* StructValue, FAssetBundleData& AssetBundle) const;

	/** UObject wrapper */
	virtual void InitializeAssetBundlesFromMetadata(const UObject* Object, FAssetBundleData& AssetBundle) const
	{
		InitializeAssetBundlesFromMetadata(Object->GetClass(), Object, AssetBundle);
	}
#endif

protected:
	/** Internal helper function that attempts to get asset data from the specified path; Accounts for possibility of blueprint classes ending in _C */
	virtual void GetAssetDataForPathInternal(class IAssetRegistry& AssetRegistry, const FString& AssetPath, OUT FAssetData& OutAssetData) const;

	/** Updates the asset data cached on the name data */
	virtual void UpdateCachedAssetData(const FPrimaryAssetId& PrimaryAssetId, const FAssetData& NewAssetData, bool bAllowDuplicates);

	/** Returns the NameData for a specific type/name pair */
	FPrimaryAssetData* GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector = true);
	const FPrimaryAssetData* GetNameData(const FPrimaryAssetId& PrimaryAssetId, bool bCheckRedirector = true) const;

	/** Loads the redirector maps */
	virtual void LoadRedirectorMaps();

	/** Rebuilds the ObjectReferenceList, needed after global object state has changed */
	virtual void RebuildObjectReferenceList();

	/** Called when an internal load handle finishes, handles setting to pending state */
	virtual void OnAssetStateChangeCompleted(FPrimaryAssetId PrimaryAssetId, TSharedPtr<FStreamableHandle> BoundHandle, FStreamableDelegate WrappedDelegate);

#if WITH_EDITOR
	/** Helper function which requests the asset register scan a filename from the asset path */
	virtual void ScanForAssetPath(class IAssetRegistry& AssetRegistry, const FString& AssetPath) const;

	/** Called when asset registry is done loading off disk, will finish any deferred loads */
	virtual void OnAssetRegistryFilesLoaded();

	/** Handles updating manager when a new asset is created */
	virtual void OnInMemoryAssetCreated(UObject *Object);

	/** Handles updating manager if deleted object is relevant*/
	virtual void OnInMemoryAssetDeleted(UObject *Object);

	/** When asset is renamed */
	virtual void OnAssetRenamed(const FAssetData& NewData, const FString& OldPath);

	/** Try to remove an old asset identifier when it has been deleted/renamed */
	virtual void RemovePrimaryAssetId(const FPrimaryAssetId& PrimaryAssetId);

	/** Called right before PIE starts, will refresh asset directory and can be overriden to preload assets */
	virtual void PreBeginPIE(bool bStartSimulate);

	/** Called after PIE ends, resets loading state */
	virtual void EndPIE(bool bStartSimulate);

	/** Copy of the asset state before PIE was entered, return to that when PIE completes */
	TMap<FPrimaryAssetId, TArray<FName>> PrimaryAssetStateBeforePIE;
#endif // WITH_EDITOR

	/** Per-type asset information */
	TMap<FName, TSharedRef<FPrimaryAssetTypeData>> AssetTypeMap;

	/** Map from string reference path to Primary Asset Identifier */
	TMap<FStringAssetReference, FPrimaryAssetId> AssetPathMap;

	/** Cached map of asset bundles, global and per primary asset */
	TMap<FPrimaryAssetId, TMap<FName, FAssetBundleEntry>> CachedAssetBundles;

	/** The streamable manager used for all primary asset loading */
	FStreamableManager StreamableManager;

	/** List of UObjects that are being kept from being GCd, derived from the asset type map. Arrays are currently more efficient than Sets */
	UPROPERTY()
	TArray<UObject*> ObjectReferenceList;

	/** True if we are running a build that is already scanning assets globally so we can perhaps avoid scanning paths synchronously */
	UPROPERTY()
	bool bIsGlobalAsyncScanEnvironment;

	/** True if we should keep hard references to loaded assets, to stop them from garbage collecting */
	UPROPERTY()
	bool bShouldKeepHardRefs;

	/** True if PrimaryAssetType/Name will be implied for loading assets that don't have it saved on disk. Won't work for all projects */
	UPROPERTY()
	bool bShouldGuessTypeAndName;

	/** True if we should always use synchronous loads, this speeds up cooking */
	UPROPERTY()
	bool bShouldUseSynchronousLoad;

	/** True if we are currently in bulk scanning mode */
	UPROPERTY()
	bool bIsBulkScanning;

	/** Redirector maps loaded out of AssetMigrations.ini */
	TMap<FName, FName> PrimaryAssetTypeRedirects;
	TMap<FString, FString> PrimaryAssetIdRedirects;
	TMap<FString, FString> AssetPathRedirects;
};
