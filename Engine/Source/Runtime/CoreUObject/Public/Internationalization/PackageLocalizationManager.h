// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class IPackageLocalizationCache;

/** Singleton class that manages localized package data. */
class COREUOBJECT_API FPackageLocalizationManager
{
public:
	/**
	 * Initialize the manager using the given cache. This will perform an initial scan for localized packages.
	 *
	 * @param InCache	The cache the manager should use.
	 */
	void InitializeFromCache(const TSharedRef<IPackageLocalizationCache>& InCache);

	/**
	 * Initialize this manager using the default cache. This will perform an initial scan for localized packages.
	 */
	void InitializeFromDefaultCache();

	/**
	 * Try and find the localized package name for the given source package for the active culture.
	 *
	 * @param InSourcePackageName	The name of the source package to find.
	 *
	 * @return The localized package name, or NAME_None if there is no localized package.
	 */
	FName FindLocalizedPackageName(const FName InSourcePackageName) const;

	/**
	 * Try and find the localized package name for the given source package for the given culture.
	 *
	 * @param InSourcePackageName	The name of the source package to find.
	 * @param InCultureName			The name of the culture to find the package for.
	 *
	 * @return The localized package name, or NAME_None if there is no localized package.
	 */
	FName FindLocalizedPackageNameForCulture(const FName InSourcePackageName, const FString& InCultureName) const;

	/**
	 * Singleton accessor.
	 *
	 * @return The singleton instance of the localization manager.
	 */
	static FPackageLocalizationManager& Get();

private:
	/**
	 * Try and find the localized package name for the given source package for the given culture, but without going through the cache.
	 *
	 * @param InSourcePackageName	The name of the source package to find.
	 * @param InCultureName			The name of the culture to find the package for.
	 *
	 * @return The localized package name, or NAME_None if there is no localized package.
	 */
	FName FindLocalizedPackageNameNoCache(const FName InSourcePackageName, const FString& InCultureName) const;

	/** Pointer to our currently active cache. Only valid after Initialize has been called. */
	TSharedPtr<IPackageLocalizationCache> ActiveCache;
};
