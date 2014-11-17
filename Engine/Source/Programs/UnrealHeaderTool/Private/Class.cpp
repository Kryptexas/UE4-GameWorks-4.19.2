// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"
#include "Class.h"
#include "Classes.h"
#include "ClassMaps.h"

bool FClass::Inherits(const FClass* SuspectBase) const
{
	return IsChildOf(SuspectBase);
}

FString FClass::GetNameWithPrefix(EEnforceInterfacePrefix::Type EnforceInterfacePrefix) const
{
	const TCHAR* Prefix = 0;

	if (HasAnyClassFlags(CLASS_Interface))
	{
		// Grab the expected prefix for interfaces (U on the first one, I on the second one)
		switch (EnforceInterfacePrefix)
		{
			case EEnforceInterfacePrefix::None:
				// For old-style files: "I" for interfaces, unless it's the actual "Interface" class, which gets "U"
				if (GetFName() == NAME_Interface)
				{
					Prefix = TEXT("U");
				}
				else
				{
					Prefix = TEXT("I");
				}
				break;

			case EEnforceInterfacePrefix::I:
				Prefix = TEXT("I");
				break;

			case EEnforceInterfacePrefix::U:
				Prefix = TEXT("U");
				break;

			default:
				check(false);
		}
	}
	else
	{
		// Get the expected class name with prefix
		Prefix = GetPrefixCPP();
	}

	return FString::Printf(TEXT("%s%s"), Prefix, *GetName());
}

FClass* FClass::GetSuperClass() const
{
	return static_cast<FClass*>(static_cast<const UClass*>(this)->GetSuperClass());
}

FClass* FClass::GetClassWithin() const
{
	return (FClass*)ClassWithin;
}

TArray<FName> FClass::GetDependentNames() const
{
	TArray<FName> Result;

	for (auto Name : *GClassDependentOnMap.FindOrAdd(const_cast<FClass*>(this)))
	{
		Result.Add(Name);
	}

	return Result;
}

TArray<FClass*> FClass::GetInterfaceTypes() const
{
	TArray<FClass*> Result;
	for (auto& i : Interfaces)
	{
		Result.Add((FClass*)i.Class);
	}
	return Result;
}
