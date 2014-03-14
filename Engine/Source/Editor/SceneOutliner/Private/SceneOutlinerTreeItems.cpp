// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SceneOutlinerPrivatePCH.h"
#include "SceneOutlinerTreeItems.h"
#include "SSceneOutliner.h"

#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SceneOutlinerTreeItems"

namespace SceneOutliner
{
	/** Called to broadcast that a folder is going to be moved.
	 *	Since folders are implied only by actor folder paths, they don't actually exist anywhere
	 *	in a level. We use this broadcast mechanism to change folders to ensure that all folders in
	 *	all currently open scene outliners stay up to date.
	 */
	void BroadcastFolderMove(TOutlinerFolderTreeItem& Folder, FName NewPath)
	{
		// We copy this array as the broadcast may cause children to be removed
		auto ChildItemsCopy = Folder.ChildItems;
		for (auto ChildIt = ChildItemsCopy.CreateIterator(); ChildIt; ++ChildIt)
		{
			auto ChildItem = ChildIt->Pin();
			if (ChildItem.IsValid() && ChildItem->Type == TOutlinerTreeItem::Folder)
			{
				auto ChildFolder = StaticCastSharedPtr<TOutlinerFolderTreeItem>(ChildItem);
				FString NewChildPath = NewPath.ToString() / ChildFolder->LeafName.ToString();
				BroadcastFolderMove(*ChildFolder, FName(*NewChildPath));
			}
		}

		SSceneOutliner::OnFolderMove.Broadcast(Folder.Path, NewPath);
	}

	/** Called to set all the recursive actor children to have a different folder */
	void SetActorFoldersRecursive(TOutlinerFolderTreeItem& Folder, FName NewPath)
	{
		// Now set the paths on all the actor children - operate on a copy in case the actual child items is manipulated
		auto ChildItemsCopy = Folder.ChildItems;
		for (auto& WeakChild : ChildItemsCopy)
		{
			auto ChildItem = WeakChild.Pin();
			if (ChildItem.IsValid())
			{
				if (ChildItem->Type == TOutlinerTreeItem::Actor)
				{
					auto ChildActorItem = StaticCastSharedPtr<TOutlinerActorTreeItem>(ChildItem);
					auto* ChildActor = ChildActorItem->Actor.Get();
					if (ChildActor && ChildActor->GetFolderPath() != NewPath)
					{
						ChildActor->SetFolderPath(NewPath);
					}
				}
				else
				{
					auto ChildFolderItem = StaticCastSharedPtr<TOutlinerFolderTreeItem>(ChildItem);
					const FName NewChildPath(*(NewPath.ToString() / ChildFolderItem->LeafName.ToString()));
					SetActorFoldersRecursive(*ChildFolderItem, NewChildPath);
				}
			}
		}
	}

	bool TOutlinerActorTreeItem::IsVisible() const
	{
		const auto* ActorPtr = Actor.Get();
		return ActorPtr && !ActorPtr->IsTemporarilyHiddenInEditor();
	}

	void TOutlinerActorTreeItem::SetIsVisible(bool bIsVisible)
	{
		auto* ActorPtr = Actor.Get();
		if (ActorPtr)
		{
			// Save the actor to the transaction buffer to support undo/redo, but do
			// not call Modify, as we do not want to dirty the actor's package and
			// we're only editing temporary, transient values
			SaveToTransactionBuffer(ActorPtr, false);
			ActorPtr->SetIsTemporarilyHiddenInEditor( !bIsVisible );
		}
	}

	bool TOutlinerFolderTreeItem::IsVisible() const
	{
		for (const auto& Child : ChildItems)
		{
			auto ChildPtr = Child.Pin();
			if (ChildPtr.IsValid() && ChildPtr->IsVisible())
			{
				return true;
			}
		}
		return false;
	}

	void TOutlinerFolderTreeItem::SetIsVisible(bool bIsVisible)
	{
		for (const auto& Child : ChildItems)
		{
			auto ChildPtr = Child.Pin();
			if (ChildPtr.IsValid())
			{
				ChildPtr->SetIsVisible(bIsVisible);
			}	
		}
	}

	void TOutlinerFolderTreeItem::RelocateChildren(FName NewPath)
	{
		// Change actor paths first, then broadcast the folder move (to catch any empty folders which should be moved as well)
		SetActorFoldersRecursive(*this, NewPath);
		BroadcastFolderMove(*this, NewPath);
	}

	void TOutlinerFolderTreeItem::Rename(FName NewLabel)
	{
		const FScopedTransaction Transaction(LOCTEXT("UndoAction_RenameFolder", "Rename Folder"));

		FName NewPath = GetParentPath();
		if (NewPath.IsNone())
		{
			NewPath = FName(*NewLabel.ToString());
		}
		else
		{
			NewPath = FName(*(NewPath.ToString() / NewLabel.ToString()));
		}

		RelocateChildren(NewPath);
	}

	void TOutlinerFolderTreeItem::MoveTo(FName NewPath)
	{
		const FScopedTransaction Transaction(LOCTEXT("UndoAction_MoveFolder", "Move Folder"));

		if (NewPath.IsNone())
		{
			NewPath = LeafName;
		}
		else
		{
			NewPath = FName(*(NewPath.ToString() / LeafName.ToString()));
		}
		RelocateChildren(NewPath);
	}

	TSharedRef<FDecoratedDragDropOp> CreateDragDropOperation(TArray<TSharedPtr<TOutlinerTreeItem>>& InTreeItems)
	{
		TArray<TWeakObjectPtr<AActor>> Actors;
		TArray<TWeakPtr<TOutlinerFolderTreeItem>> Folders;

		for (const auto& Item : InTreeItems)
		{
			if (Item->Type == TOutlinerTreeItem::Actor)
			{
				Actors.Add(StaticCastSharedPtr<TOutlinerActorTreeItem>(Item)->Actor);
			}
			else
			{
				Folders.Add(StaticCastSharedPtr<TOutlinerFolderTreeItem>(Item));
			}
		}

		TSharedPtr<FDecoratedDragDropOp> Operation;
		if (Actors.Num())
		{
			TSharedRef<FFolderActorDragDropOp> ActorOp = MakeShareable(new FFolderActorDragDropOp);
			FSlateApplication::GetDragDropReflector().RegisterOperation<FFolderActorDragDropOp>(ActorOp);
			ActorOp->Init(Actors);
			ActorOp->Folders = Folders;
			Operation = ActorOp;
		}
		else
		{
			TSharedRef<FFolderDragDropOp> FolderOp = MakeShareable(new FFolderDragDropOp);
			FSlateApplication::GetDragDropReflector().RegisterOperation<FFolderDragDropOp>(FolderOp);
			FolderOp->Folders = Folders;
			Operation = FolderOp;
		}

		if (Folders.Num())
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("NumActors"), Actors.Num());
			Args.Add(TEXT("NumFolders"), Folders.Num());

			if (Actors.Num())
			{
				if (Folders.Num() == 1)
				{
					if (Actors.Num() == 1)
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorAndFolder", "{NumActors} Actor, {NumFolders} Folder"), Args).ToString();
					}
					else
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorsAndFolder", "{NumActors} Actors, {NumFolders} Folder"), Args).ToString();
					}
				}
				else
				{
					if (Actors.Num() == 1)
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorAndFolders", "{NumActors} Actor, {NumFolders} Folders"), Args).ToString();
					}
					else
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorsAndFolders", "{NumActors} Actor, {NumFolders} Folders"), Args).ToString();
					}
				}
			}
			else if (Folders.Num() == 1)
			{
				auto TheFolder = Folders[0].Pin();
				if (TheFolder.IsValid())
				{
					Operation->CurrentHoverText = FText::FromString(TheFolder->LeafName.ToString()).ToString();
					Operation->CurrentIconBrush = FEditorStyle::Get().GetBrush(TEXT("SceneOutliner.FolderClosed"));
				}
			}
			else
			{
				Operation->CurrentHoverText = FText::Format(LOCTEXT("DragFolders", "{NumFolders} Folders"), Args).ToString();
			}
		}

		Operation->SetupDefaults();
		Operation->Construct();
		return Operation.ToSharedRef();
	}
}

#undef LOCTEXT_NAMESPACE