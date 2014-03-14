// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "SSceneOutlinerItemWidget.h"

namespace SceneOutliner
{
	struct TOutlinerTreeItem
	{
		/** The actor this tree item is associated with. */
		TWeakObjectPtr<AActor> Actor;

		/** true if this item has been filtered out. */
		bool bIsFilteredOut;

		/** true if this item exists in both the current world and PIE. */
		bool bExistsInCurrentWorldAndPIE;

		/** True if this item will enter inline renaming on the next scroll into view */
		bool bRenameWhenScrolledIntoView;

		/** Delegate for hooking up an inline editable text block to be notified that a rename is requested. */
		DECLARE_DELEGATE( FOnRenameRequest );

		/** Broadcasts whenever a rename is requested */
		FOnRenameRequest RenameRequestEvent;

		TOutlinerTreeItem()
			: bIsFilteredOut(false)
			, bExistsInCurrentWorldAndPIE(false)
			, bRenameWhenScrolledIntoView(false)
		{

		}
	};

	class SSceneOutliner;

	typedef TSharedPtr<TOutlinerTreeItem> FOutlinerTreeItemPtr;

	/** IDs for list columns */
	static const FName ColumnID_ActorLabel( "Actor" );

	class SOutlinerTreeView : public STreeView< FOutlinerTreeItemPtr >
	{
	public:
		void FlashHighlightOnItem( FOutlinerTreeItemPtr FlashHighlightOnItem );
	};

	/** Widget that represents a row in the outliner's tree control.  Generates widgets for each column on demand. */
	class SSceneOutlinerTreeRow
		: public SMultiColumnTableRow< FOutlinerTreeItemPtr >
	{

	public:

		SLATE_BEGIN_ARGS( SSceneOutlinerTreeRow ) {}

			/** The Scene Outliner widget that owns the tree.  We'll only keep a weak reference to it. */
			SLATE_ARGUMENT( TSharedPtr< SSceneOutliner >, SceneOutliner )

			/** The list item for this row */
			SLATE_ARGUMENT( FOutlinerTreeItemPtr, Item )

		SLATE_END_ARGS()


		/** Construct function for this widget */
		void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView );


		/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the tree row. */
		virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) OVERRIDE;

		TSharedPtr< SSceneOutlinerItemWidget > GetItemWidget();

	protected:

		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

		FReply HandleDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

		void PerformDetachment(TWeakObjectPtr<AActor> ParentActorPtr );

		void PerformAttachment(FName SocketName, TWeakObjectPtr<AActor> ParentActorPtr, const TArray< TWeakObjectPtr<AActor> > ChildrenPtrs);

		void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );

		void OnDragLeave( const FDragDropEvent& DragDropEvent );

		FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );


	private:

		/** Weak reference to the outliner widget that owns our list */
		TWeakPtr< SSceneOutliner > SceneOutlinerWeak;

		/** The widget contained in this row representing the associated item */
		TSharedPtr< SSceneOutlinerItemWidget > ItemWidget;

		/** The item associated with this row of data */
		FOutlinerTreeItemPtr Item;

		/** State to restore to the active drag drop item when it leaves this widget */
		FActorDragDropGraphEdOp::ToolTipTextType DragDropTooltipToRestore;
	};
}		// namespace SceneOutliner
