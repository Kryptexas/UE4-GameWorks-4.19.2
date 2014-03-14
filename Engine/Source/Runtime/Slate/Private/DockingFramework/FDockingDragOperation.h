// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A Sample implementation of IDragDropOperation */
class FDockingDragOperation : public FDragDropOperation
{
public:

	/**
	 * Represents a target for the user re-arranging some layout.
	 * A user expresses their desire to re-arrange layout by placing a tab relative to some layout node.
	 * e.g. I want my tab left of the viewport tab.
	 */
	struct FDockTarget
	{
		FDockTarget()
		: TargetNode()
		, DockDirection()
		{
		}

		FDockTarget( const TSharedPtr<class SDockingNode>& InTargetNode, SDockingNode::RelativeDirection InDockDirection )
		: TargetNode( InTargetNode )
		, DockDirection( InDockDirection )
		{
		}

		bool operator==( const FDockTarget& Other )
		{
			return
				this->TargetNode == Other.TargetNode &&
				this->DockDirection == Other.DockDirection;
		}

		bool operator!=( const FDockTarget& Other )
		{
			return !((*this)==Other);
		}

		/** We'll put the tab relative to this node */
		TWeakPtr<class SDockingNode> TargetNode;
		/** Relation to node where we will put the tab.qqq */
		SDockingNode::RelativeDirection DockDirection;
	};

	static FString GetTypeId() {static FString Type = TEXT("FDockingDragOperation"); return Type;}

	/**
	 * Invoked when the drag and drop operation has ended.
	 * 
	 * @param bDropWasHandled   true when the drop was handled by some widget; false otherwise
	 */
	virtual void OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent );

	/** 
	 * Called when the mouse was moved during a drag and drop operation
	 *
	 * @param DragDropEvent    The event that describes this drag drop operation.
	 */
	virtual void OnDragged( const FDragDropEvent& DragDropEvent );
	
	/**
	 * DragTestArea widgets invoke this method when a drag enters them
	 *
	 * @param ThePanel   That tab well that we just dragged something into.
	 */
	void OnTabWellEntered( const TSharedRef<class SDockingTabWell>& ThePanel );

	/**
	 * DragTestArea widgets invoke this method when a drag leaves them
	 *
	 * @param ThePanel   That tab well that we just dragged something out of.
	 */
	void OnTabWellLeft( const TSharedRef<class SDockingTabWell>& ThePanel, const FGeometry& DockNodeGeometry );

	/**
	 * Given a docking direction and the geometry of the dockable area, figure out the area that will be occupied by a new tab if it is docked there.
	 *
	 * @param DockableArea       The area of a TabStack that you're hovering
	 * @param DockingDirection   Where relative to this TabStack you want to dock: e.g. to the right.
	 *
	 * @return The area that the new tab will occupy.
	 */
	FSlateRect GetPreviewAreaForDirection ( const FSlateRect& DockableArea, SDockingArea::RelativeDirection DockingDirection );

	/** Update which dock target, if any, is currently hovered as a result of the InputEvent */
	void SetHoveredTarget( const FDockTarget& InTarget, const FInputEvent& InputEvent );

	/**
	 * Create this Drag and Drop Content
	 *
	 * @param InWidgetBeingDragged    The Widget being dragged in this drag and drop operation
	 * @param InTabOwnerStack         The Widget that originally contained what we are dragging
	 *
	 * @return a new FDockingDragOperation
	 */
	static TSharedRef<FDockingDragOperation> New( const TSharedRef<SDockTab>& InTabToBeDragged, const FVector2D InTabGrabOffset, TSharedRef<class SDockingArea> InTabOwnerArea, const FVector2D& OwnerAreaSize );

	/** @return the widget being dragged */
	TSharedPtr<SDockTab> GetTabBeingDragged();

	/** @return location where the user grabbed within the tab as a fraction of the tab's size */
	FVector2D GetTabGrabOffsetFraction() const;

	virtual ~FDockingDragOperation();

protected:
	/** The constructor is protected, so that this class can only be instantiated as a shared pointer. */
	FDockingDragOperation( const TSharedRef<class SDockTab>& InTabToBeDragged, const FVector2D InTabGrabOffsetFraction, TSharedRef<class SDockingArea> InTabOwnerArea, const FVector2D& OwnerAreaSize );

	/** @return The offset into the tab where the user grabbed in Slate Units. */
	const FVector2D GetDecoratorOffsetFromCursor();

	/** @return the size of the DockNode that looks good in a preview given the initial size of the tab that we grabbed. */
	static FVector2D DesiredSizeFrom( const FVector2D& InitialTabSize );

	/** What is actually being dragged in this operation */
	TSharedPtr<class SDockTab> TabBeingDragged;

	/** Where the user grabbed the tab as a fraction of the tab's size */
	FVector2D TabGrabOffsetFraction;

	/** The area from which we initially started dragging */
	TSharedPtr<SDockingArea> TabOwnerAreaOfOrigin;

	/** The TabWell over which we are currently hovering */
	TWeakPtr<class SDockingTabWell> HoveredTabPanelPtr;

	/** Some target dock node over which we are currently hovering; it could be a TabStack or a DockAre */
	FDockTarget HoveredDockTarget;

	/** What the size of the content was when it was last shown. The user drags splitters to set this size; it is legitimate state. */
	FVector2D LastContentSize;
};
