// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorActorFolders.generated.h"

/** Multicast delegates for broadcasting various folder events */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActorFolderCreate, const UWorld&, FName);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnActorFolderMove, const UWorld&, FName, FName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActorFolderDelete, const UWorld&, FName);

/** Actor Folder UObject. This is used to support undo/redo reliably */
UCLASS()
class UEditorActorFolders : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FName> Paths;
};

/** Class responsible for managing an in-memory representation of actor folders in the editor */
struct UNREALED_API FActorFolders : public FGCObject
{
	FActorFolders();
	~FActorFolders();

	// FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;
	// End FGCObject Interface

	/** Singleton access */
	static FActorFolders& Get();

	/** Initialize the singleton instance - called on Editor Startup */
	static void Init();

	/** Clean up the singleton instance - called on Editor Exit */
	static void Cleanup();

	/** Folder creation and deletion events. Called whenever a folder is created or deleted in a world. */
	static FOnActorFolderCreate OnFolderCreate;
	static FOnActorFolderDelete OnFolderDelete;

	/** Check if the specified path is a child of the specified parent */
	static bool PathIsChildOf(const FString& InPotentialChild, const FString& InParent);

	/** Get an array of folders for the specified world */
	const TArray<FName>& GetFoldersForWorld(const UWorld& InWorld);

	/** Get a default folder name under the specified parent path */
	FName GetDefaultFolderName(UWorld& InWorld, FName ParentPath = FName());
	
	/** Get a new default folder name that would apply to the current selection */
	FName GetDefaultFolderNameForSelection(UWorld& InWorld);

	/** Create a new folder in the specified world, of the specified path */
	void CreateFolder(UWorld& InWorld, FName Path);

	/** Same as CreateFolder, but moves the current actor selection into the new folder as well */
	void CreateFolderContainingSelection(UWorld& InWorld, FName Path);

	/** Delete the specified folder in the world */
	void DeleteFolder(const UWorld& InWorld, FName FolderToDelete);

	/** Rename the specified path to a new name */
	void RenameFolderInWorld(UWorld& World, FName OldPath, FName NewPath);

private:

	/** Internal (non-const) implementation of GetFoldersForWorld */
	UEditorActorFolders& GetFoldersForWorld_Internal(const UWorld& InWorld);

	/** Rebuild the folder list for the specified world. This can be very slow as it
		iterates all actors in memory to rebuild the array of actors for this world */
	void RebuildFolderListForWorld(UWorld& InWorld);

	/** Called when an actor's folder has changed */
	void OnActorFolderChanged(const AActor* InActor, FName OldPath);

	/** Called when the actor list of the current world has changed */
	void OnLevelActorListChanged();

	/** Called when the global map in the editor has changed */
	void OnMapChange(uint32 MapChangeFlags);

	/** Called when an actor is added to the world */
	void OnLevelActorAdded(AActor* Actor);

	/** Remove any references to folder arrays for dead worlds */
	void Housekeeping();

	/** Scan the specified world for folders. This can be very slow as it iterates
		all actors in memory to rebuild the array of actors for this world */
	TSet<FName> ScanWorldForFolders(UWorld& InWorld);

	/** Transient map of folders, keyed on world pointer */
	TMap<TWeakObjectPtr<UWorld>, UEditorActorFolders*> TemporaryWorldFolders;

	/** Singleton instance maintained by the editor */
	static FActorFolders* Singleton;
};
