// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A widget that displays a hover cue and handles dropping assets of allowed types onto this widget
 */
class EDITORWIDGETS_API SDropTarget : public SCompoundWidget
{
public:
	/** Called when a valid asset is dropped */
	DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnDrop, TSharedPtr<FDragDropOperation>);

	DECLARE_DELEGATE_RetVal_OneParam(bool, FVerifyDrag, TSharedPtr<FDragDropOperation>);

	SLATE_BEGIN_ARGS(SDropTarget)
	{ }
		/* Content to display for the in the drop target */
		SLATE_DEFAULT_SLOT( FArguments, Content )
		/** Called when a valid asset is dropped */
		SLATE_EVENT(FOnDrop, OnDrop)
		/** Called to check if an asset is acceptable for dropping */
		SLATE_EVENT(FVerifyDrag, OnAllowDrop)
		/** Called to check if an asset is acceptable for dropping */
		SLATE_EVENT(FVerifyDrag, OnIsRecognized)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs );

protected:

	bool AllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const;

	virtual bool OnAllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const;
	virtual bool OnIsRecognized(TSharedPtr<FDragDropOperation> DragDropOperation) const;

protected:
	FSlateColor GetDropBorderColor() const;

	// SWidget interface
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) override;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	// End of SWidget interface

	/** @return Visibility of the overlay text when dragging is occuring. */
	EVisibility GetDragOverlayVisibility() const;

	/** Get the brightness on the background. */
	FSlateColor GetBackgroundBrightness() const;

private:
	/** Delegate to call when an asset is dropped */
	FOnDrop DroppedEvent;
	/** Delegate to call to check validity of the asset */
	FVerifyDrag AllowDropEvent;
	/** Delegate to call to check validity of the asset */
	FVerifyDrag IsRecognizedEvent;

	/** Whether or not we are being dragged over by a recognized event*/
	mutable bool bIsDragEventRecognized;
	/** Whether or not we currently allow dropping */
	mutable bool bAllowDrop;
	/** Is the drag operation currently over our airspace? */
	mutable bool bIsDragOver;
};