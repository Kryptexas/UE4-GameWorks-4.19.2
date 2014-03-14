// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCollectionManager : public ICollectionManager
{
public:
	FCollectionManager();
	virtual ~FCollectionManager();

	// ICollectionManager implementation
	virtual void GetCollectionNames(ECollectionShareType::Type ShareType, TArray<FName>& CollectionNames) const OVERRIDE;
	virtual bool CollectionExists(FName CollectionName, ECollectionShareType::Type ShareType) const OVERRIDE;
	virtual bool GetAssetsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& AssetPaths) const OVERRIDE;
	virtual bool GetObjectsInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ObjectPaths) const OVERRIDE;
	virtual bool GetClassesInCollection(FName CollectionName, ECollectionShareType::Type ShareType, TArray<FName>& ClassPaths) const OVERRIDE;
	virtual void GetCollectionsContainingAsset(FName ObjectPath, ECollectionShareType::Type ShareType, TArray<FName>& OutCollectionNames) const OVERRIDE;
	virtual void CreateUniqueCollectionName(const FName& BaseName, ECollectionShareType::Type ShareType, FName& OutCollectionName) const OVERRIDE;
	virtual bool CreateCollection(FName CollectionName, ECollectionShareType::Type ShareType) OVERRIDE;
	virtual bool RenameCollection(FName CurrentCollectionName, ECollectionShareType::Type CurrentShareType, FName NewCollectionName, ECollectionShareType::Type NewShareType) OVERRIDE;
	virtual bool DestroyCollection(FName CollectionName, ECollectionShareType::Type ShareType) OVERRIDE;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) OVERRIDE;
	virtual bool AddToCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumAdded = NULL) OVERRIDE;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, FName ObjectPath) OVERRIDE;
	virtual bool RemoveFromCollection(FName CollectionName, ECollectionShareType::Type ShareType, const TArray<FName>& ObjectPaths, int32* OutNumRemoved = NULL) OVERRIDE;
	virtual bool EmptyCollection(FName CollectionName, ECollectionShareType::Type ShareType) OVERRIDE;
	virtual bool IsCollectionEmpty(FName CollectionName, ECollectionShareType::Type ShareType) OVERRIDE;
	virtual FText GetLastError() const OVERRIDE { return LastError; }

	/** Event for when collections are created */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionCreatedEvent, FCollectionCreatedEvent );
	virtual FCollectionCreatedEvent& OnCollectionCreated() OVERRIDE { return CollectionCreatedEvent; }

	/** Event for when collections are destroyed */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionDestroyedEvent, FCollectionDestroyedEvent );
	virtual FCollectionDestroyedEvent& OnCollectionDestroyed() OVERRIDE { return CollectionDestroyedEvent; }

	/** Event for when assets are added to a collection */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FAssetsAddedEvent, FAssetsAddedEvent ); 
	virtual FAssetsAddedEvent& OnAssetsAdded() OVERRIDE { return AssetsAddedEvent; }

	/** Event for when assets are removed to a collection */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FAssetsRemovedEvent, FAssetsRemovedEvent );
	virtual FAssetsRemovedEvent& OnAssetsRemoved() OVERRIDE { return AssetsRemovedEvent; }

	/** Event for when collections are renamed */
	DECLARE_DERIVED_EVENT( FCollectionManager, ICollectionManager::FCollectionRenamedEvent, FCollectionRenamedEvent );
	virtual FCollectionRenamedEvent& OnCollectionRenamed() OVERRIDE { return CollectionRenamedEvent; }

private:
	/** Loads all collection files from disk */
	void LoadCollections();

	/** Returns true if the specified share type requires source control */
	bool ShouldUseSCC(ECollectionShareType::Type ShareType) const;

	/** Adds a collection to the lookup maps */
	class FCollection* AddCollection(const FCollection& Collection, ECollectionShareType::Type ShareType);

	/** Removes a collection from the lookup maps */
	bool RemoveCollection(FCollection* Collection, ECollectionShareType::Type ShareType);

private:
	/** The folders that contain collections */
	FString CollectionFolders[ECollectionShareType::CST_All];

	/** The extension used for collection files */
	FString CollectionExtension;

	/** A map of collection names to FCollection objects */
	TMap<FName, FCollection*> CachedCollections[ECollectionShareType::CST_All];

	/** A count of the number of collections created so we can be sure they are all destroyed when we shut down */
	int32 NumCollections;

	/** The most recent error that occurred */
	FText LastError;

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