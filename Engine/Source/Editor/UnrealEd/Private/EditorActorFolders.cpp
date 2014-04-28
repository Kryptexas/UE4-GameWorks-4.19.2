// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EditorActorFolders.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FActorFolders"

UEditorActorFolders::UEditorActorFolders(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{ }

/** Convert an old path to a new path, replacing an ancestor branch with something else */
FName OldPathToNewPath(const FString& InOldBranch, const FString& InNewBranch, const FString& PathToMove)
{
	return FName(*(InNewBranch + PathToMove.RightChop(InOldBranch.Len())));
}

// Static member definitions
FOnActorFolderCreate	FActorFolders::OnFolderCreate;
FOnActorFolderDelete	FActorFolders::OnFolderDelete;
FActorFolders*			FActorFolders::Singleton;

FActorFolders::FActorFolders()
{
	check(GEngine);
	GEngine->OnLevelActorFolderChanged().AddRaw(this, &FActorFolders::OnActorFolderChanged);
	GEngine->OnLevelActorListChanged().AddRaw( this, &FActorFolders::OnLevelActorListChanged );
	GEngine->OnLevelActorAdded().AddRaw(this, &FActorFolders::OnLevelActorAdded);
	FEditorDelegates::MapChange.AddRaw( this, &FActorFolders::OnMapChange );

	UWorld* World = GWorld;
	if (World)
	{
		RebuildFolderListForWorld(*World);
	}
}

FActorFolders::~FActorFolders()
{
	check(GEngine);
	GEngine->OnLevelActorFolderChanged().RemoveAll(this);
	GEngine->OnLevelActorListChanged().RemoveAll(this);
	FEditorDelegates::MapChange.RemoveAll(this);
}

void FActorFolders::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Add references for all our UObjects so they don't get collected
	for (auto Pair : TemporaryWorldFolders)
	{
		Collector.AddReferencedObject(Pair.Value);
	}
}

FActorFolders& FActorFolders::Get()
{
	check(Singleton);
	return *Singleton;
}

void FActorFolders::Init()
{
	Singleton = new FActorFolders;
}

void FActorFolders::Cleanup()
{
	delete Singleton;
	Singleton = nullptr;
}

void FActorFolders::Housekeeping()
{
	TArray<TWeakObjectPtr<UWorld>> Worlds;
	TemporaryWorldFolders.GenerateKeyArray(Worlds);
	for (const auto& World : Worlds)
	{
		if (!World.Get())
		{
			TemporaryWorldFolders.Remove(World);
		}
	}
}

void FActorFolders::OnLevelActorListChanged()
{
	Housekeeping();
	check(GWorld);
	RebuildFolderListForWorld(*GWorld);
}

void FActorFolders::OnMapChange(uint32 MapChangeFlags)
{
	Housekeeping();
	check(GWorld);
	RebuildFolderListForWorld(*GWorld);
}

void FActorFolders::OnLevelActorAdded(AActor* Actor)
{
	check(Actor && Actor->GetWorld());

	if (!Actor->GetFolderPath().IsNone())
	{
		UEditorActorFolders& Folders = GetFoldersForWorld_Internal(*Actor->GetWorld());
		const FName& ActorFolder = Actor->GetFolderPath();
		if (!Folders.Paths.Contains(ActorFolder))
		{
			const FScopedTransaction Transaction(LOCTEXT("UndoAction_ActorAdded", "Actor Folder Added"));
			Folders.Modify();
			Folders.Paths.Add(ActorFolder);
		}
	}
}

void FActorFolders::OnActorFolderChanged(const AActor* InActor, FName OldPath)
{
	if (const auto* World = InActor->GetWorld())
	{
		const FScopedTransaction Transaction(LOCTEXT("UndoAction_FolderChanged", "Actor Folder Changed"));

		UEditorActorFolders& Folders = GetFoldersForWorld_Internal(*World);
		Folders.Modify();

		const auto NewPath = InActor->GetFolderPath();
		if (!NewPath.IsNone() && !Folders.Paths.Contains(NewPath))
		{
			Folders.Paths.Add(NewPath);
			OnFolderCreate.Broadcast(*World, NewPath);
		}
	}
}

bool FActorFolders::PathIsChildOf(const FString& InPotentialChild, const FString& InParent)
{
	const int32 ParentLen = InParent.Len();
	return
		InPotentialChild.Len() > ParentLen &&
		InPotentialChild[ParentLen] == '/' &&
		InPotentialChild.Left(ParentLen) == InParent;
}

void FActorFolders::RebuildFolderListForWorld(UWorld& InWorld)
{
	GetFoldersForWorld_Internal(InWorld).Paths = ScanWorldForFolders(InWorld).Array();
}

const TArray<FName>& FActorFolders::GetFoldersForWorld(const UWorld& InWorld)
{
	return GetFoldersForWorld_Internal(InWorld).Paths;
}

UEditorActorFolders& FActorFolders::GetFoldersForWorld_Internal(const UWorld& InWorld)
{
	if (UEditorActorFolders** Folders = TemporaryWorldFolders.Find(&InWorld))
	{
		return **Folders;
	}

	UEditorActorFolders* Folders = ConstructObject<UEditorActorFolders>(UEditorActorFolders::StaticClass(), GetTransientPackage(), NAME_None, RF_Transactional);
	return *TemporaryWorldFolders.Add(&InWorld, Folders);
}

TSet<FName> FActorFolders::ScanWorldForFolders(UWorld& InWorld)
{
	TSet<FName> Folders;

	// Iterate over every actor in memory. WARNING: This is potentially very expensive!
	for( FActorIterator ActorIt(&InWorld); ActorIt; ++ActorIt )
	{
		AActor* Actor = *ActorIt;
		check(Actor);
		const FName& Path = Actor->GetFolderPath();
		if (!Path.IsNone())
		{
			Folders.Add(Actor->GetFolderPath());
		}
	}

	// Add any empty folders
	for (const auto& Folder : GetFoldersForWorld_Internal(InWorld).Paths)
	{
		Folders.Add(Folder);
	}

	return Folders;
}

FName FActorFolders::GetDefaultFolderNameForSelection(UWorld& InWorld)
{
	// Find a common parent folder, or put it at the root
	FName CommonParentFolder;
	for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt; ++SelectionIt )
	{
		AActor* Actor = CastChecked<AActor>(*SelectionIt);
		if (CommonParentFolder.IsNone())
		{
			CommonParentFolder = Actor->GetFolderPath();
		}
		else if (Actor->GetFolderPath() != CommonParentFolder)
		{
			CommonParentFolder = NAME_None;
			break;
		}
	}

	return GetDefaultFolderName(InWorld, CommonParentFolder);
}

FName FActorFolders::GetDefaultFolderName(UWorld& InWorld, FName ParentPath)
{
	// This is potentially very slow but necessary to find a unique name
	const TSet<FName> ExistingFolders = ScanWorldForFolders(InWorld);

	// Create a valid base name for this folder
	uint32 Suffix = 1;
	FText LeafName = FText::Format(LOCTEXT("DefaultFolderNamePattern", "NewFolder{0}"), FText::AsNumber(Suffix++));

	FString ParentFolderPath = ParentPath.IsNone() ? TEXT("") : ParentPath.ToString();
	if (!ParentFolderPath.IsEmpty())
	{
		ParentFolderPath += "/";
	}

	FName FolderName(*(ParentFolderPath + LeafName.ToString()));
	while (ExistingFolders.Contains(FolderName))
	{
		LeafName = FText::Format(LOCTEXT("DefaultFolderNamePattern", "NewFolder{0}"), FText::AsNumber(Suffix++));
		FolderName = FName(*(ParentFolderPath + LeafName.ToString()));
		if (Suffix == 0)
		{
			// We've wrapped around a 32bit unsigned int - something must be seriously wrong!
			return FName();
		}
	}

	return FolderName;
}

void FActorFolders::CreateFolderContainingSelection(UWorld& InWorld, FName Path)
{
	const FScopedTransaction Transaction(LOCTEXT("UndoAction_CreateFolder", "Create Folder"));
	CreateFolder(InWorld, Path);

	// Move the currently selected actors into the new folder
	for( FSelectionIterator SelectionIt( *GEditor->GetSelectedActors() ); SelectionIt; ++SelectionIt )
	{
		AActor* Actor = CastChecked<AActor>(*SelectionIt);
		Actor->SetFolderPath(Path);
	}
}

void FActorFolders::CreateFolder(UWorld& InWorld, FName Path)
{
	const FScopedTransaction Transaction(LOCTEXT("UndoAction_CreateFolder", "Create Folder"));

	UEditorActorFolders& Folders = GetFoldersForWorld_Internal(InWorld);
	if (!Folders.Paths.Contains(Path))
	{
		Folders.Modify();
		Folders.Paths.Add(Path);
		OnFolderCreate.Broadcast(InWorld, Path);
	}
}

void FActorFolders::DeleteFolder(const UWorld& InWorld, FName FolderToDelete)
{
	const FScopedTransaction Transaction(LOCTEXT("UndoAction_DeleteFolder", "Delete Folder"));

	UEditorActorFolders& Folders = GetFoldersForWorld_Internal(InWorld);
	if (Folders.Paths.Contains(FolderToDelete))
	{
		Folders.Modify();
		Folders.Paths.Remove(FolderToDelete);
		OnFolderDelete.Broadcast(InWorld, FolderToDelete);
	}
}

bool FActorFolders::RenameFolderInWorld(UWorld& World, FName OldPath, FName NewPath)
{
	if (OldPath.IsNone() || OldPath == NewPath || PathIsChildOf(NewPath.ToString(), OldPath.ToString()))
	{
		return false;
	}

	const FScopedTransaction Transaction(LOCTEXT("UndoAction_RenameFolder", "Rename Folder"));

	const FString OldPathString = OldPath.ToString();
	const FString NewPathString = NewPath.ToString();

	TSet<FName> RenamedFolders;

	// Move any folders we currently hold - old ones will be deleted later
	UEditorActorFolders& FoldersInWorld = GetFoldersForWorld_Internal(World);
	FoldersInWorld.Modify();

	auto PathsCopy = FoldersInWorld.Paths;
	for (const auto& Path : PathsCopy)
	{
		const FString FolderPath = Path.ToString();
		if (OldPath == Path || PathIsChildOf(FolderPath, OldPathString))
		{
			const FName NewFolder = OldPathToNewPath(OldPathString, NewPathString, FolderPath);
			if (!FoldersInWorld.Paths.Contains(NewFolder))
			{
				FoldersInWorld.Paths.Add(NewFolder);
				OnFolderCreate.Broadcast(World, NewFolder);
			}
			RenamedFolders.Add(Path);
		}
	}

	// Now that we have folders created, move any actors that ultimately reside in that folder too
	for (auto ActorIt = FActorIterator(&World); ActorIt; ++ActorIt)
	{
		const FName& OldActorPath = ActorIt->GetFolderPath();
		
		AActor* Actor = *ActorIt;
		if (OldActorPath.IsNone())
		{
			continue;
		}

		if (Actor->GetFolderPath() == OldPath || PathIsChildOf(OldActorPath.ToString(), OldPathString))
		{
			RenamedFolders.Add(OldActorPath);
			ActorIt->SetFolderPath(OldPathToNewPath(OldPathString, NewPathString, OldActorPath.ToString()));
		}
	}

	// Cleanup any old folders
	for (const auto& Path : RenamedFolders)
	{
		FoldersInWorld.Paths.Remove(Path);
		OnFolderDelete.Broadcast(World, Path);
	}

	return RenamedFolders.Num() != 0;
}

#undef LOCTEXT_NAMESPACE