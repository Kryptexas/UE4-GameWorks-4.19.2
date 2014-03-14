// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

#include "StringClassReference.h"
#include "PropertyTag.h"

FStringClassReference::FStringClassReference(const class UClass* InClass)
{
	if (InClass)
	{
		ClassName = InClass->GetPathName();
	}
}

FStringClassReference::FStringClassReference(const FString& PathString)
	: ClassName(PathString)
{
	if (ClassName == TEXT("None"))
	{
		ClassName = TEXT("");
	}
}

bool FStringClassReference::Serialize(FArchive& Ar)
{
	Ar << ClassName;
	return true;
}

bool FStringClassReference::operator==(FStringClassReference const& Other) const
{
	return ClassName == Other.ClassName;
}

void FStringClassReference::operator=(FStringClassReference const& Other)
{
	ClassName = Other.ClassName;
}

bool FStringClassReference::ExportTextItem(FString& ValueStr, FStringClassReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	if (!ClassName.IsEmpty())
	{
		ValueStr += ClassName;
	}
	else
	{
		ValueStr += TEXT("None");
	}
	return true;
}

bool FStringClassReference::ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText )
{
	ClassName = TEXT("");
	const TCHAR* NewBuffer = UPropertyHelpers::ReadToken( Buffer, ClassName, 1 );
	if( !NewBuffer )
	{
		return false;
	}
	Buffer = NewBuffer;
	if( ClassName==TEXT("None") )
	{
		ClassName = TEXT("");
	}
	else
	{
		if( *Buffer == TCHAR('\'') )
		{
			NewBuffer = UPropertyHelpers::ReadToken( Buffer, ClassName, 1 );
			if( !NewBuffer )
			{
				return false;
			}
			Buffer = NewBuffer;
			if( *Buffer++ != TCHAR('\'') )
			{
				return false;
			}
		}
	}
	return true;
}

bool FStringClassReference::SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar)
{
	if (Tag.Type == NAME_ClassProperty)
	{
		UClass* Class = NULL;
		Ar << Class;
		if (Class)
		{
			ClassName = Class->GetPathName();
		}
		else
		{
			ClassName = FString();
		}
		return true;
	}
	else if( Tag.Type == NAME_StrProperty )
	{
		FString String;
		Ar << String;

		ClassName = String;
		return true;
	}
	return false;
}

UClass *FStringClassReference::ResolveClass() const
{
	if (!IsValid())
	{
		return NULL;
	}

	// construct full name
	FString FullPath = ToString();
	UClass* FoundClass = FindObject<UClass>(ANY_PACKAGE, *FullPath);
	return FoundClass;
}

FStringClassReference FStringClassReference::GetOrCreateIDForClass(const class UClass *InClass)
{
	check(InClass);
	return FStringClassReference(InClass);
}
