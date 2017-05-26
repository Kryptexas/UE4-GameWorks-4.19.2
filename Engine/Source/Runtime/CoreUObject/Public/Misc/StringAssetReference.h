// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeCounter.h"
#include "UObject/Class.h"
#include "HAL/ThreadSingleton.h"

/**
 * A struct that contains a string reference to an asset on disk.
 * This can be used to make soft references to assets that are loaded on demand.
 * This is stored internally as a string of the form /package/path.assetname[.objectname]
 * If the MetaClass metadata is applied to a UProperty with this the UI will restrict to that type of asset.
 */
struct COREUOBJECT_API FStringAssetReference
{
	FStringAssetReference() {}

	/** Construct from another asset reference */
	FStringAssetReference(const FStringAssetReference& Other)
	{
		SetPath(Other.ToString());
	}

	/** Construct from a path string */
	FStringAssetReference(FString PathString)
	{
		SetPath(MoveTemp(PathString));
	}

	/** Construct from an existing object in memory */
	FStringAssetReference(const UObject* InObject);

	~FStringAssetReference() {}

	/** Returns string representation of reference, in form /package/path.assetname[.objectname] */
	FORCEINLINE const FString& ToString() const
	{
		return AssetLongPathname;		
	}

	/** Returns /package/path, leaving off the asset name */
	FString GetLongPackageName() const
	{
		FString PackageName;
		ToString().Split(TEXT("."), &PackageName, nullptr, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		return PackageName;
	}
	
	/** Returns assetname string, leaving off the /package/path part */
	const FString GetAssetName() const
	{
		FString AssetName;
		ToString().Split(TEXT("."), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromStart);
		return AssetName;
	}	

	/** Sets asset path of this reference based on a string path */
	void SetPath(FString Path);

	/**
	 * Attempts to load the asset, this will call LoadObject which can be very slow
	 *
	 * @return Loaded UObject, or nullptr if the reference is null or the asset fails to load
	 */
	UObject* TryLoad() const;

	/**
	 * Attempts to find a currently loaded object that matches this path
	 *
	 * @return Found UObject, or nullptr if not currently in memory
	 */
	UObject* ResolveObject() const;

	/** Resets reference to point to null */
	void Reset()
	{		
		SetPath(TEXT(""));
	}
	
	/** Check if this could possibly refer to a real object, or was initialized to null */
	bool IsValid() const
	{
		return ToString().Len() > 0;
	}

	/** Checks to see if this is initialized to null */
	bool IsNull() const
	{
		return ToString().Len() == 0;
	}

	/** Struct overrides */
	bool Serialize(FArchive& Ar);
	bool operator==(FStringAssetReference const& Other) const;
	bool operator!=(FStringAssetReference const& Other) const
	{
		return !(*this == Other);
	}
	FStringAssetReference& operator=(FStringAssetReference const& Other);
	bool ExportTextItem(FString& ValueStr, FStringAssetReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText );
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

	/** Serializes the internal path and also handles save/PIE fixups. Call this from the archiver overrides */
	void SerializePath(FArchive& Ar, bool bSkipSerializeIfArchiveHasSize = false);

	/** Fixes up string asset reference for saving, call if saving with a method that skips SerializePath. This can modify the path */
	void PreSavePath();

	/** Handles when a string asset reference has been loaded, call if loading with a method that skips SerializePath. This does not modify path but might call callbacks */
	void PostLoadPath() const;

	FORCEINLINE friend uint32 GetTypeHash(FStringAssetReference const& This)
	{
		return GetTypeHash(This.ToString());
	}

	/** Code needed by FAssetPtr internals */
	static int32 GetCurrentTag()
	{
		return CurrentTag.GetValue();
	}
	static int32 InvalidateTag()
	{
		return CurrentTag.Increment();
	}
	static FStringAssetReference GetOrCreateIDForObject(const UObject* Object);
	
	/** Enables special PIE path handling, to remove pie prefix as needed */
	static void SetPackageNamesBeingDuplicatedForPIE(const TArray<FString>& InPackageNamesBeingDuplicatedForPIE);
	
	/** Disables special PIE path handling */
	static void ClearPackageNamesBeingDuplicatedForPIE();

private:
	/** Asset path */
	FString AssetLongPathname;

	/** Fixes up this StringAssetReference to add or remove the PIE prefix depending on what is currently active */
	void FixupForPIE();

	/** Global counter that determines when we need to re-search based on path because more objects have been loaded **/
	static FThreadSafeCounter CurrentTag;

	/** Package names currently being duplicated, needed by FixupForPIE */
	static TArray<FString> PackageNamesBeingDuplicatedForPIE;

	friend COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FStringAssetReference();
};

enum class EStringAssetReferenceCollectType : uint8
{
	/** References is not tracked in any situation, transient reference */
	NeverCollect,
	/** Editor only reference, this is tracked for redirector fixup but not for cooking */
	EditorOnlyCollect,
	/** Game reference, this is gathered for both redirector fixup and cooking */
	AlwaysCollect,
};

class COREUOBJECT_API FStringAssetReferenceThreadContext : public TThreadSingleton<FStringAssetReferenceThreadContext>
{
	friend TThreadSingleton<FStringAssetReferenceThreadContext>;
	friend struct FStringAssetReferenceSerializationScope;

	FStringAssetReferenceThreadContext() {}

	struct FSerializationOptions
	{
		FName PackageName;
		FName PropertyName;
		EStringAssetReferenceCollectType CollectType;

		FSerializationOptions() : CollectType(EStringAssetReferenceCollectType::AlwaysCollect) {}
		FSerializationOptions(FName InPackageName, FName InPropertyName, EStringAssetReferenceCollectType InCollectType) : PackageName(InPackageName), PropertyName(InPropertyName), CollectType(InCollectType) {}
	};

	TArray<FSerializationOptions> OptionStack;
public:
	/** 
	 * Returns the current serialization options that were added using SerializationScope or LinkerLoad
	 *
	 * @param OutPackageName Package that this string asset belongs to
	 * @param OutPropertyName Property that this string asset reference belongs to
	 * @param OutCollectType Type of collecting that should be done
	 */
	bool GetSerializationOptions(FName& OutPackageName, FName& OutPropertyName, EStringAssetReferenceCollectType& OutCollectType) const;
};

/** Helper class to set and restore serialization options for string asset references */
struct FStringAssetReferenceSerializationScope
{
	/** 
	 * Create a new serialization scope, which affects the way that string asset references are saved
	 *
	 * @param SerializingPackageName Package that this string asset belongs to
	 * @param SerializingPropertyName Property that this string asset reference belongs to
	 * @param CollectType Set type of collecting that should be done, can be used to disable tracking entirely
	 */
	FStringAssetReferenceSerializationScope(FName SerializingPackageName, FName SerializingPropertyName, EStringAssetReferenceCollectType CollectType)
	{
		FStringAssetReferenceThreadContext::Get().OptionStack.Emplace(SerializingPackageName, SerializingPropertyName, CollectType);
	}

	~FStringAssetReferenceSerializationScope()
	{
		FStringAssetReferenceThreadContext::Get().OptionStack.Pop();
	}
};