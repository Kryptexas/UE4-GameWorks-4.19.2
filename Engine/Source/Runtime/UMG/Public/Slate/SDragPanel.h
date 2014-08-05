// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A delegate that is invoked when widgets want to notify a user that they have been clicked.
 * Intended for use by buttons and other button-like widgets.
 */
DECLARE_DELEGATE_RetVal_TwoParams( 
	TSharedPtr<FDragDropOperation>,
	FOnDragOperationRequested,
	FGeometry,
	FPointerEvent);

/**
 * 
 */
class UMG_API SDragPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDragPanel)
	: _Content()
	{
		_Visibility = EVisibility::Visible;
	}
		/** Slot for this designers content (optional) */
		SLATE_DEFAULT_SLOT(FArguments, Content)

		/**  */
		SLATE_EVENT(FOnDragOperationRequested, OnDragDetected)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	/** See Content slot */
	void SetContent(TSharedRef<SWidget> InContent);

private:

	/** The delegate to execute when the drag drop operation is requested */
	FOnDragOperationRequested OnDragOperationRequested;
};
