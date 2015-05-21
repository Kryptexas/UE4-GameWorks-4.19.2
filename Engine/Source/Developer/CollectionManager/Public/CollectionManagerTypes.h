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
