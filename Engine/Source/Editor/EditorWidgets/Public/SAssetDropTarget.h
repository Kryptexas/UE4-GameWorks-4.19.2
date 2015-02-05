// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A widget that displays a hover cue and handles dropping assets of allowed types onto this widget
 */
class EDITORWIDGETS_API SAssetDropTarget : public SCompoundWidget
{
public:
	/** Called when a valid asset is dropped */
	DECLARE_DELEGATE_OneParam( FOnAssetDropped, UObject* );

	/** Called when we need to check if an asset type is valid for dropping */
	DECLARE_DELEGATE_RetVal_OneParam( bool, FIsAssetAcceptableForDrop, const UObject* );

	SLATE_BEGIN_ARGS(SAssetDropTarget)
	{ }
		/* Content to display for the in the drop target */
		SLATE_DEFAULT_SLOT( FArguments, Content )
		/** Called when a valid asset is dropped */
		SLATE_EVENT( FOnAssetDropped, OnAssetDropped )
		/** Called to check if an asset is acceptible for dropping */
		SLATE_EVENT( FIsAssetAcceptableForDrop, OnIsAssetAcceptableForDrop )
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs );

private:
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

	/** @return The text to show in the overlay when a drag object is recognized and is acceptable or not. */
	FText GetDragOverlayText() const;

	/** Get the brightness on the background. */
	FSlateColor GetBackgroundBrightness() const;

	/**
	 * Gets the dropped object from a drag drop event
	 */
	UObject* GetDroppedObject(TSharedPtr<FDragDropOperation> DragDropOperation, bool& bOutRecognizedEvent) const;

	/** Utility function that can determine if this asset drop target accepts the item being dragged and dropped. */
	bool AllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const;
private:
	/** Delegate to call when an asset is dropped */
	FOnAssetDropped OnAssetDropped;
	/** Delegate to call to check validity of the asset */
	FIsAssetAcceptableForDrop OnIsAssetAcceptableForDrop;
	/** Whether or not we are being dragged over by a recognized event*/
	mutable bool bIsDragEventRecognized;
	/** Whether or not we currently allow dropping */
	mutable bool bAllowDrop;
	/** Is the drag operation currently over our airspace? */
	mutable bool bIsDragOver;
};