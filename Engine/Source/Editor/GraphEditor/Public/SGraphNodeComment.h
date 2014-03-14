// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SGraphNodeComment : public SGraphNode
{
	/**
	 * The Comment window zone the user is interacting with
	 */
	enum ECommentWindowZone
	{
		CWZ_NotInWindow			= 0,
		CWZ_InWindow			= 1,
		CWZ_RightBorder			= 2,
		CWZ_BottomBorder		= 3,
		CWZ_BottomRightBorder	= 4,
		CWZ_LeftBorder			= 5,
		CWZ_TopBorder			= 6,
		CWZ_TopLeftBorder		= 7,
		CWZ_TopRightBorder		= 8,
		CWZ_BottomLeftBorder	= 9,
		CWZ_TitleBar			= 10,
	};

public:
	SLATE_BEGIN_ARGS(SGraphNodeComment){}
	SLATE_END_ARGS()

	// Begin SWidget Interface
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual bool OnHitTest( const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition ) OVERRIDE;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	// End SWidget Interface

	// Begin SNodePanel::SNode interface
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const OVERRIDE;
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const;
	virtual bool ShouldAllowCulling() const OVERRIDE { return false; }
	// End SNodePanel::SNode interface

	// Begin SPanel Interface
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	// End SPanel interface

	// Begin SGraphNode Interface
	virtual bool IsNameReadOnly() const OVERRIDE;
	// ENd SGraphNode Interface

	void Construct( const FArguments& InArgs, UEdGraphNode* InNode );

	/** return if the node can be selected, by pointing given location */
	virtual bool CanBeSelected( const FVector2D& MousePositionInNode ) const OVERRIDE;

	/** return size of the title bar */
	virtual FVector2D GetDesiredSizeForMarquee() const OVERRIDE;

	/** return rect of the title bar */
	virtual FSlateRect GetTitleRect() const OVERRIDE;

protected:
	// SGraphNode Interface
	virtual void UpdateGraphNode() OVERRIDE;
	virtual bool ShouldScaleNodeComment()const OVERRIDE;

	/**
	 * Helper method to update selection state of comment and any nodes 'contained' within it
	 * @param bSelected	If true comment is being selected, false otherwise
	 * @param bUpdateNodesUnderComment If true then force the rebuild of the list of nodes under the comment
	 */
	void HandleSelection(bool bIsSelected, bool bUpdateNodesUnderComment = false) const;

	/** called when user is moving the comment node */
	virtual void MoveTo(const FVector2D& NewPosition) OVERRIDE;

private:

	/** Find the current window zone the mouse is in for the comment */
	SGraphNodeComment::ECommentWindowZone FindMouseZone(const FVector2D& LocalGeo) const;

	/** @return true if the current window zone is considered a selection area */
	bool InSelectionArea() const { return InSelectionArea(MouseZone); }

	/** @return true if the passed zone is a selection area */
	bool InSelectionArea(ECommentWindowZone InZone) const;

	/** @return the color to tint the comment body */
	FSlateColor GetCommentBodyColor() const;

	/** @return the color to tint the title bar */
	FSlateColor GetCommentTitleBarColor() const;

	/** Function to store anchor point before resizing the node. The node will be anchored to this point when resizing happens*/
	void InitNodeAnchorPoint();

	/** Function to fetch the corrected node position based on anchor point*/
	FVector2D GetCorrectedNodePosition() const;

	/** Returns the width to wrap the text of the comment at */
	float GetWrapAt() const;

private:
	/** The desired size of the comment box, set during a drag */
	FVector2D UserSize;

	/** The original size of the comment box while resizing */
	FVector2D StoredUserSize;

	/** Anchor point used to correct node position on resizing the node*/
	FVector2D NodeAnchorPoint;

	/** The current window zone the mouse is in */
	ECommentWindowZone MouseZone;

	/** If true the user is actively dragging the comment */
	bool bUserIsDragging;

	/** The current selection state of the comment */
	mutable bool bIsSelected;

	/** the title bar, needed to obtain it's height */
	TSharedPtr<SBorder> TitleBar;

	/** cached comment title */
	FString CachedCommentTitle; 

	/** cached comment title */
	int32 CachedWidth;
};
