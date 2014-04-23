// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"

#include "StringAssetReference.h"
#include "PropertyTag.h"

FStringAssetReference::FStringAssetReference(const class UObject* InObject)
{
	if (InObject)
	{
		AssetLongPathname = InObject->GetPathName();
	}
}

bool FStringAssetReference::Serialize(FArchive& Ar)
{
#if WITH_EDITOR
	if (Ar.IsSaving() && Ar.IsPersistent() && FCoreDelegates::StringAssetReferenceSaving.IsBound())
	{
		AssetLongPathname = FCoreDelegates::StringAssetReferenceSaving.Execute(AssetLongPathname);
	}
#endif // WITH_EDITOR
	Ar << AssetLongPathname;
#if WITH_EDITOR
	if (Ar.IsLoading() && Ar.IsPersistent() && FCoreDelegates::StringAssetReferenceLoaded.IsBound())
	{
		FCoreDelegates::StringAssetReferenceLoaded.Execute(AssetLongPathname);
	}
#endif // WITH_EDITOR

	if (Ar.IsLoading() && (Ar.GetPortFlags()&PPF_DuplicateForPIE))
	{
		// Remap unique ID if necessary
		FixupForPIE();
	}

	return true;
}
bool FStringAssetReference::operator==(FStringAssetReference const& Other) const
{
	return AssetLongPathname == Other.AssetLongPathname;
}
void FStringAssetReference::operator=(FStringAssetReference const& Other)
{
	AssetLongPathname = Other.AssetLongPathname;
}
bool FStringAssetReference::ExportTextItem(FString& ValueStr, FStringAssetReference const& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	if (!AssetLongPathname.IsEmpty())
	{
		ValueStr += AssetLongPathname;
	}
	else
	{
		ValueStr += TEXT("None");
	}
	return true;
}
bool FStringAssetReference::ImportTextItem( const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText )
{
	AssetLongPathname = TEXT("");
	const TCHAR* NewBuffer = UPropertyHelpers::ReadToken( Buffer, AssetLongPathname, 1 );
	if( !NewBuffer )
	{
		return false;
	}
	Buffer = NewBuffer;
	if( AssetLongPathname==TEXT("None") )
	{
		AssetLongPathname = TEXT("");
	}
	else
	{
		if( *Buffer == TCHAR('\'') )
		{
			NewBuffer = UPropertyHelpers::ReadToken( Buffer, AssetLongPathname, 1 );
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


bool FStringAssetReference::SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar)
{
	if (Tag.Type == NAME_ObjectProperty)
	{
		UObject* Object = NULL;
		Ar << Object;
		if (Object)
		{
			AssetLongPathname = Object->GetPathName();
		}
		else
		{
			AssetLongPathname = FString();
		}
		return true;
	}
	else if( Tag.Type == NAME_StrProperty )
	{
		FString String;
		Ar << String;

		AssetLongPathname = String;
		return true;
	}
	return false;
}

UObject *FStringAssetReference::ResolveObject() const
{
	// Don't try to resolve if we're saving a package because StaticFindObject can't be used here
	// and we usually don't want to force references to weak pointers while saving.
	if (!IsValid() || GIsSavingPackage)
	{
		return NULL;
	}
	UObject* FoundObject = NULL;
	// construct full name
	FString FullPath = ToString();
	FoundObject = StaticFindObject( UObject::StaticClass(), NULL, *FullPath);

	UObjectRedirector* Redir = Cast<UObjectRedirector>(FoundObject);
	if (Redir)
	{
		// If we found a redirector, follow it
		FoundObject = Redir->DestinationObject;
	}

	return FoundObject;
}

FStringAssetReference FStringAssetReference::GetOrCreateIDForObject(const class UObject *Object)
{
	check(Object);
	return FStringAssetReference(Object);
}

void FStringAssetReference::SetPackagesBeingDuplicatedForPIE(const TArray<UPackage*>& InPackagesBeingDuplicatedForPIE)
{
	for ( auto PackageIt = InPackagesBeingDuplicatedForPIE.CreateConstIterator(); PackageIt; ++PackageIt )
	{
		UPackage* Package = *PackageIt;
		if ( Package )
		{
			PackageNamesBeingDuplicatedForPIE.Add(Package->GetName());
		}
	}
}

void FStringAssetReference::ClearPackagesBeingDuplicatedForPIE()
{
	PackageNamesBeingDuplicatedForPIE.Empty();
}

void FStringAssetReference::FixupForPIE()
{
	if (FPlatformProperties::HasEditorOnlyData() && IsValid())
	{
		FString Path = ToString();

		// Determine if this reference has already been fixed up for PIE
		const FString ShortPackageOuterAndName = FPackageName::GetLongPackageAssetName(Path);
		if (!ShortPackageOuterAndName.StartsWith(PLAYWORLD_PACKAGE_PREFIX))
		{
			check(GPlayInEditorID != -1);

			// Determine if this refers to a package that is being duplicated for PIE
			for (auto PackageNameIt = PackageNamesBeingDuplicatedForPIE.CreateConstIterator(); PackageNameIt; ++PackageNameIt)
			{
				const FString PathPrefix = (*PackageNameIt) + TEXT(".");
				if (Path.StartsWith(PathPrefix))
				{
					// Need to prepend PIE prefix, as we're in PIE and this refers to an object in a PIE package
					Path = FString::Printf(TEXT("%s/%s_%d_%s"), *FPackageName::GetLongPackagePath(Path), PLAYWORLD_PACKAGE_PREFIX, GPlayInEditorID, *ShortPackageOuterAndName);
					AssetLongPathname = Path;

					break;
				}
			}
		}
	}
}

int32 FStringAssetReference::CurrentTag = 1;
TArray<FString> FStringAssetReference::PackageNamesBeingDuplicatedForPIE;