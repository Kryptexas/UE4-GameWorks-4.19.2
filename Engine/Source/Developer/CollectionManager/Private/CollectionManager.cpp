// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CollectionManagerPrivatePCH.h"

#define LOCTEXT_NAMESPACE "CollectionManager"

FCollectionManager::FCollectionManager()
{
	LastError = LOCTEXT("Error_Unknown", "None");

	CollectionFolders[ECollectionShareType::CST_Local] = FPaths::GameSavedDir() / TEXT("Collections");
	CollectionFolders[ECollectionShareType::CST_Private] = FPaths::GameUserDeveloperDir() / TEXT("Collections");
	CollectionFolders[ECollectionShareType::CST_Shared] = FPaths::GameContentDir() / TEXT("Collections");

	CollectionExtension = TEXT("collection");

	LoadCollections();
}

FCollectionManager::~FCollectionManager()
{
}

bool FCollectionManager::HasCollections() const
{
	return CachedCollections.Num() > 0;
}

void FCollectionManager::GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const
{
	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		if (ShareType == ECollectionShareType::CST_All || ShareType == CollectionKey.Type)
		{
			CollectionNames.Add(CollectionKey.Name);
		}
	}
}

bool FCollectionManager::CollectionExists(FName CollectionName, ECollectionShareType::Type ShareType) const
{
	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked to check all share types...
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			if (CachedCollections.Contains(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx))))
			{
				// Collection exists in at least one cache
				return true;
			}
		}

		// Collection not found in any cache
		return false;
	}
	else
	{
		return CachedCollections.Contains(FCollectionNameType(CollectionName, ShareType));
	}
}

bool FCollectionManager::GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& AssetsPaths) const
{
	auto GetAssetsInCollectionInternal = [this, &AssetsPaths](const FCollectionNameType& InCollectionKey) -> bool
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetAssetsInCollection(AssetsPaths);
			return true;
		}
		return false;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find assets in the specified collection name in any cache
		bool bFoundAssets = false;
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			bFoundAssets |= GetAssetsInCollectionInternal(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)));
		}
		return bFoundAssets;
	}
	else
	{
		return GetAssetsInCollectionInternal(FCollectionNameType(CollectionName, ShareType));
	}
}

bool FCollectionManager::GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths) const
{
	auto GetClassesInCollectionInternal = [this, &ClassPaths](const FCollectionNameType& InCollectionKey) -> bool
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetClassesInCollection(ClassPaths);
			return true;
		}
		return false;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find classes in the specified collection name in any cache
		bool bFoundAssets = false;
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			bFoundAssets |= GetClassesInCollectionInternal(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)));
		}
		return bFoundAssets;
	}
	else
	{
		return GetClassesInCollectionInternal(FCollectionNameType(CollectionName, ShareType));
	}
}

bool FCollectionManager::GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths) const
{
	auto GetObjectsInCollectionInternal = [this, &ObjectPaths](const FCollectionNameType& InCollectionKey) -> bool
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetObjectsInCollection(ObjectPaths);
			return true;
		}
		return false;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find objects in the specified collection name in any cache
		bool bFoundAssets = false;
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			bFoundAssets |= GetObjectsInCollectionInternal(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)));
		}
		return bFoundAssets;
	}
	else
	{
		return GetObjectsInCollectionInternal(FCollectionNameType(CollectionName, ShareType));
	}
}

void FCollectionManager::GetCollectionsContainingObject(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames) const
{
	const auto* ObjectCollectionsPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionsPtr)
	{
		for (const FCollectionNameType& CollectionKey : *ObjectCollectionsPtr)
		{
			if (ShareType == ECollectionShareType::CST_All || ShareType == CollectionKey.Type)
			{
				OutCollectionNames.Add(CollectionKey.Name);
			}
		}
	}
}

void FCollectionManager::GetCollectionsContainingObject(FName ObjectPath, TArray<FCollectionNameType>& OutCollections) const
{
	const auto* ObjectCollectionsPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionsPtr)
	{
		OutCollections.Append(*ObjectCollectionsPtr);
	}
}

void FCollectionManager::GetCollectionsContainingObjects(const TArray<FName>& ObjectPaths, TMap<FCollectionNameType, TArray<FName>>& OutCollectionsAndMatchedObjects) const
{
	for (const FName& ObjectPath : ObjectPaths)
	{
		const auto* ObjectCollectionsPtr = CachedObjects.Find(ObjectPath);
		if (ObjectCollectionsPtr)
		{
			for (const FCollectionNameType& CollectionKey : *ObjectCollectionsPtr)
			{
				TArray<FName>& MatchedObjects = OutCollectionsAndMatchedObjects.FindOrAdd(CollectionKey);
				MatchedObjects.Add(ObjectPath);
			}
		}
	}
}

FString FCollectionManager::GetCollectionsStringForObject(FName ObjectPath, ECollectionShareType::Type ShareType) const
{
	const auto* ObjectCollectionsPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionsPtr)
	{
		TArray<FString> CollectionNameStrings;

		for (const FCollectionNameType& CollectionKey : *ObjectCollectionsPtr)
		{
			if (ShareType == ECollectionShareType::CST_All || ShareType == CollectionKey.Type)
			{
				CollectionNameStrings.Add(CollectionKey.Name.ToString());
			}
		}

		if (CollectionNameStrings.Num() > 0)
		{
			CollectionNameStrings.Sort();
			return FString::Join(CollectionNameStrings, TEXT(", "));
		}
	}

	return FString();
}

void FCollectionManager::CreateUniqueCollectionName(const FName& BaseName, ECollectionShareType::Type ShareType, FName& OutCollectionName) const
{
	if (!ensure(ShareType != ECollectionShareType::CST_All))
	{
		return;
	}

	int32 IntSuffix = 1;
	bool CollectionAlreadyExists = false;
	do
	{
		if (IntSuffix <= 1)
		{
			OutCollectionName = BaseName;
		}
		else
		{
			OutCollectionName = *FString::Printf(TEXT("%s%d"), *BaseName.ToString(), IntSuffix);
		}

		CollectionAlreadyExists = CachedCollections.Contains(FCollectionNameType(OutCollectionName, ShareType));
		++IntSuffix;
	}
	while (CollectionAlreadyExists);
}

bool FCollectionManager::CreateCollection(FName CollectionName, ECollectionShareType::Type ShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	// Try to add the collection
	bool bUseSCC = ShouldUseSCC(ShareType);
	FString SourceFolder = CollectionFolders[ShareType];

	TSharedRef<FCollection> NewCollection = MakeShareable(new FCollection(CollectionName, SourceFolder, CollectionExtension, bUseSCC));
	if (!AddCollection(NewCollection, ShareType))
	{
		// Failed to add the collection, it already exists
		LastError = LOCTEXT("Error_AlreadyExists", "The collection already exists.");
		return false;
	}

	if (NewCollection->Save(LastError))
	{
		// Collection saved!
		CollectionCreatedEvent.Broadcast(FCollectionNameType(CollectionName, ShareType));
		return true;
	}
	else
	{
		// Collection failed to save, remove it from the cache
		RemoveCollection(NewCollection, ShareType);
		return false;
	}
}

bool FCollectionManager::RenameCollection(FName CurrentCollectionName, ECollectionShareType::Type CurrentShareType, FName NewCollectionName, ECollectionShareType::Type NewShareType)
{
	if (!ensure(CurrentShareType < ECollectionShareType::CST_All) || !ensure(NewShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	// Get the object paths for the assets in the collection
	TArray<FName> ObjectPaths;
	if (!GetAssetsInCollection(CurrentCollectionName, CurrentShareType, ObjectPaths))
	{
		// Failed to get assets in the current collection
		return false;
	}

	// Create a new collection
	if (!CreateCollection(NewCollectionName, NewShareType))
	{
		// Failed to create collection
		return false;
	}

	if (ObjectPaths.Num() > 0)
	{
		// Add all the objects from the old collection to the new collection
		if (!AddToCollection(NewCollectionName, NewShareType, ObjectPaths))
		{
			// Failed to add paths to the new collection. Destroy the collection we created.
			DestroyCollection(NewCollectionName, NewShareType);
			return false;
		}
	}

	// Delete the old collection
	if (!DestroyCollection(CurrentCollectionName, CurrentShareType))
	{
		// Failed to destroy the old collection. Destroy the collection we created.
		DestroyCollection(NewCollectionName, NewShareType);
		return false;
	}

	RebuildCachedObjects();

	// Success
	const FCollectionNameType OriginalCollectionKey(CurrentCollectionName, CurrentShareType);
	const FCollectionNameType NewCollectionKey(NewCollectionName, NewShareType);
	CollectionRenamedEvent.Broadcast(OriginalCollectionKey, NewCollectionKey);
	return true;
}

bool FCollectionManager::DestroyCollection(FName CollectionName, ECollectionShareType::Type ShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	const FCollectionNameType CollectionKey(CollectionName, ShareType);
	TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(CollectionKey);
	if (!CollectionRefPtr)
	{
		// The collection doesn't exist
		LastError = LOCTEXT("Error_DoesntExist", "The collection doesn't exist.");
		return false;
	}

	if ((*CollectionRefPtr)->DeleteSourceFile(LastError))
	{
		RemoveCollection(*CollectionRefPtr, ShareType);

		RebuildCachedObjects();

		CollectionDestroyedEvent.Broadcast(CollectionKey);
		return true;
	}
	else
	{
		// Failed to delete the source file
		return false;
	}
}

bool FCollectionManager::AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath)
{
	TArray<FName> Paths;
	Paths.Add(ObjectPath);
	return AddToCollection(CollectionName, ShareType, Paths);
}

bool FCollectionManager::AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumAdded)
{
	if (OutNumAdded)
	{
		*OutNumAdded = 0;
	}

	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	const FCollectionNameType CollectionKey(CollectionName, ShareType);
	TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(CollectionKey);
	if (!CollectionRefPtr)
	{
		// Collection doesn't exist
		LastError = LOCTEXT("Error_DoesntExist", "The collection doesn't exist.");
		return false;
	}

	int32 NumAdded = 0;
	for (const FName& ObjectPath : ObjectPaths)
	{
		if ((*CollectionRefPtr)->AddAssetToCollection(ObjectPath))
		{
			NumAdded++;
		}
	}

	if (NumAdded > 0)
	{
		if ((*CollectionRefPtr)->Save(LastError))
		{
			// Added and saved
			if (OutNumAdded)
			{
				*OutNumAdded = NumAdded;
			}

			RebuildCachedObjects();

			AssetsAddedEvent.Broadcast(CollectionKey, ObjectPaths);
			return true;
		}
		else
		{
			// Added but not saved, revert the add
			for (const FName& ObjectPath : ObjectPaths)
			{
				(*CollectionRefPtr)->RemoveAssetFromCollection(ObjectPath);
			}
			return false;
		}
	}
	else
	{
		// Failed to add, all of the objects were already in the collection
		LastError = LOCTEXT("Error_AlreadyInCollection", "All of the assets were already in the collection.");
		return false;
	}
}

bool FCollectionManager::RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath)
{
	TArray<FName> Paths;
	Paths.Add(ObjectPath);
	return RemoveFromCollection(CollectionName, ShareType, Paths);
}

bool FCollectionManager::RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumRemoved)
{
	if (OutNumRemoved)
	{
		*OutNumRemoved = 0;
	}

	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	const FCollectionNameType CollectionKey(CollectionName, ShareType);
	TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(CollectionKey);
	if (!CollectionRefPtr)
	{
		// Collection not found
		LastError = LOCTEXT("Error_DoesntExist", "The collection doesn't exist.");
		return false;
	}

	TArray<FName> RemovedAssets;
	for (const FName& ObjectPath : ObjectPaths)
	{
		if ((*CollectionRefPtr)->RemoveAssetFromCollection(ObjectPath))
		{
			RemovedAssets.Add(ObjectPath);
		}
	}

	if (RemovedAssets.Num() == 0)
	{
		// Failed to remove, none of the objects were in the collection
		LastError = LOCTEXT("Error_NotInCollection", "None of the assets were in the collection.");
		return false;
	}
			
	if ((*CollectionRefPtr)->Save(LastError))
	{
		// Removed and saved
		if (OutNumRemoved)
		{
			*OutNumRemoved = RemovedAssets.Num();
		}

		RebuildCachedObjects();

		AssetsRemovedEvent.Broadcast(CollectionKey, ObjectPaths);
		return true;
	}
	else
	{
		// Removed but not saved, revert the remove
		for (const FName& RemovedAssetName : RemovedAssets)
		{
			(*CollectionRefPtr)->AddAssetToCollection(RemovedAssetName);
		}
		return false;
	}
}

bool FCollectionManager::EmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return false;
	}

	TArray<FName> ObjectPaths;
	if (!GetAssetsInCollection(CollectionName, ShareType, ObjectPaths))
	{
		// Failed to load collection
		return false;
	}
	
	if (ObjectPaths.Num() == 0)
	{
		// Collection already empty
		return true;
	}
	
	if (RemoveFromCollection(CollectionName, ShareType, ObjectPaths))
	{
		RebuildCachedObjects();
		return true;
	}

	return false;
}

bool FCollectionManager::IsCollectionEmpty(FName CollectionName, ECollectionShareType::Type ShareType) const
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return true;
	}

	const FCollectionNameType CollectionKey(CollectionName, ShareType);
	const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(CollectionKey);
	if (CollectionRefPtr)
	{
		return (*CollectionRefPtr)->IsEmpty();
	}

	return true;
}

bool FCollectionManager::IsObjectInCollection(FName ObjectPath, FName CollectionName, ECollectionShareType::Type ShareType) const
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return true;
	}

	const FCollectionNameType CollectionKey(CollectionName, ShareType);
	const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(CollectionKey);
	if (CollectionRefPtr)
	{
		return (*CollectionRefPtr)->IsObjectInCollection(ObjectPath);
	}

	return false;
}

void FCollectionManager::LoadCollections()
{
	const double LoadStartTime = FPlatformTime::Seconds();
	const int32 PrevNumCollections = CachedCollections.Num();

	for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
	{
		const FString& CollectionFolder = CollectionFolders[CacheIdx];
		const FString WildCard = FString::Printf(TEXT("%s/*.%s"), *CollectionFolder, *CollectionExtension);

		TArray<FString> Filenames;
		IFileManager::Get().FindFiles(Filenames, *WildCard, true, false);

		for (const FString& BaseFilename : Filenames)
		{
			const FString Filename = CollectionFolder / BaseFilename;

			TSharedRef<FCollection> NewCollection = MakeShareable(new FCollection());
			const bool bUseSCC = ShouldUseSCC(ECollectionShareType::Type(CacheIdx));
			if (NewCollection->LoadFromFile(Filename, bUseSCC))
			{
				AddCollection(NewCollection, ECollectionShareType::Type(CacheIdx));
			}
			else
			{
				UE_LOG(LogCollectionManager, Warning, TEXT("Failed to load collection file %s"), *Filename);
			}
		}
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Loaded %d collections in %0.6f seconds" ), CachedCollections.Num() - PrevNumCollections, FPlatformTime::Seconds() - LoadStartTime);

	RebuildCachedObjects();
}

void FCollectionManager::RebuildCachedObjects()
{
	const double LoadStartTime = FPlatformTime::Seconds();

	CachedObjects.Empty();

	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		const TSharedRef<FCollection>& Collection = CachedCollection.Value;

		TArray<FName> ObjectsInCollection;
		Collection->GetObjectsInCollection(ObjectsInCollection);

		for (const FName& ObjectPath : ObjectsInCollection)
		{
			auto& ObjectCollections = CachedObjects.FindOrAdd(ObjectPath);
			ObjectCollections.AddUnique(CollectionKey);
		}
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Rebuilt the cache for %d objects in %0.6f seconds" ), CachedObjects.Num(), FPlatformTime::Seconds() - LoadStartTime);
}

bool FCollectionManager::ShouldUseSCC(ECollectionShareType::Type ShareType) const
{
	return ShareType != ECollectionShareType::CST_Local && ShareType != ECollectionShareType::CST_System;
}

bool FCollectionManager::AddCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		return false;
	}

	const FCollectionNameType CollectionKey(CollectionRef->GetCollectionName(), ShareType);
	if (CachedCollections.Contains(CollectionKey))
	{
		UE_LOG(LogCollectionManager, Warning, TEXT("Failed to add collection '%s' because it already exists."), *CollectionRef->GetCollectionName().ToString());
		return false;
	}

	CachedCollections.Add(CollectionKey, CollectionRef);
	return true;
}

bool FCollectionManager::RemoveCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		return false;
	}

	const FCollectionNameType CollectionKey(CollectionRef->GetCollectionName(), ShareType);
	return CachedCollections.Remove(CollectionKey) > 0;
}

#undef LOCTEXT_NAMESPACE
