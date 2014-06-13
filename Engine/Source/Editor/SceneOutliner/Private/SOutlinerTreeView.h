// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Editor/UnrealEd/Public/DragAndDrop/ActorDragDropGraphEdOp.h"
#include "SSceneOutlinerItemWidget.h"

namespace SceneOutliner
{
	class SSceneOutliner;

	typedef TSharedPtr<TOutlinerTreeItem> FOutlinerTreeItemPtr;
	typedef TSharedRef<TOutlinerTreeItem> FOutlinerTreeItemRef;

	/** IDs for list columns */
	static const FName ColumnID_ActorLabel( "Actor" );

	class SOutlinerTreeView : public STreeView< FOutlinerTreeItemPtr >
	{
	public:
		
		/** Construct this widget */
		void Construct(const FArguments& InArgs, TSharedRef<SSceneOutliner> Owner);

		void FlashHighlightOnItem( FOutlinerTreeItemPtr FlashHighlightOnItem );

		/** Check that it is valid to move the current selection to the specified folder path */
		bool ValidateMoveSelectionTo(FName NewParent, FText* OutValidationText = nullptr);

	protected:

		virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;

		virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;

		virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;

		/** Weak reference to the outliner widget that owns this list */
		TWeakPtr<SSceneOutliner> SceneOutlinerWeak;
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
		virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override;

		TSharedPtr< SSceneOutlinerItemWidget > GetItemWidget();

	protected:

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

		FReply HandleDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
		
		void PerformDetachment(TWeakObjectPtr<AActor> ParentActorPtr );

		void PerformAttachment(FName SocketName, TWeakObjectPtr<AActor> ParentActorPtr, const TArray< TWeakObjectPtr<AActor> > ChildrenPtrs);

		virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;

		virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override
		{
			return FReply::Handled();
		}

		virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;


	private:

		/** Weak reference to the outliner widget that owns our list */
		TWeakPtr< SSceneOutliner > SceneOutlinerWeak;

		/** The widget contained in this row representing the associated item */
		TSharedPtr< SSceneOutlinerItemWidget > ItemWidget;

		/** The item associated with this row of data */
		FOutlinerTreeItemPtr Item;
	};
}		// namespace SceneOutliner
