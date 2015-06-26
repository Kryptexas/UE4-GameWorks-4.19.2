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

void FCollectionManager::GetCollections(TArray<FCollectionNameType>& OutCollections) const
{
	OutCollections.Reserve(CachedCollections.Num());
	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		OutCollections.Add(CollectionKey);
	}
}

void FCollectionManager::GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const
{
	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		if (ShareType == ECollectionShareType::CST_All || ShareType == CollectionKey.Type)
		{
			CollectionNames.AddUnique(CollectionKey.Name);
		}
	}
}

void FCollectionManager::GetRootCollections(TArray<FCollectionNameType>& OutCollections) const
{
	OutCollections.Reserve(CachedCollections.Num());
	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		const TSharedRef<FCollection>& Collection = CachedCollection.Value;

		// A root collection either has no parent GUID, or a parent GUID that cannot currently be found - the check below handles both
		if (!CachedCollectionNamesFromGuids.Contains(Collection->GetParentCollectionGuid()))
		{
			OutCollections.Add(CollectionKey);
		}
	}
}

void FCollectionManager::GetRootCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const
{
	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		const TSharedRef<FCollection>& Collection = CachedCollection.Value;

		if (ShareType == ECollectionShareType::CST_All || ShareType == CollectionKey.Type)
		{
			// A root collection either has no parent GUID, or a parent GUID that cannot currently be found - the check below handles both
			if (!CachedCollectionNamesFromGuids.Contains(Collection->GetParentCollectionGuid()))
			{
				CollectionNames.AddUnique(CollectionKey.Name);
			}
		}
	}
}

void FCollectionManager::GetChildCollections(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FCollectionNameType>& OutCollections) const
{
	auto GetChildCollectionsInternal = [this, &OutCollections](const FCollectionNameType& InCollectionKey)
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			const auto* ChildCollectionGuids = CachedHierarchy.Find((*CollectionRefPtr)->GetCollectionGuid());
			if (ChildCollectionGuids)
			{
				for (const FGuid& ChildCollectionGuid : *ChildCollectionGuids)
				{
					const FCollectionNameType* const ChildCollectionKeyPtr = CachedCollectionNamesFromGuids.Find(ChildCollectionGuid);
					if (ChildCollectionKeyPtr)
					{
						OutCollections.Add(*ChildCollectionKeyPtr);
					}
				}
			}
		}
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find children in the specified collection name in any cache
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			GetChildCollectionsInternal(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)));
		}
	}
	else
	{
		GetChildCollectionsInternal(FCollectionNameType(CollectionName, ShareType));
	}
}

void FCollectionManager::GetChildCollectionNames(FName CollectionName, ECollectionShareType::Type ShareType, ECollectionShareType::Type ChildShareType, TArray<FName>& CollectionNames) const
{
	auto GetChildCollectionsInternal = [this, &ChildShareType, &CollectionNames](const FCollectionNameType& InCollectionKey)
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			const auto* ChildCollectionGuids = CachedHierarchy.Find((*CollectionRefPtr)->GetCollectionGuid());
			if (ChildCollectionGuids)
			{
				for (const FGuid& ChildCollectionGuid : *ChildCollectionGuids)
				{
					const FCollectionNameType* const ChildCollectionKeyPtr = CachedCollectionNamesFromGuids.Find(ChildCollectionGuid);
					if (ChildCollectionKeyPtr && (ChildShareType == ECollectionShareType::CST_All || ChildShareType == ChildCollectionKeyPtr->Type))
					{
						CollectionNames.AddUnique(ChildCollectionKeyPtr->Name);
					}
				}
			}
		}
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find children in the specified collection name in any cache
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			GetChildCollectionsInternal(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)));
		}
	}
	else
	{
		GetChildCollectionsInternal(FCollectionNameType(CollectionName, ShareType));
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

bool FCollectionManager::GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& AssetsPaths, ECollectionRecursionFlags::Flags RecursionMode) const
{
	bool bFoundAssets = false;

	auto GetAssetsInCollectionWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetAssetsInCollection(AssetsPaths);
			bFoundAssets = true;
		}
		return ERecursiveWorkerFlowControl::Continue;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find assets in the specified collection name in any cache
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			RecursionHelper_DoWork(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)), RecursionMode, GetAssetsInCollectionWorker);
		}
	}
	else
	{
		RecursionHelper_DoWork(FCollectionNameType(CollectionName, ShareType), RecursionMode, GetAssetsInCollectionWorker);
	}

	return bFoundAssets;
}

bool FCollectionManager::GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths, ECollectionRecursionFlags::Flags RecursionMode) const
{
	bool bFoundClasses = false;

	auto GetClassesInCollectionWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetClassesInCollection(ClassPaths);
			bFoundClasses = true;
		}
		return ERecursiveWorkerFlowControl::Continue;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find classes in the specified collection name in any cache
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			RecursionHelper_DoWork(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)), RecursionMode, GetClassesInCollectionWorker);
		}
	}
	else
	{
		RecursionHelper_DoWork(FCollectionNameType(CollectionName, ShareType), RecursionMode, GetClassesInCollectionWorker);
	}

	return bFoundClasses;
}

bool FCollectionManager::GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths, ECollectionRecursionFlags::Flags RecursionMode) const
{
	bool bFoundObjects = false;

	auto GetObjectsInCollectionWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			(*CollectionRefPtr)->GetObjectsInCollection(ObjectPaths);
			bFoundObjects = true;
		}
		return ERecursiveWorkerFlowControl::Continue;
	};

	if (ShareType == ECollectionShareType::CST_All)
	{
		// Asked for all share types, find classes in the specified collection name in any cache
		for (int32 CacheIdx = 0; CacheIdx < ECollectionShareType::CST_All; ++CacheIdx)
		{
			RecursionHelper_DoWork(FCollectionNameType(CollectionName, ECollectionShareType::Type(CacheIdx)), RecursionMode, GetObjectsInCollectionWorker);
		}
	}
	else
	{
		RecursionHelper_DoWork(FCollectionNameType(CollectionName, ShareType), RecursionMode, GetObjectsInCollectionWorker);
	}

	return bFoundObjects;
}

void FCollectionManager::GetCollectionsContainingObject(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames, ECollectionRecursionFlags::Flags RecursionMode) const
{
	const auto* ObjectCollectionInfosPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionInfosPtr)
	{
		for (const FObjectCollectionInfo& ObjectCollectionInfo : *ObjectCollectionInfosPtr)
		{
			if ((ShareType == ECollectionShareType::CST_All || ShareType == ObjectCollectionInfo.CollectionKey.Type) && (RecursionMode & ObjectCollectionInfo.Reason) != 0)
			{
				OutCollectionNames.Add(ObjectCollectionInfo.CollectionKey.Name);
			}
		}
	}
}

void FCollectionManager::GetCollectionsContainingObject(FName ObjectPath, TArray<FCollectionNameType>& OutCollections, ECollectionRecursionFlags::Flags RecursionMode) const
{
	const auto* ObjectCollectionInfosPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionInfosPtr)
	{
		OutCollections.Reserve(OutCollections.Num() + ObjectCollectionInfosPtr->Num());
		for (const FObjectCollectionInfo& ObjectCollectionInfo : *ObjectCollectionInfosPtr)
		{
			if ((RecursionMode & ObjectCollectionInfo.Reason) != 0)
			{
				OutCollections.Add(ObjectCollectionInfo.CollectionKey);
			}
		}
	}
}

void FCollectionManager::GetCollectionsContainingObjects(const TArray<FName>& ObjectPaths, TMap<FCollectionNameType, TArray<FName>>& OutCollectionsAndMatchedObjects, ECollectionRecursionFlags::Flags RecursionMode) const
{
	for (const FName& ObjectPath : ObjectPaths)
	{
		const auto* ObjectCollectionInfosPtr = CachedObjects.Find(ObjectPath);
		if (ObjectCollectionInfosPtr)
		{
			for (const FObjectCollectionInfo& ObjectCollectionInfo : *ObjectCollectionInfosPtr)
			{
				if ((RecursionMode & ObjectCollectionInfo.Reason) != 0)
				{
					TArray<FName>& MatchedObjects = OutCollectionsAndMatchedObjects.FindOrAdd(ObjectCollectionInfo.CollectionKey);
					MatchedObjects.Add(ObjectPath);
				}
			}
		}
	}
}

FString FCollectionManager::GetCollectionsStringForObject(FName ObjectPath, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode) const
{
	const auto* ObjectCollectionInfosPtr = CachedObjects.Find(ObjectPath);
	if (ObjectCollectionInfosPtr)
	{
		TArray<FString> CollectionNameStrings;

		for (const FObjectCollectionInfo& ObjectCollectionInfo : *ObjectCollectionInfosPtr)
		{
			if ((ShareType == ECollectionShareType::CST_All || ShareType == ObjectCollectionInfo.CollectionKey.Type) && (RecursionMode & ObjectCollectionInfo.Reason) != 0)
			{
				CollectionNameStrings.Add(ObjectCollectionInfo.CollectionKey.Name.ToString());
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
	const bool bUseSCC = ShouldUseSCC(ShareType);
	const FString CollectionFilename = GetCollectionFilename(CollectionName, ShareType);

	TSharedRef<FCollection> NewCollection = MakeShareable(new FCollection(CollectionFilename, bUseSCC));
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

	const FCollectionNameType OriginalCollectionKey(CurrentCollectionName, CurrentShareType);
	const FCollectionNameType NewCollectionKey(NewCollectionName, NewShareType);

	TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(OriginalCollectionKey);
	if (!CollectionRefPtr)
	{
		// The collection doesn't exist
		LastError = LOCTEXT("Error_DoesntExist", "The collection doesn't exist.");
		return false;
	}

	// Add the new collection
	TSharedPtr<FCollection> NewCollection;
	{
		const bool bUseSCC = ShouldUseSCC(NewShareType);
		const FString NewCollectionFilename = GetCollectionFilename(NewCollectionName, NewShareType);

		// Create an exact copy of the collection using its new path - this will preserve its GUID and avoid losing hierarchy data
		NewCollection = (*CollectionRefPtr)->Clone(NewCollectionFilename, bUseSCC, ECollectionCloneMode::Exact);
		if (!AddCollection(NewCollection.ToSharedRef(), NewShareType))
		{
			// Failed to add the collection, it already exists
			LastError = LOCTEXT("Error_AlreadyExists", "The collection already exists.");
			return false;
		}

		if (!NewCollection->Save(LastError))
		{
			// Collection failed to save, remove it from the cache
			RemoveCollection(NewCollection.ToSharedRef(), NewShareType);

			// Restore the original GUID mapping since AddCollection replaced it
			CachedCollectionNamesFromGuids.Add((*CollectionRefPtr)->GetCollectionGuid(), OriginalCollectionKey);

			return false;
		}
	}

	// Remove the old collection
	{
		if ((*CollectionRefPtr)->DeleteSourceFile(LastError))
		{
			RemoveCollection(*CollectionRefPtr, CurrentShareType);

			// Re-add the new GUID mapping since RemoveCollection removed it
			CachedCollectionNamesFromGuids.Add(NewCollection->GetCollectionGuid(), NewCollectionKey);
		}
		else
		{
			// Failed to remove the old collection, so remove the collection we created.
			NewCollection->DeleteSourceFile(LastError);
			RemoveCollection(NewCollection.ToSharedRef(), NewShareType);

			// Restore the original GUID mapping since RemoveCollection removed it
			CachedCollectionNamesFromGuids.Add((*CollectionRefPtr)->GetCollectionGuid(), OriginalCollectionKey);

			return false;
		}
	}

	RebuildCachedObjects();

	// Success
	CollectionRenamedEvent.Broadcast(OriginalCollectionKey, NewCollectionKey);
	return true;
}

bool FCollectionManager::ReparentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType)
{
	if (!ensure(ShareType < ECollectionShareType::CST_All) || (!ParentCollectionName.IsNone() && !ensure(ParentShareType < ECollectionShareType::CST_All)))
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

	const FGuid OldParentGuid = (*CollectionRefPtr)->GetParentCollectionGuid();
	FGuid NewParentGuid;

	TOptional<FCollectionNameType> OldParentCollectionKey;
	TOptional<FCollectionNameType> NewParentCollectionKey;

	if (!ParentCollectionName.IsNone())
	{
		// Find and set the new parent GUID
		NewParentCollectionKey = FCollectionNameType(ParentCollectionName, ParentShareType);
		TSharedRef<FCollection>* const ParentCollectionRefPtr = CachedCollections.Find(NewParentCollectionKey.GetValue());
		if (!ParentCollectionRefPtr)
		{
			// The collection doesn't exist
			LastError = LOCTEXT("Error_DoesntExist", "The collection doesn't exist.");
			return false;
		}

		// Does the parent collection need saving in order to have a stable GUID?
		if ((*ParentCollectionRefPtr)->GetCollectionVersion() < ECollectionVersion::AddedCollectionGuid)
		{
			// Try and re-save the parent collection now
			if (!(*ParentCollectionRefPtr)->Save(LastError))
			{
				return false;
			}
		}

		if (!IsValidParentCollection(CollectionName, ShareType, ParentCollectionName, ParentShareType))
		{
			// IsValidParentCollection fills in LastError itself
			return false;
		}

		NewParentGuid = (*ParentCollectionRefPtr)->GetCollectionGuid();
	}

	// Anything changed?
	if (OldParentGuid == NewParentGuid)
	{
		return true;
	}

	(*CollectionRefPtr)->SetParentCollectionGuid(NewParentGuid);

	// Try and save with the new parent GUID
	if (!(*CollectionRefPtr)->Save(LastError))
	{
		// Failed to save... rollback the collection to use its old parent GUID
		(*CollectionRefPtr)->SetParentCollectionGuid(OldParentGuid);
		return false;
	}

	// Find the old parent so we can notify about the change
	{
		const FCollectionNameType* const OldParentCollectionKeyPtr = CachedCollectionNamesFromGuids.Find(OldParentGuid);
		if (OldParentCollectionKeyPtr)
		{
			OldParentCollectionKey = *OldParentCollectionKeyPtr;
		}
	}

	RebuildCachedHierarchy();
	RebuildCachedObjects();

	// Success
	CollectionReparentedEvent.Broadcast(CollectionKey, OldParentCollectionKey, NewParentCollectionKey);
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

		RebuildCachedHierarchy();
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
		if ((*CollectionRefPtr)->AddObjectToCollection(ObjectPath))
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
				(*CollectionRefPtr)->RemoveObjectFromCollection(ObjectPath);
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
		if ((*CollectionRefPtr)->RemoveObjectFromCollection(ObjectPath))
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
			(*CollectionRefPtr)->AddObjectToCollection(RemovedAssetName);
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
	if (!GetObjectsInCollection(CollectionName, ShareType, ObjectPaths))
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

bool FCollectionManager::IsObjectInCollection(FName ObjectPath, FName CollectionName, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode) const
{
	if (!ensure(ShareType < ECollectionShareType::CST_All))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return true;
	}

	bool bFoundObject = false;

	auto IsObjectInCollectionWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
	{
		const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
		if (CollectionRefPtr)
		{
			bFoundObject = (*CollectionRefPtr)->IsObjectInCollection(ObjectPath);
		}
		return (bFoundObject) ? ERecursiveWorkerFlowControl::Stop : ERecursiveWorkerFlowControl::Continue;
	};

	RecursionHelper_DoWork(FCollectionNameType(CollectionName, ShareType), RecursionMode, IsObjectInCollectionWorker);

	return bFoundObject;
}

bool FCollectionManager::IsValidParentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType) const
{
	if (!ensure(ShareType < ECollectionShareType::CST_All) || (!ParentCollectionName.IsNone() && !ensure(ParentShareType < ECollectionShareType::CST_All)))
	{
		// Bad share type
		LastError = LOCTEXT("Error_Internal", "There was an internal error.");
		return true;
	}

	if (ParentCollectionName.IsNone())
	{
		// Clearing the parent is always valid
		return true;
	}

	bool bValidParent = true;

	auto IsValidParentCollectionWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
	{
		const bool bMatchesCollectionBeingReparented = (CollectionName == InCollectionKey.Name && ShareType == InCollectionKey.Type);
		if (bMatchesCollectionBeingReparented)
		{
			bValidParent = false;
			LastError = (InReason == ECollectionRecursionFlags::Self) 
				? LOCTEXT("InvalidParent_CannotParentToSelf", "A collection cannot be parented to itself") 
				: LOCTEXT("InvalidParent_CannotParentToChildren", "A collection cannot be parented to its children");
			return ERecursiveWorkerFlowControl::Stop;
		}
		return ERecursiveWorkerFlowControl::Continue;
	};

	RecursionHelper_DoWork(FCollectionNameType(ParentCollectionName, ParentShareType), ECollectionRecursionFlags::SelfAndParents, IsValidParentCollectionWorker);

	return bValidParent;
}

void FCollectionManager::HandleFixupRedirectors(ICollectionRedirectorFollower& InRedirectorFollower)
{
	const double LoadStartTime = FPlatformTime::Seconds();

	TArray<TPair<FName, FName>> ObjectsToRename;

	// Build up the list of redirected object into rename pairs
	for (const auto& CachedObjectInfo : CachedObjects)
	{
		FName NewObjectPath;
		if (InRedirectorFollower.FixupObject(CachedObjectInfo.Key, NewObjectPath))
		{
			ObjectsToRename.Add(TPairInitializer<FName, FName>(CachedObjectInfo.Key, NewObjectPath));
		}
	}

	// Notify the rename for each redirected object
	for (const auto& ObjectToRename : ObjectsToRename)
	{
		HandleObjectRenamed(ObjectToRename.Key, ObjectToRename.Value);
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Fixed up redirectors for %d collections in %0.6f seconds (updated %d objects)" ), CachedCollections.Num(), FPlatformTime::Seconds() - LoadStartTime, ObjectsToRename.Num());

	for (const auto& ObjectToRename : ObjectsToRename)
	{
		UE_LOG(LogCollectionManager, Verbose, TEXT( "\tRedirected '%s' to '%s'" ), *ObjectToRename.Key.ToString(), *ObjectToRename.Value.ToString());
	}
}

bool FCollectionManager::HandleRedirectorDeleted(const FName& ObjectPath)
{
	bool bSavedAllCollections = true;

	FTextBuilder AllErrors;

	// We don't have a cache for on-disk objects, so we have to do this the slower way and query each collection in turn
	for (const auto& CachedCollection : CachedCollections)
	{
		const TSharedRef<FCollection>& Collection = CachedCollection.Value;

		if (Collection->IsRedirectorInCollection(ObjectPath))
		{
			FText SaveError;
			if (!Collection->Save(SaveError))
			{
				AllErrors.AppendLine(SaveError);
				bSavedAllCollections = false;
			}
		}
	}

	if (!bSavedAllCollections)
	{
		LastError = AllErrors.ToText();
	}

	return bSavedAllCollections;
}

void FCollectionManager::HandleObjectRenamed(const FName& OldObjectPath, const FName& NewObjectPath)
{
	// Remove the info about the collections that the old object path is within - we'll add this info back in under the new object path shortly
	TArray<FObjectCollectionInfo> OldObjectCollectionInfos;
	if (!CachedObjects.RemoveAndCopyValue(OldObjectPath, OldObjectCollectionInfos))
	{
		return;
	}

	// Replace this object reference in all collections that use it, and update our object cache
	auto& NewObjectCollectionInfos = CachedObjects.FindOrAdd(NewObjectPath);
	for (const FObjectCollectionInfo& OldObjectCollectionInfo : OldObjectCollectionInfos)
	{
		if ((OldObjectCollectionInfo.Reason & ECollectionRecursionFlags::Self) != 0)
		{
			// The old object is contained directly within this collection (rather than coming from a parent or child collection), so update the object reference
			const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(OldObjectCollectionInfo.CollectionKey);
			if (CollectionRefPtr)
			{
				(*CollectionRefPtr)->RemoveObjectFromCollection(OldObjectPath);
				(*CollectionRefPtr)->AddObjectToCollection(NewObjectPath);
			}
		}

		// Merge the collection references for the old object with any collection references that already exist for the new object
		FObjectCollectionInfo* ObjectInfoPtr = NewObjectCollectionInfos.FindByPredicate([&](const FObjectCollectionInfo& InCollectionInfo) { return InCollectionInfo.CollectionKey == OldObjectCollectionInfo.CollectionKey; });
		if (ObjectInfoPtr)
		{
			ObjectInfoPtr->Reason |= OldObjectCollectionInfo.Reason;
		}
		else
		{
			NewObjectCollectionInfos.Add(OldObjectCollectionInfo);
		}
	}
}

void FCollectionManager::HandleObjectDeleted(const FName& ObjectPath)
{
	// Remove the info about the collections that the old object path is within
	TArray<FObjectCollectionInfo> ObjectCollectionInfos;
	if (!CachedObjects.RemoveAndCopyValue(ObjectPath, ObjectCollectionInfos))
	{
		return;
	}

	// Remove this object reference from all collections that use it
	for (const FObjectCollectionInfo& ObjectCollectionInfo : ObjectCollectionInfos)
	{
		if ((ObjectCollectionInfo.Reason & ECollectionRecursionFlags::Self) != 0)
		{
			// The object is contained directly within this collection (rather than coming from a parent or child collection), so remove the object reference
			const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(ObjectCollectionInfo.CollectionKey);
			if (CollectionRefPtr)
			{
				(*CollectionRefPtr)->RemoveObjectFromCollection(ObjectPath);
			}
		}
	}
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
			const bool bUseSCC = ShouldUseSCC(ECollectionShareType::Type(CacheIdx));

			FText LoadErrorText;
			TSharedRef<FCollection> NewCollection = MakeShareable(new FCollection(Filename, bUseSCC));
			if (NewCollection->Load(LoadErrorText))
			{
				AddCollection(NewCollection, ECollectionShareType::Type(CacheIdx));
			}
			else
			{
				UE_LOG(LogCollectionManager, Warning, TEXT("%s"), *LoadErrorText.ToString());
			}
		}
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Loaded %d collections in %0.6f seconds" ), CachedCollections.Num() - PrevNumCollections, FPlatformTime::Seconds() - LoadStartTime);

	RebuildCachedHierarchy();
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

		if (ObjectsInCollection.Num() > 0)
		{
			auto RebuildCachedObjectsWorker = [&](const FCollectionNameType& InCollectionKey, ECollectionRecursionFlags::Flag InReason) -> ERecursiveWorkerFlowControl
			{
				// The worker reason will tell us why this collection is being processed (eg, because it is a parent of the collection we told it to DoWork on),
				// however, the reason this object exists in that parent collection is because a child collection contains it, and this is the reason we need
				// to put into the FObjectCollectionInfo, since that's what we'll test against later when we do the "do my children contain this object"? test
				// That's why we flip the reason logic here...
				ECollectionRecursionFlags::Flag ReasonObjectInCollection = InReason;
				switch (InReason)
				{
				case ECollectionRecursionFlags::Parents:
					ReasonObjectInCollection = ECollectionRecursionFlags::Children;
					break;
				case ECollectionRecursionFlags::Children:
					ReasonObjectInCollection = ECollectionRecursionFlags::Parents;
					break;
				default:
					break;
				}

				for (const FName& ObjectPath : ObjectsInCollection)
				{
					auto& ObjectCollectionInfos = CachedObjects.FindOrAdd(ObjectPath);
					FObjectCollectionInfo* ObjectInfoPtr = ObjectCollectionInfos.FindByPredicate([&](const FObjectCollectionInfo& InCollectionInfo) { return InCollectionInfo.CollectionKey == InCollectionKey; });
					if (ObjectInfoPtr)
					{
						ObjectInfoPtr->Reason |= ReasonObjectInCollection;
					}
					else
					{
						ObjectCollectionInfos.Add(FObjectCollectionInfo(InCollectionKey, ReasonObjectInCollection));
					}
				}
				return ERecursiveWorkerFlowControl::Continue;
			};

			// Recursively process all collections so that they know they contain these objects (and why!)
			RecursionHelper_DoWork(CollectionKey, ECollectionRecursionFlags::All, RebuildCachedObjectsWorker);
		}
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Rebuilt the object cache for %d collections in %0.6f seconds (found %d objects)" ), CachedCollections.Num(), FPlatformTime::Seconds() - LoadStartTime, CachedObjects.Num());
}

void FCollectionManager::RebuildCachedHierarchy()
{
	const double LoadStartTime = FPlatformTime::Seconds();

	CachedHierarchy.Empty();

	for (const auto& CachedCollection : CachedCollections)
	{
		const FCollectionNameType& CollectionKey = CachedCollection.Key;
		const TSharedRef<FCollection>& Collection = CachedCollection.Value;

		// Make sure this is a known parent GUID before adding it to the map
		const FGuid& ParentCollectionGuid = Collection->GetParentCollectionGuid();
		if (CachedCollectionNamesFromGuids.Contains(ParentCollectionGuid))
		{
			auto& CollectionChildren = CachedHierarchy.FindOrAdd(ParentCollectionGuid);
			CollectionChildren.AddUnique(Collection->GetCollectionGuid());
		}
	}

	UE_LOG(LogCollectionManager, Log, TEXT( "Rebuilt the hierarchy cache for %d collections in %0.6f seconds" ), CachedCollections.Num(), FPlatformTime::Seconds() - LoadStartTime);
}

bool FCollectionManager::ShouldUseSCC(ECollectionShareType::Type ShareType) const
{
	return ShareType != ECollectionShareType::CST_Local && ShareType != ECollectionShareType::CST_System;
}

FString FCollectionManager::GetCollectionFilename(const FName& InCollectionName, const ECollectionShareType::Type InCollectionShareType) const
{
	FString CollectionFilename = CollectionFolders[InCollectionShareType] / InCollectionName.ToString() + TEXT(".") + CollectionExtension;
	FPaths::NormalizeFilename(CollectionFilename);
	return CollectionFilename;
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
	CachedCollectionNamesFromGuids.Add(CollectionRef->GetCollectionGuid(), CollectionKey);
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
	CachedCollectionNamesFromGuids.Remove(CollectionRef->GetCollectionGuid());
	return CachedCollections.Remove(CollectionKey) > 0;
}

void FCollectionManager::RecursionHelper_DoWork(const FCollectionNameType& InCollectionKey, const ECollectionRecursionFlags::Flags InRecursionMode, const FRecursiveWorkerFunc& InWorkerFunc) const
{
	if ((InRecursionMode & ECollectionRecursionFlags::Self) && InWorkerFunc(InCollectionKey, ECollectionRecursionFlags::Self) == ERecursiveWorkerFlowControl::Stop)
	{
		return;
	}

	if ((InRecursionMode & ECollectionRecursionFlags::Parents) && RecursionHelper_DoWorkOnParents(InCollectionKey, InWorkerFunc) == ERecursiveWorkerFlowControl::Stop)
	{
		return;
	}

	if ((InRecursionMode & ECollectionRecursionFlags::Children) && RecursionHelper_DoWorkOnChildren(InCollectionKey, InWorkerFunc) == ERecursiveWorkerFlowControl::Stop)
	{
		return;
	}
}

FCollectionManager::ERecursiveWorkerFlowControl FCollectionManager::RecursionHelper_DoWorkOnParents(const FCollectionNameType& InCollectionKey, const FRecursiveWorkerFunc& InWorkerFunc) const
{
	const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
	if (CollectionRefPtr)
	{
		const FCollectionNameType* const ParentCollectionKeyPtr = CachedCollectionNamesFromGuids.Find((*CollectionRefPtr)->GetParentCollectionGuid());
		if (ParentCollectionKeyPtr)
		{
			if (InWorkerFunc(*ParentCollectionKeyPtr, ECollectionRecursionFlags::Parents) == ERecursiveWorkerFlowControl::Stop || RecursionHelper_DoWorkOnParents(*ParentCollectionKeyPtr, InWorkerFunc) == ERecursiveWorkerFlowControl::Stop)
			{
				return ERecursiveWorkerFlowControl::Stop;
			}
		}
	}

	return ERecursiveWorkerFlowControl::Continue;
}

FCollectionManager::ERecursiveWorkerFlowControl FCollectionManager::RecursionHelper_DoWorkOnChildren(const FCollectionNameType& InCollectionKey, const FRecursiveWorkerFunc& InWorkerFunc) const
{
	const TSharedRef<FCollection>* const CollectionRefPtr = CachedCollections.Find(InCollectionKey);
	if (CollectionRefPtr)
	{
		const TArray<FGuid>* const ChildCollectionGuids = CachedHierarchy.Find((*CollectionRefPtr)->GetCollectionGuid());
		if (ChildCollectionGuids)
		{
			for (const FGuid& ChildCollectionGuid : *ChildCollectionGuids)
			{
				const FCollectionNameType* const ChildCollectionKeyPtr = CachedCollectionNamesFromGuids.Find(ChildCollectionGuid);
				if (ChildCollectionKeyPtr)
				{
					if (InWorkerFunc(*ChildCollectionKeyPtr, ECollectionRecursionFlags::Children) == ERecursiveWorkerFlowControl::Stop || RecursionHelper_DoWorkOnChildren(*ChildCollectionKeyPtr, InWorkerFunc) == ERecursiveWorkerFlowControl::Stop)
					{
						return ERecursiveWorkerFlowControl::Stop;
					}
				}
			}
		}
	}

	return ERecursiveWorkerFlowControl::Continue;
}

#undef LOCTEXT_NAMESPACE
