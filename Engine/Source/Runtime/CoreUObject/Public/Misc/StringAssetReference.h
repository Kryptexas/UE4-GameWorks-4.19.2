// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A struct that contains a string reference to an asset, can be used to make soft references to assets
 */
struct COREUOBJECT_API FStringAssetReference
{
	// Caution, this is mirrored in Object.h as a non export struct.
	FString AssetLongPathname;

	FStringAssetReference()
	{
	}
	FStringAssetReference(FStringAssetReference const& Other)
		: AssetLongPathname(Other.AssetLongPathname)
	{
	}

	/**
	 * Construct from a path string
	 */
	FStringAssetReference(const FString& PathString)
		: AssetLongPathname(PathString)
	{
		if (AssetLongPathname == TEXT("None"))
		{
			AssetLongPathname = TEXT("");
		}
	}

	/**
	 * Construct from an existing object, will do some string processing
	 */
	FStringAssetReference(const class UObject* InObject);

	/**
	 * Converts in to a string
	 */
	const FString& ToString() const 
	{
		return AssetLongPathname;
	}

	/**
	 * Fixes up this UniqueObjectID to add or remove the PIE prefix depending on what is currently active
	 */
	FStringAssetReference FixupForPIE() const;

	/**
	 * Attempts to find a currently loaded object that matches this object ID
	 * @return Found UObject, or NULL if not currently loaded
	 */
	class UObject *ResolveObject() const;

	/**
	 * Resets reference to point to NULL
	 */
	void Reset()
	{		
		AssetLongPathname = TEXT("");
	}
	
	/**
	 * Check if this could possibly refer to a real object, or was initialized to NULL
	 */
	bool IsValid() const
	{
		return AssetLongPathname.Len() > 0;
	}

	bool Serialize(FArchive& Ar);
	bool operator==(FStringAssetReference const& Other) const;
	bool operator!=(FStringAssetReference const& Other) const
	{
		return !(*this == Other);
	}
	void operator=(FStringAssetReference const& Other);
	bool ExportTextItem(FString& ValueStr, FStringAssetReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText );
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

	friend uint32 GetTypeHash(FStringAssetReference const& This)
	{
		return GetTypeHash(This.AssetLongPathname);
	}

	friend FArchive& operator<<(FArchive& Ar,FStringAssetReference& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	/** Code needed by AssetPtr to track rather object references should be rechecked */

	static int32 GetCurrentTag()
	{
		return CurrentTag;
	}
	static int32 InvalidateTag()
	{
		return CurrentTag++;
	}

	static FStringAssetReference GetOrCreateIDForObject(const class UObject *Object);

private:

	/** Global counter that determines when we need to re-search for GUIDs because more objects have been loaded **/
	static int32 CurrentTag;

};

template<>
struct TStructOpsTypeTraits<FStringAssetReference> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithZeroConstructor = true,
		WithSerializer = true,
		WithCopy = true,
		WithIdenticalViaEquality = true,
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithSerializeFromMismatchedTag = true,
	};
};
