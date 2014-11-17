
#pragma once

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"

namespace SceneOutliner
{

struct SCENEOUTLINER_API TOutlinerTreeItem : TSharedFromThis<TOutlinerTreeItem>
{
	enum ItemType { Folder, Actor };
	/* Flags structure */
	struct FlagsType
	{
		union
		{
			/* Access to unioned flag structure */
			int8 AllFlags;
			struct
			{
				/* true if this item should be interactively renamed when it is scrolled into view */
				bool RenameWhenInView : 1;
				/* true if this item is filtered out */
				bool IsFilteredOut : 1;
			};
		};
	};

	/** The type of this tree item */
	ItemType Type;

	/** Delegate for hooking up an inline editable text block to be notified that a rename is requested. */
	DECLARE_DELEGATE( FOnRenameRequest );

	/** Broadcasts whenever a rename is requested */
	FOnRenameRequest RenameRequestEvent;
	
	/** Flags for this item */
	FlagsType Flags;

	/** Constructor */
	TOutlinerTreeItem(const ItemType InType)
		: Type(InType)
	{
		Flags.AllFlags = 0;
	}

	/** Determine if this tree item is visible or not */
	virtual bool IsVisible() const = 0;

	/** Set the visibility of this item */
	virtual void SetIsVisible(bool bIsVisible) = 0;
};

struct SCENEOUTLINER_API TOutlinerActorTreeItem : TOutlinerTreeItem
{
	/** The actor this tree item is associated with. */
	TWeakObjectPtr<AActor> Actor;

	/** true if this item exists in both the current world and PIE. */
	bool bExistsInCurrentWorldAndPIE;

	TOutlinerActorTreeItem(const AActor* InActor)
		: TOutlinerTreeItem(TOutlinerTreeItem::Actor)
		, bExistsInCurrentWorldAndPIE(false)
		, Actor(InActor)
	{
	}

	/** Determine if this tree item is visible or not */
	virtual bool IsVisible() const override;

	/** Set the visibility of this item */
	virtual void SetIsVisible(bool bIsVisible) override;
};

struct SCENEOUTLINER_API TOutlinerFolderTreeItem : TOutlinerTreeItem
{
	/** The leaf name of this folder */
	FName LeafName;

	/** The path of this folder. / separated. */
	FName Path;

	/** An array of child items for this folder */
	TArray<TWeakPtr<TOutlinerTreeItem>> ChildItems;

	/** Constructor that takes a path to this folder (including leaf-name) */
	TOutlinerFolderTreeItem(FName InPath)
		: TOutlinerTreeItem(TOutlinerTreeItem::Folder)
	{
		ParsePath(InPath);
	}

	/** Determine if this tree item is visible or not */
	virtual bool IsVisible() const override;
	
	/** Set the visibility of this item */
	virtual void SetIsVisible(bool bIsVisible) override;

	/** Parse a new path (including leaf-name) into this tree item. Does not do any notification */
	void ParsePath(FName InPath)
	{
		Path = InPath;
		FString PathString = Path.ToString();
		int32 LeafIndex = 0;
		if (PathString.FindLastChar('/', LeafIndex))
		{
			LeafName = FName(*PathString.RightChop(LeafIndex + 1));
		}
		else
		{
			LeafName = Path;
		}
	}

	/** Get the parent path for this folder */
	FName GetParentPath() const
	{
		return GetParentPath(Path);
	}

	/** Populate the specified set with all our parent paths */
	void GetParentPaths(TSet<FName>& ParentPaths) const
	{
		FString ParentPath = *FPaths::GetPath(Path.ToString());
		while (!ParentPath.IsEmpty())
		{
			ParentPaths.Add(FName(*ParentPath));
			ParentPath = *FPaths::GetPath(ParentPath);
		}
	}

	/** Get the parent path for the specified folder path */
	static FName GetParentPath(FName Path)
	{
		return FName(*FPaths::GetPath(Path.ToString()));
	}
};

/** Construct a new Drag and drop operation for a scene outliner */
TSharedRef<FDecoratedDragDropOp> CreateDragDropOperation(TArray<TSharedPtr<TOutlinerTreeItem>>& InTreeItems);

/* A drag/drop operation involving a mix of actors and (optionally) folders */
struct SCENEOUTLINER_API FFolderActorDragDropOp : public FActorDragDropGraphEdOp
{
	DRAG_DROP_OPERATOR_TYPE(FFolderActorDragDropOp, FActorDragDropGraphEdOp)

	/** Array of folders that we are dragging in addition to the actors */
	TArray<TWeakPtr<TOutlinerFolderTreeItem>> Folders;
};

/** A drag/drop operation involving only folders */
struct SCENEOUTLINER_API FFolderDragDropOp : public FDecoratedDragDropOp
{
	DRAG_DROP_OPERATOR_TYPE(FFolderDragDropOp, FDecoratedDragDropOp)

	/** Array of folders that we are dragging */
	TArray<TWeakPtr<TOutlinerFolderTreeItem>> Folders;
};

}	// namespace SceneOutliner