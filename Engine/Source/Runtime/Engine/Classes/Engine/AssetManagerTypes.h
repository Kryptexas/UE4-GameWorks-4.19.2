// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrimaryAssetId.h"
#include "AssetData.h"
#include "AssetBundleData.h"
#include "EngineTypes.h"
#include "AssetManagerTypes.generated.h"

/** Rule about when to cook/ship a primary asset */
UENUM()
enum class EPrimaryAssetCookRule : uint8
{
	/** Nothing specifically known about this asset, it will cook if something else depends on it */
	Unknown,

	/** Asset should never be cooked/shipped in any situation, error if it something depends on it */
	NeverCook,

	/** Asset can be cooked for development and testing, but should never be shipped in a production build */
	DevelopmentCook,

	/** Asset should always be cooked, for production and development */
	AlwaysCook,
};

/** Structure defining rules for what to do with assets, this is defined per type and can be overridden per asset */
USTRUCT()
struct FPrimaryAssetRules
{
	GENERATED_BODY()
	
	/** Primary Assets with a higher priority will take precedence over lower priorities when assigning management for referenced assets. If priorities match, both will manage the same asset. */
	UPROPERTY(EditAnywhere, Category = Rules)
	int32 Priority;

	/** If true, will apply this rule to all referenced assets recurively, if they are not managed by a higher priority */
	UPROPERTY(EditAnywhere, Category = Rules)
	bool bApplyRecursively;

	/** Assets will be put into this chunk id, if not -1 */
	UPROPERTY(EditAnywhere, Category = Rules)
	int32 ChunkId;

	/** Rather this asset should be cooked or not */
	UPROPERTY(EditAnywhere, Category = Rules)
	EPrimaryAssetCookRule CookRule;

	FPrimaryAssetRules() 
		: Priority(-1), bApplyRecursively(true), ChunkId(-1), CookRule(EPrimaryAssetCookRule::Unknown)
	{
	}

	bool operator==(const FPrimaryAssetRules& Other) const
	{
		return Priority == Other.Priority
			&& bApplyRecursively == Other.bApplyRecursively
			&& ChunkId == Other.ChunkId
			&& CookRule == Other.CookRule;
	}

	/** Checks if all rules are the same as the default, if so this will be ignored */
	ENGINE_API bool IsDefault() const;

	/** Override non-default rules from an override struct */
	ENGINE_API void OverrideRules(const FPrimaryAssetRules& OverrideRules);

	/** Propagate cook rules from parent to child, won't override non default values */
	ENGINE_API void PropagateCookRules(const FPrimaryAssetRules& ParentRules);
};

/** Structure with publicly exposed information about an asset type. These can be loaded out of a config file */
USTRUCT()
struct FPrimaryAssetTypeInfo
{
	GENERATED_BODY()

	// Loaded out of ini or set via ScanPathsForPrimaryAssets

	/** Asset type name. This is a raw name as this is what generates the acceptable list */
	UPROPERTY(EditAnywhere, Category = AssetType)
	FName PrimaryAssetType;

private:
	/** Base Class of all assets of this type */
	UPROPERTY(EditAnywhere, Category = AssetType)
	TAssetSubclassOf<UObject> AssetBaseClass;
public:

	/** Base Class of all assets of this type */
	UPROPERTY(Transient)
	UClass* AssetBaseClassLoaded;

	/** True if the objects loaded are blueprints classes, false if they are normal UObjects */
	UPROPERTY(EditAnywhere, Category = AssetType)
	bool bHasBlueprintClasses;

	/** True if this type is editor only */
	UPROPERTY(EditAnywhere, Category = AssetType)
	bool bIsEditorOnly;

private:
	/** Directories to search for this asset type */
	UPROPERTY(EditAnywhere, Category = AssetType, meta = (RelativeToGameContentDir))
	TArray<FDirectoryPath> Directories;

	/** Individual assets to scan */
	UPROPERTY(EditAnywhere, Category = AssetType)
	TArray<FStringAssetReference> SpecificAssets;
public:

	/** Management rules for entire type, this is read out of ini but can be modified at run/cook time */
	UPROPERTY(EditAnywhere, Category = Rules, meta = (ShowOnlyInnerProperties))
	FPrimaryAssetRules Rules;

	/** Combination of directories and individual assets to search for this asset type. Will have the Directories and Assets added to it */
	UPROPERTY(Transient)
	TArray<FString> AssetScanPaths;

	/** True if this is an asset created at runtime that has no on disk representation. Cannot be set in config */
	UPROPERTY(Transient)
	bool bIsDynamicAsset;

	/** Number of tracked assets of that type */
	UPROPERTY(Transient)
	int32 NumberOfAssets;

	FPrimaryAssetTypeInfo() : AssetBaseClass(UObject::StaticClass()), AssetBaseClassLoaded(UObject::StaticClass()), bHasBlueprintClasses(false), bIsEditorOnly(false),  bIsDynamicAsset(false), NumberOfAssets(0) {}

	FPrimaryAssetTypeInfo(FName InPrimaryAssetType, UClass* InAssetBaseClass, bool bInHasBlueprintClasses, bool bInIsEditorOnly) 
		: PrimaryAssetType(InPrimaryAssetType), AssetBaseClass(InAssetBaseClass), AssetBaseClassLoaded(InAssetBaseClass), bHasBlueprintClasses(bInHasBlueprintClasses), bIsEditorOnly(bInIsEditorOnly), bIsDynamicAsset(false), NumberOfAssets(0)
	{
	}

	/** Fills out transient variables based on parsed ones */
	ENGINE_API bool FillRuntimeData();

};
