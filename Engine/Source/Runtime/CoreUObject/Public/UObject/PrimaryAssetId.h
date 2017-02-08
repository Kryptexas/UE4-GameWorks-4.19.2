// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * This identifies an object as a "primary" asset that can be searched for by the AssetManager and used in various tools
 */
struct FPrimaryAssetId
{
	/** The Type of this object, by default it's base class's name */
	FName PrimaryAssetType;
	/** The Name of this object, by default it's short name */
	FName PrimaryAssetName;

	/** Static names to represent the AssetRegistry tags for the above data */
	static COREUOBJECT_API const FName PrimaryAssetTypeTag;
	static COREUOBJECT_API const FName PrimaryAssetNameTag;

	FPrimaryAssetId(FName InAssetType, FName InAssetName)
		: PrimaryAssetType(InAssetType), PrimaryAssetName(InAssetName)
	{}

	FPrimaryAssetId(FString InString)
	{
		FString TypeString;
		FString NameString;

		if (InString.Split(TEXT(":"), &TypeString, &NameString))
		{
			PrimaryAssetType = *TypeString;
			PrimaryAssetName = *NameString;
		}
	}

	FPrimaryAssetId()
		: PrimaryAssetType(NAME_None), PrimaryAssetName(NAME_None)
	{}

	/** Returns true if this is a valid identifier */
	bool IsValid() const
	{
		return PrimaryAssetType != NAME_None && PrimaryAssetName != NAME_None;
	}

	/** Returns string version of this identifier in Type:Name format */
	FString ToString() const
	{
		return FString::Printf(TEXT("%s:%s"), *PrimaryAssetType.ToString(), *PrimaryAssetName.ToString());
	}

	/** Converts from Type:Name format */
	static FPrimaryAssetId FromString(const FString& String)
	{
		// To right of :: is value
		FString TypeString;
		FString NameString;

		if (String.Split(TEXT(":"), &TypeString, &NameString))
		{
			return FPrimaryAssetId(*TypeString, *NameString);
		}
		return FPrimaryAssetId();
	}

	friend inline bool operator==(const FPrimaryAssetId& A, const FPrimaryAssetId& B)
	{
		return A.PrimaryAssetType == B.PrimaryAssetType && A.PrimaryAssetName == B.PrimaryAssetName;
	}

	friend inline bool operator!=(const FPrimaryAssetId& A, const FPrimaryAssetId& B)
	{
		return !(A==B);
	}

	friend inline uint32 GetTypeHash(const FPrimaryAssetId& Key)
	{
		uint32 Hash = 0;

		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetType));
		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetName));
		return Hash;
	}
};