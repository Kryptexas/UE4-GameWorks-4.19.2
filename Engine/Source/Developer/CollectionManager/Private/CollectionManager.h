// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCollectionManager : public ICollectionManager
{
public:
	FCollectionManager();
	virtual ~FCollectionManager();

	// ICollectionManager implementation
	virtual bool HasCollections() const override;
	virtual void GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const override;
	virtual bool CollectionExists(FName CollectionName, ECollectionShareType::Type ShareType) const override;
	virtual bool GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& AssetPaths) const override;
	virtual bool GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths) const override;
	virtual bool GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths) const override;
	virtual void GetCollectionsContainingObject(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames) const override;
	virtual void GetCollectionsContainingObject(FName ObjectPath, TArray<FCollectionNameType>& OutCollections) const override;
	virtual void GetCollectionsContainingObjects(const TArray<FName>& ObjectPaths, TMap<FCollectionNameType, TArray<FName>>& OutCollectionsAndMatchedObjects) const override;
	virtual FString GetCollectionsStringForObject(FName ObjectPath, ECollectionShareType::Type ShareType) const override;
	virtual void CreateUniqueCollectionName(const FName& BaseName, ECollectionShareType::Type ShareType, FName& OutCollectionName) const override;
	virtual bool CreateCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool RenameCollection(FName CurrentCollectionName, ECollectionShareType::Type CurrentShareType, FName NewCollectionName, ECollectionShareType::Type NewShareType) override;
	virtual bool DestroyCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) override;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumAdded = nullptr) override;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) override;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumRemoved = nullptr) override;
	virtual bool EmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType) override;
	virtual bool IsCollectionEmpty(FName CollectionName, ECollectionShareType::Type ShareType) const override;
	virtual bool IsObjectInCollection(FName ObjectPath, FName CollectionName, ECollectionShareType::Type ShareType) const override;
	virtual FText GetLastError() const override { return LastError; }

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

private:
	/** Loads all collection files from disk */
	void LoadCollections();

	/** Rebuild the entire cached objects map based on the current collection data */
	void RebuildCachedObjects();

	/** Returns true if the specified share type requires source control */
	bool ShouldUseSCC(ECollectionShareType::Type ShareType) const;

	/** Adds a collection to the lookup maps */
	bool AddCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType);

	/** Removes a collection from the lookup maps */
	bool RemoveCollection(const TSharedRef<FCollection>& CollectionRef, ECollectionShareType::Type ShareType);

private:
	/** The folders that contain collections */
	FString CollectionFolders[ECollectionShareType::CST_All];

	/** The extension used for collection files */
	FString CollectionExtension;

	/** A map of collection names to FCollection objects */
	TMap<FCollectionNameType, TSharedRef<FCollection>> CachedCollections;

	/** A map of object paths to their associated collection names */
	TMap<FName, TArray<FCollectionNameType>> CachedObjects;

	/** The most recent error that occurred */
	mutable FText LastError;

	/** Event for when assets are added to a collection */
	FAssetsAddedEvent AssetsAddedEvent;

	/** Event for when assets are removed from a collection */
	FAssetsRemovedEvent AssetsRemovedEvent;

	/** Event for when collections are renamed */
	FCollectionRenamedEvent CollectionRenamedEvent;

	/** Event for when collections are created */
	FCollectionCreatedEvent CollectionCreatedEvent;

	/** Event for when collections are destroyed */
	FCollectionDestroyedEvent CollectionDestroyedEvent;
};
