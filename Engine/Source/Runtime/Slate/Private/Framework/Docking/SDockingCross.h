// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Targets used by docking code. When re-arranging layout, hovering over targets
 * gives the user a preview of what will happen if they drop on that target.
 * Dropping actually commits the layout-restructuring.
 */
class SLATE_API SDockingCross : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDockingCross){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedPtr<class SDockingNode>& InOwnerNode );

	// SWidget interface
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	// End of SWidget interface
	
private:

	/** The DockNode relative to which we want to dock */
	TWeakPtr<class SDockingNode> OwnerNode;
};

