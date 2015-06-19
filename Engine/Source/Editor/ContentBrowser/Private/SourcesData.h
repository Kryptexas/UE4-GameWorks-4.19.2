// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FSourcesData
{
	TArray<FName> PackagePaths;
	TArray<FCollectionNameType> Collections;

	bool IsEmpty() const
	{
		return PackagePaths.Num() == 0 && Collections.Num() == 0;
	}

	FARFilter MakeFilter(bool bRecurse, bool bUsingFolders) const
	{
		FARFilter Filter;

		// Package Paths
		Filter.PackagePaths = PackagePaths;
		Filter.bRecursivePaths = bRecurse || !bUsingFolders;

		// Collections
		TArray<FName> ObjectPathsFromCollections;
		if ( Collections.Num() && FCollectionManagerModule::IsModuleAvailable() )
		{
			// Collection manager module should already be loaded since it may cause a hitch on the first search
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();

			// Include objects from child collections if we're recursing
			const ECollectionRecursionFlags::Flags CollectionRecursionMode = (Filter.bRecursivePaths) ? ECollectionRecursionFlags::SelfAndChildren : ECollectionRecursionFlags::Self;

			for ( int32 CollectionIdx = 0; CollectionIdx < Collections.Num(); ++CollectionIdx )
			{
				// Find the collection
				const FCollectionNameType& CollectionNameType = Collections[CollectionIdx];
				CollectionManagerModule.Get().GetObjectsInCollection(CollectionNameType.Name, CollectionNameType.Type, ObjectPathsFromCollections, CollectionRecursionMode);
			}
		}
		Filter.ObjectPaths = ObjectPathsFromCollections;

		return Filter;
	}
};