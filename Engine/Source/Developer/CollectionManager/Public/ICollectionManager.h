// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CollectionManagerTypes.h"

class ICollectionManager
{
public:
	/** Virtual destructor */
	virtual ~ICollectionManager() {}

	/** Returns whether or not the collection manager contains any collections */
	virtual bool HasCollections() const = 0;

	/** Returns the list of collections */
	virtual void GetCollections(TArray<FCollectionNameType>& OutCollections) const = 0;

	/** Returns the list of collection names of the specified share type */
	virtual void GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const = 0;

	/** Returns the list of root-level collections */
	virtual void GetRootCollections(TArray<FCollectionNameType>& OutCollections) const = 0;

	/** Returns the list of root-level collection names of the specified share type */
	virtual void GetRootCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const = 0;

	/** Returns the list of child collections of the given collection */
	virtual void GetChildCollections(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FCollectionNameType>& OutCollections) const = 0;

	/** Returns the list of child collections of the given collection that are also of the specified share type */
	virtual void GetChildCollectionNames(FName CollectionName, ECollectionShareType::Type ShareType, ECollectionShareType::Type ChildShareType, TArray<FName>& CollectionNames) const = 0;

	/** Returns true if the collection exists */
	virtual bool CollectionExists(FName CollectionName, ECollectionShareType::Type ShareType) const = 0;

	/** Returns a list of asset paths found in the specified collection and share type */
	virtual bool GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& OutAssetPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a list of class paths found in the specified collection and share type */
	virtual bool GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a list of object paths found in the specified collection and share type */
	virtual bool GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a list of collections in which the specified object exists of the specified share type */
	virtual void GetCollectionsContainingObject(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a list of collections in which the specified object exists */
	virtual void GetCollectionsContainingObject(FName ObjectPath, TArray<FCollectionNameType>& OutCollections, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a list of collections in which any of the specified objects exist */
	virtual void GetCollectionsContainingObjects(const TArray<FName>& ObjectPaths, TMap<FCollectionNameType, TArray<FName>>& OutCollectionsAndMatchedObjects, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Returns a string containing a comma separated list of collections in which the specified object exists of the specified share type */
	virtual FString GetCollectionsStringForObject(FName ObjectPath, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/** Creates a unique collection name for the given type taking the form BaseName+(unique number) */
	virtual void CreateUniqueCollectionName(const FName& BaseName, ECollectionShareType::Type ShareType, FName& OutCollectionName) const = 0;

	/**
	  * Adds a collection to the asset registry. A .collection file will be added to disk.
	  *
	  * @param CollectionName The name of the new collection
	  * @param ShareType The way the collection is shared.
	  * @return true if the add was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool CreateCollection(FName CollectionName, ECollectionShareType::Type ShareType) = 0;

	/**
	  * Renames a collection. A .collection file will be added to disk and a .collection file will be removed.
	  *
	  * @param CurrentCollectionName The current name of the collection.
	  * @param CurrentShareType The current way the collection is shared.
	  * @param NewCollectionName The new name of the collection.
	  * @param NewShareType The new way the collection is shared.
	  * @return true if the rename was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool RenameCollection(FName CurrentCollectionName, ECollectionShareType::Type CurrentShareType, FName NewCollectionName, ECollectionShareType::Type NewShareType) = 0;

	/**
	  * Re-parents a collection. The parent collection may be re-saved if it's too old to have a stable GUID.
	  *
	  * @param CollectionName The name of the collection to set the parent of.
	  * @param ShareType The way the collection is shared.
	  * @param ParentCollectionName The new parent of the collection, or None to have the collection become a root collection.
	  * @param ParentShareType The way the new parent collection is shared.
	  * @return true if the re-parent was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool ReparentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType) = 0;

	/**
	  * Removes a collection to the asset registry. A .collection file will be deleted from disk.
	  *
	  * @param CollectionName The name of the collection to remove.
	  * @param ShareType The way the collection is shared.
	  * @return true if the remove was successful
	  */
	virtual bool DestroyCollection(FName CollectionName, ECollectionShareType::Type ShareType) = 0;

	/**
	  * Adds an asset to the specified collection.
	  *
	  * @param CollectionName The collection in which to add the asset
	  * @param ShareType The way the collection is shared.
	  * @param ObjectPath the ObjectPath of the asset to add.
	  * @param OutNumAdded if non-NULL, the number of objects successfully added to the collection
	  * @return true if the add was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) = 0;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumAdded = nullptr) = 0;

	/**
	  * Removes the asset from the specified collection.
	  *
	  * @param CollectionName The collection from which to remove the asset
	  * @param ShareType The way the collection is shared.
	  * @param ObjectPath the ObjectPath of the asset to remove.
	  * @param OutNumRemoved if non-NULL, the number of objects successfully removed from the collection
	  * @return true if the remove was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) = 0;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumRemoved = nullptr) = 0;

	/**
	  * Removes all assets from the specified collection.
	  *
	  * @param CollectionName The collection from which to remove the asset
	  * @param ShareType The way the collection is shared.
	  * @return true if the remove was successful. If false, GetLastError will return a human readable string description of the error.
	  */
	virtual bool EmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType) = 0;

	/**
	  * @param CollectionName The collection from which to remove the asset
	  * @param ShareType The way the collection is shared.
	  * @return true if the collection is empty.
	  */
	virtual bool IsCollectionEmpty(FName CollectionName, ECollectionShareType::Type ShareType) const = 0;

	/**
	 * Check to see if the given object exists in the given collection
	 *
	 * @param ObjectPath The object to test for its existence in the collection.
	 * @param CollectionName The collection to test.
	 * @param ShareType The way the collection is shared.
	 * @return true if the object is in the collection.
	 */
	virtual bool IsObjectInCollection(FName ObjectPath, FName CollectionName, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const = 0;

	/**
	  * Check to see if the given collection is valid to be used as the parent of another collection. A collection may not be parented to itself, nor any of its current children.
	  *
	  * @param CollectionName The name of the collection to check the parent of.
	  * @param ShareType The way the collection is shared.
	  * @param ParentCollectionName The name of the parent collection to test against.
	  * @param ParentShareType The way the new parent collection is shared.
	  * @return true if the parent is considered valid for this collection. If false, GetLastError will return a human readable string describing why the parent is invalid.
	  */
	virtual bool IsValidParentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType) const = 0;

	/** Returns the most recent error. */
	virtual FText GetLastError() const = 0;

	/**
	 * Called to notify the collections that they should fix-up their object references so that they no longer contain any redirectors
	 * References are only updated in-memory, and won't be saved to disk until a redirector is deleted (which forces our hand), or the collection is saved for any other reason
	 */
	virtual void HandleFixupRedirectors(ICollectionRedirectorFollower& InRedirectorFollower) = 0;

	/**
	 * Called to notify the collections that a redirector has been deleted and that they should ensure their on-disk representation is re-saved with the fixed up in-memory version
	 * @return true if all of the collections that were referencing this redirector could be re-saved, false otherwise
	 */
	virtual bool HandleRedirectorDeleted(const FName& ObjectPath) = 0;

	/** Called to notify the collections that an object has been renamed or moved */
	virtual void HandleObjectRenamed(const FName& OldObjectPath, const FName& NewObjectPath) = 0;

	/** Called to notify the collections that an object has been deleted */
	virtual void HandleObjectDeleted(const FName& ObjectPath) = 0;

	/** Event for when collections are created */
	DECLARE_EVENT_OneParam( ICollectionManager, FCollectionCreatedEvent, const FCollectionNameType& );
	virtual FCollectionCreatedEvent& OnCollectionCreated() = 0;

	/** Event for when collections are destroyed */
	DECLARE_EVENT_OneParam( ICollectionManager, FCollectionDestroyedEvent, const FCollectionNameType& );
	virtual FCollectionDestroyedEvent& OnCollectionDestroyed() = 0;

	/** Event for when assets are added to a collection */
	DECLARE_EVENT_TwoParams( ICollectionManager, FAssetsAddedEvent, const FCollectionNameType&, const TArray<FName>& );
	virtual FAssetsAddedEvent& OnAssetsAdded() = 0;

	/** Event for when assets are removed from a collection */
	DECLARE_EVENT_TwoParams( ICollectionManager, FAssetsRemovedEvent, const FCollectionNameType&, const TArray<FName>& );
	virtual FAssetsRemovedEvent& OnAssetsRemoved() = 0;

	/** Event for when collections are renamed */
	DECLARE_EVENT_TwoParams( ICollectionManager, FCollectionRenamedEvent, const FCollectionNameType&, const FCollectionNameType& );
	virtual FCollectionRenamedEvent& OnCollectionRenamed() = 0;

	/** Event for when collections are re-parented (params: Collection, OldParent, NewParent) */
	DECLARE_EVENT_ThreeParams( ICollectionManager, FCollectionReparentedEvent, const FCollectionNameType&, const TOptional<FCollectionNameType>&, const TOptional<FCollectionNameType>& );
	virtual FCollectionReparentedEvent& OnCollectionReparented() = 0;
};
