// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A struct that contains a string reference to a class, can be used to make soft references to classes
 */
struct COREUOBJECT_API FStringClassReference
{
	// Caution, this is mirrored in Object.h as a non export struct.
	FString ClassName;

	FStringClassReference()
	{
	}

	FStringClassReference(FStringClassReference const& Other)
		: ClassName(Other.ClassName)
	{
	}

	/**
	 * Construct from a path string
	 */
	FStringClassReference(const FString& PathString);

	/**
	 * Construct from an existing class, will do some string processing
	 */
	FStringClassReference(const class UClass* InClass);

	/**
	 * Converts in to a string
	 */
	const FString& ToString() const 
	{
		return ClassName;
	}

	/**
	 * Attempts to find a currently loaded object that matches this object ID
	 * @return Found UClass, or NULL if not currently loaded
	 */
	class UClass *ResolveClass() const;

	/**
	 * Resets reference to point to NULL
	 */
	void Reset()
	{		
		ClassName = TEXT("");
	}
	
	/**
	 * Check if this could possibly refer to a real class, or was initialized to NULL
	 */
	bool IsValid() const
	{
		return ClassName.Len() > 0;
	}

	bool Serialize(FArchive& Ar);
	bool operator==(FStringClassReference const& Other) const;
	bool operator!=(FStringClassReference const& Other) const
	{
		return !(*this == Other);
	}
	void operator=(FStringClassReference const& Other);
	bool ExportTextItem(FString& ValueStr, FStringClassReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;
	bool ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText );
	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

	friend uint32 GetTypeHash(FStringClassReference const& This)
	{
		return GetTypeHash(This.ClassName);
	}

	friend FArchive& operator<<(FArchive& Ar,FStringClassReference& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	static FStringClassReference GetOrCreateIDForClass(const class UClass *InClass);
};

template<>
struct TStructOpsTypeTraits<FStringClassReference> : public TStructOpsTypeTraitsBase
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
