// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A list item representing a collection */
struct FCollectionItem
{
	struct FCompareFCollectionItemByName
	{
		FORCEINLINE bool operator()( const TSharedPtr<FCollectionItem>& A, const TSharedPtr<FCollectionItem>& B ) const
		{
			return A->CollectionName < B->CollectionName;
		}
	};

	/** The name of the collection */
	FName CollectionName;

	/** The type of the collection */
	ECollectionShareType::Type CollectionType;

	/** Pointer to our parent collection (if any) */
	TWeakPtr<FCollectionItem> ParentCollection;

	/** Array of pointers to our child collections (if any) */
	TArray<TWeakPtr<FCollectionItem>> ChildCollections;

	/** If true, will set up an inline rename after next ScrollIntoView */
	bool bRenaming;

	/** If true, this item will be created the next time it is renamed */
	bool bNewCollection;

	/** Broadcasts whenever renaming a collection item is requested */
	DECLARE_MULTICAST_DELEGATE( FRenamedRequestEvent )

	/** Broadcasts whenever a rename is requested */
	FRenamedRequestEvent OnRenamedRequestEvent;

	/** Constructor */
	FCollectionItem(const FName& InCollectionName, const ECollectionShareType::Type& InCollectionType)
		: CollectionName(InCollectionName)
		, CollectionType(InCollectionType)
		, ParentCollection()
		, ChildCollections()
		, bRenaming(false)
		, bNewCollection(false)
	{}
};
