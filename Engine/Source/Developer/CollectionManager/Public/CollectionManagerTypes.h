// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct ECollectionShareType
{
	enum Type
	{
		CST_System,
		CST_Local,
		CST_Private,
		CST_Shared,

		CST_All
	};

	static Type FromString(const TCHAR* InString, const Type ReturnIfConversionFails = CST_All)
	{
		if(FPlatformString::Stricmp(InString, TEXT("System")) == 0)		return CST_System;
		if(FPlatformString::Stricmp(InString, TEXT("Local")) == 0)		return CST_Local;
		if(FPlatformString::Stricmp(InString, TEXT("Private")) == 0)	return CST_Private;
		if(FPlatformString::Stricmp(InString, TEXT("Shared")) == 0)		return CST_Shared;
		if(FPlatformString::Stricmp(InString, TEXT("All")) == 0)		return CST_All;
		return ReturnIfConversionFails;
	}

	static const TCHAR* ToString(const Type InType)
	{
		switch(InType)
		{
		case CST_System:
			return TEXT("System");
		case CST_Local:
			return TEXT("Local");
		case CST_Private:
			return TEXT("Private");
		case CST_Shared:
			return TEXT("Shared");
		case CST_All:
			return TEXT("All");
		default:
			break;
		}
		return TEXT("");
	}

	static FText ToText(const Type InType)
	{
		switch(InType)
		{
		case CST_System:
			return NSLOCTEXT("ECollectionShareType", "CST_System", "System");
		case CST_Local:
			return NSLOCTEXT("ECollectionShareType", "CST_Local", "Local");
		case CST_Private:
			return NSLOCTEXT("ECollectionShareType", "CST_Private", "Private");
		case CST_Shared:
			return NSLOCTEXT("ECollectionShareType", "CST_Shared", "Shared");
		case CST_All:
			return NSLOCTEXT("ECollectionShareType", "CST_All", "All");
		default:
			break;
		}
		return FText::GetEmpty();
	}

	static FText GetDescription(const Type InType)
	{
		switch (InType)
		{
		case CST_Local:
			return NSLOCTEXT("ECollectionShareType", "CST_Local_Description", "Local. This collection is only visible to you and is not in source control.");
		case CST_Private:
			return NSLOCTEXT("ECollectionShareType", "CST_Private_Description", "Private. This collection is only visible to you.");
		case CST_Shared:
			return NSLOCTEXT("ECollectionShareType", "CST_Shared_Description", "Shared. This collection is visible to everyone.");
		default:
			break;
		}
		return FText::GetEmpty();
	}

	static FName GetIconStyleName(const Type InType, const TCHAR* InSizeSuffix = TEXT(".Small"))
	{
		switch (InType)
		{
		case CST_Local:
			return FName(*FString::Printf(TEXT("ContentBrowser.Local%s"), InSizeSuffix));
		case CST_Private:
			return FName(*FString::Printf(TEXT("ContentBrowser.Private%s"), InSizeSuffix));
		case CST_Shared:
			return FName(*FString::Printf(TEXT("ContentBrowser.Shared%s"), InSizeSuffix));
		default:
			break;
		}
		return NAME_None;
	}
};

/** Controls how the collections manager will recurse when performing work against a given collection */
struct ECollectionRecursionFlags
{
	typedef uint8 Flags;
	enum Flag
	{
		/** Include the current collection when performing work */
		Self = 1<<0,
		/** Include the parent collections when performing work */
		Parents = 1<<1,
		/** Include the child collections when performing work */
		Children = 1<<2,

		/** Include parent collections in addition to the current collection */
		SelfAndParents = Self | Parents,
		/** Include child collections in addition to the current collection */
		SelfAndChildren = Self | Children,
		/** Include parent and child collections in addition to the current collection */
		All = Self | Parents | Children,
	};
};

/** A name/type pair to uniquely identify a collection */
struct FCollectionNameType
{
	FCollectionNameType(FName InName, ECollectionShareType::Type InType)
		: Name(InName)
		, Type(InType)
	{}

	bool operator==(const FCollectionNameType& Other) const
	{
		return Name == Other.Name && Type == Other.Type;
	}

	friend inline uint32 GetTypeHash( const FCollectionNameType& Key )
	{
		uint32 Hash = 0;
		HashCombine(Hash, GetTypeHash(Key.Name));
		HashCombine(Hash, GetTypeHash(Key.Type));
		return Hash;
	}

	FName Name;
	ECollectionShareType::Type Type;
};

class ICollectionRedirectorFollower
{
public:
	virtual ~ICollectionRedirectorFollower() {}

	/** Given an object path, will see if it needs to follow any redirectors, and if so, will populate OutNewObjectPath with the new name and return true */
	virtual bool FixupObject(const FName& InObjectPath, FName& OutNewObjectPath) = 0;
};
