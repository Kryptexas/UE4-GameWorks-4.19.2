// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SceneOutlinerPrivatePCH.h"
#include "SceneOutlinerTreeItems.h"
#include "SSceneOutliner.h"

#include "ScopedTransaction.h"
#include "EditorActorFolders.h"
#define LOCTEXT_NAMESPACE "SceneOutlinerTreeItems"

namespace SceneOutliner
{

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
			ActorOp->Init(Actors);
			ActorOp->Folders = Folders;
			Operation = ActorOp;
		}
		else
		{
			TSharedRef<FFolderDragDropOp> FolderOp = MakeShareable(new FFolderDragDropOp);
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
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorAndFolder", "{NumActors} Actor, {NumFolders} Folder"), Args);
					}
					else
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorsAndFolder", "{NumActors} Actors, {NumFolders} Folder"), Args);
					}
				}
				else
				{
					if (Actors.Num() == 1)
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorAndFolders", "{NumActors} Actor, {NumFolders} Folders"), Args);
					}
					else
					{
						Operation->CurrentHoverText = FText::Format(LOCTEXT("DragActorsAndFolders", "{NumActors} Actor, {NumFolders} Folders"), Args);
					}
				}
			}
			else if (Folders.Num() == 1)
			{
				auto TheFolder = Folders[0].Pin();
				if (TheFolder.IsValid())
				{
					Operation->CurrentHoverText = FText::FromString(TheFolder->LeafName.ToString());
					Operation->CurrentIconBrush = FEditorStyle::Get().GetBrush(TEXT("SceneOutliner.FolderClosed"));
				}
			}
			else
			{
				Operation->CurrentHoverText = FText::Format(LOCTEXT("DragFolders", "{NumFolders} Folders"), Args);
			}
		}

		Operation->SetupDefaults();
		Operation->Construct();
		return Operation.ToSharedRef();
	}
}

#undef LOCTEXT_NAMESPACE
