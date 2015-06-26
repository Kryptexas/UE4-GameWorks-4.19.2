// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCollectionManager : public ICollectionManager
{
public:
	FCollectionManager();
	virtual ~FCollectionManager();

	// ICollectionManager implementation
	virtual bool HasCollections() const override;
	virtual void GetCollections(TArray<FCollectionNameType>& OutCollections) const override;
	virtual void GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const override;
	virtual void GetRootCollections(TArray<FCollectionNameType>& OutCollections) const override;
	virtual void GetRootCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const override;
	virtual void GetChildCollections(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FCollectionNameType>& OutCollections) const override;
	virtual void GetChildCollectionNames(FName CollectionName, ECollectionShareType::Type ShareType, ECollectionShareType::Type ChildShareType, TArray<FName>& CollectionNames) const override;
	virtual bool CollectionExists(FName CollectionName, ECollectionShareType::Type ShareType) const override;
	virtual bool GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& AssetPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual bool GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual bool GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual void GetCollectionsContainingObject(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual void GetCollectionsContainingObject(FName ObjectPath, TArray<FCollectionNameType>& OutCollections, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual void GetCollectionsContainingObjects(const TArray<FName>& ObjectPaths, TMap<FCollectionNameType, TArray<FName>>& OutCollectionsAndMatchedObjects, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual FString GetCollectionsStringForObject(FName ObjectPath, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual void CreateUniqueCollectionName(const FName& BaseName, ECollectionShareType::Type ShareType, FName& OutCollectionName) const override;
	virtual bool CreateCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool RenameCollection(FName CurrentCollectionName, ECollectionShareType::Type CurrentShareType, FName NewCollectionName, ECollectionShareType::Type NewShareType) override;
	virtual bool ReparentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType) override;
	virtual bool DestroyCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) override;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumAdded = nullptr) override;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) override;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumRemoved = nullptr) override;
	virtual bool EmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool IsCollectionEmpty(FName CollectionName, ECollectionShareType::Type ShareType) const override;
	virtual bool IsObjectInCollection(FName ObjectPath, FName CollectionName, ECollectionShareType::Type ShareType, ECollectionRecursionFlags::Flags RecursionMode = ECollectionRecursionFlags::Self) const override;
	virtual bool IsValidParentCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ParentCollectionName, ECollectionShareType::Type ParentShareType) const override;
	virtual FText GetLastError() const override { return LastError; }
	virtual void HandleFixupRedirectors(ICollectionRedirectorFollower& InRedirectorFollower) override;
	virtual bool HandleRedirectorDeleted(const FName& ObjectPath) override;
	virtual void HandleObjectRenamed(const FName& OldObjectPath, const FName& NewObjectPath) override;
	virtual void HandleObjectDeleted(const FName& ObjectPath) override;

	/** Event for when collections are created */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionCreatedEvent, FCollectionCreatedEvent );
	virtual FCollectionCreatedEvent& OnCollectionCreated() override { return CollectionCreatedEvent; }

	/** Event for when collections are destroyed */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionDestroyedEvent, FCollectionDestroyedEvent );
	virtual FCollectionDestroyedEvent& OnCollectionDestroyed() override { return CollectionDestroyedEvent; }

	/** Event for when assets are added to a collection */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FAssetsAddedEvent, FAssetsAddedEvent ); 
	virtual FAssetsAddedEvent& OnAssetsAdded() override { return AssetsAddedEvent; }

	/** Event for when assets are removed to a collection */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FAssetsRemovedEvent, FAssetsRemovedEvent );
	virtual FAssetsRemovedEvent& OnAssetsRemoved() override { return AssetsRemovedEvent; }

	/** Event for when collections are renamed */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionRenamedEvent, FCollectionRenamedEvent );
	virtual FCollectionRenamedEvent& OnCollectionRenamed() override { return CollectionRenamedEvent; }

	/** Event for when collections are re-parented */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionReparentedEvent, FCollectionReparentedEvent );
	virtual FCollectionReparentedEvent& OnCollectionReparented() override { return CollectionReparentedEvent; }

private:
	/** Loads all collection files from disk */
	void LoadCollections();

	/** Rebuild the entire cached objects map based on the current collection data */
	void RebuildCachedObjects();

	/** Rebuild the entire cached hierarchy map based on the current collection data */
	void RebuildCachedHierarchy();

	/** Returns true if the specified share type requires source control */
	bool ShouldUseSCC(ECollectionShareType::Type ShareType) const;

	/** Given a collection name and share type, work out the full filename for the collection to use on disk */
	FString GetCollectionFilename(const FName& InCollectionName, const ECollectionShareType::Type InCollectionShareType) const;

	/** Adds a collection to the lookup maps */
	bool AddCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType);

	/** Removes a collection from the lookup maps */
	bool RemoveCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType);

	enum class ERecursiveWorkerFlowControl : uint8
	{
		Stop,
		Continue,
	};

	typedef TFunctionRef<ERecursiveWorkerFlowControl(const FCollectionNameType&, ECollectionRecursionFlags::Flag)> FRecursiveWorkerFunc;

	void RecursionHelper_DoWork(const FCollectionNameType& InCollectionKey, const ECollectionRecursionFlags::Flags InRecursionMode, const FRecursiveWorkerFunc& InWorkerFunc) const;
	ERecursiveWorkerFlowControl RecursionHelper_DoWorkOnParents(const FCollectionNameType& InCollectionKey, const FRecursiveWorkerFunc& InWorkerFunc) const;
	ERecursiveWorkerFlowControl RecursionHelper_DoWorkOnChildren(const FCollectionNameType& InCollectionKey, const FRecursiveWorkerFunc& InWorkerFunc) const;

private:
	/** Collection info for a given object - gives the collection name, as well as the reason this object is considered to be part of this collection */
	struct FObjectCollectionInfo
	{
		explicit FObjectCollectionInfo(const FCollectionNameType& InCollectionKey)
			: CollectionKey(InCollectionKey)
			, Reason(0)
		{
		}

		FObjectCollectionInfo(const FCollectionNameType& InCollectionKey, const ECollectionRecursionFlags::Flags InReason)
			: CollectionKey(InCollectionKey)
			, Reason(InReason)
		{
		}

		/** The key identifying the collection that contains this object */
		FCollectionNameType CollectionKey;
		/** The reason(s) why this collection contains this object - this can be tested against the recursion mode when getting the collections for an object */
		ECollectionRecursionFlags::Flags Reason;
	};

	/** The folders that contain collections */
	FString CollectionFolders[ECollectionShareType::CST_All];

	/** The extension used for collection files */
	FString CollectionExtension;

	/** A map of collection names to FCollection objects */
	TMap<FCollectionNameType, TSharedRef<FCollection>> CachedCollections;

	/** A map of collection GUIDs to their associated collection names */
	TMap<FGuid, FCollectionNameType> CachedCollectionNamesFromGuids;

	/** A map of object paths to their associated collection info - only objects that are in collections will appear in here */
	TMap<FName, TArray<FObjectCollectionInfo>> CachedObjects;

	/** A map of parent collection GUIDs to their child collection GUIDs - only collections that have children will appear in here */
	TMap<FGuid, TArray<FGuid>> CachedHierarchy;

	/** The most recent error that occurred */
	mutable FText LastError;

	/** Event for when assets are added to a collection */
	FAssetsAddedEvent AssetsAddedEvent;

	/** Event for when assets are removed from a collection */
	FAssetsRemovedEvent AssetsRemovedEvent;

	/** Event for when collections are renamed */
	FCollectionRenamedEvent CollectionRenamedEvent;

	/** Event for when collections are re-parented */
	FCollectionReparentedEvent CollectionReparentedEvent;

	/** Event for when collections are created */
	FCollectionCreatedEvent CollectionCreatedEvent;

	/** Event for when collections are destroyed */
	FCollectionDestroyedEvent CollectionDestroyedEvent;
};
