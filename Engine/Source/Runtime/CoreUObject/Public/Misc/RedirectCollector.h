// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RedirectCollector:  Editor-only global object that handles resolving redirectors and handling string asset cooking rules
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"

#if WITH_EDITOR

class COREUOBJECT_API FRedirectCollector
{
private:
	
	/** Helper struct for string asset reference tracking */
	struct FPackagePropertyPair
	{
		FPackagePropertyPair() : bReferencedByEditorOnlyProperty(false) {}

		FPackagePropertyPair(const FName& InPackage, const FName& InProperty)
		: Package(InPackage)
		, Property(InProperty)
		, bReferencedByEditorOnlyProperty(false)
		//, PackageCache(NAME_None)
		{}

		bool operator==(const FPackagePropertyPair& Other) const
		{
			return Package == Other.Package &&
				Property == Other.Property;
		}

		const FName& GetCachedPackageName() const
		{
			return Package;
			/*if (PackageCache == NAME_None)
			{
				PackageCache = FName(*Package);
			}
			return PackageCache;*/
		}


		void SetPackage(const FName& InPackage)
		{
			Package = InPackage;
		}
		void SetProperty(const FName& InProperty)
		{
			Property = InProperty;
		}
		
		void SetReferencedByEditorOnlyProperty(bool InReferencedByEditorOnlyProperty)
		{
			bReferencedByEditorOnlyProperty = InReferencedByEditorOnlyProperty;
		}

		const FName& GetPackage() const
		{
			return Package;
		}
		const FName& GetProperty() const
		{
			return Property;
		}

		bool GetReferencedByEditorOnlyProperty() const
		{
			return bReferencedByEditorOnlyProperty;
		}

	private:
		FName Package;
		FName Property;
		bool bReferencedByEditorOnlyProperty;
		// mutable FName PackageCache;
	};

public:

	/**
	 * Called from FStringAssetReference::PostLoadPath, registers this for later querying
	 * @param InString Name of the asset that was loaded
	 */
	void OnStringAssetReferenceLoaded(const FString& InString);

	/**
	 * Load the string asset references to resolve them, add that to the remap table, and empty the array
	 * @param FilterPackage If set, only fixup references that were created by FilterPackage. If empty, clear all of them
	 * @param bProcessAlreadyResolvedPackages If a package has already been resolved in a previous call of this function, should we process this package again and resolve it again?  
	 *             True = process it again, False = don't process it again
	 */
	void ResolveStringAssetReference(FName FilterPackage = NAME_None, bool bProcessAlreadyResolvedPackages = true);

	/**
	 * Returns the list of packages referenced by string asset references loaded by FilterPackage, and remove them from the internal list
	 * @param FilterPackage Return references made by loading this package. If passed null will return all references made with no explicit package
	 * @param OutReferencedPackages Return list of packages referenced by FilterPackage
	 * @param bGetEditorOnly If true will return references loaded by editor only objects, if false it will not
	 */
	void ProcessStringAssetReferencePackageList(FName FilterPackage, bool bGetEditorOnly, TSet<FName>& OutReferencedPackages);

	/** Adds a new mapping for redirector path to destination path, this is called from the Asset Registry to register all redirects it knows about */
	void AddAssetPathRedirection(const FString& OriginalPath, const FString& RedirectedPath);

	/** Removes an asset path redirection, call this when deleting redirectors */
	void RemoveAssetPathRedirection(const FString& OriginalPath);

	/** Returns a remapped asset path, if it returns null there is no relevant redirector */
	FString* GetAssetPathRedirection(const FString& OriginalPath);

	/**
	 * Do we have any references to resolve.
	 * @return true if we have references to resolve
	 */
	bool HasAnyStringAssetReferencesToResolve() const
	{
		return StringAssetReferences.Num() > 0;
	}

	/** Writes out log info for how long string asset references took to resolve */
	void LogTimers() const;

	DEPRECATED(4.17, "OnStringAssetReferenceSaved is deprecated, call GetAssetPathRedirection")
	FString OnStringAssetReferenceSaved(const FString& InString);

private:

	/** A gathered list string asset references , with the key being the string reference (GetPathName()) and the value equal to the package with the reference */
	TMultiMap<FName, FPackagePropertyPair> StringAssetReferences;

	/** When saving, apply this remapping to all string asset references */
	TMap<FString, FString> AssetPathRedirectionMap;
	TSet<FName> AlreadyRemapped;

	/** For StringAssetReferences map */
	FCriticalSection CriticalSection;
};

// global redirect collector callback structure
COREUOBJECT_API extern FRedirectCollector GRedirectCollector;

#endif // WITH_EDITOR