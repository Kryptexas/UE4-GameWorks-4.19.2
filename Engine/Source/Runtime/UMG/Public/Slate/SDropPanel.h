// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * 
 */
DECLARE_DELEGATE_TwoParams(
	FOnDragEnterEvent,
	FGeometry,
	const FDragDropEvent&);

	/**
 * 
 */
DECLARE_DELEGATE_OneParam(
	FOnDragLeaveEvent,
	const FDragDropEvent&);

/**
 * 
 */
DECLARE_DELEGATE_RetVal_TwoParams(
	FReply,
	FOnDragDropEvent,
	FGeometry,
	const FDragDropEvent&);

/**
 * 
 */
class UMG_API SDropPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDropPanel)
	: _Content()
	{
		_Visibility = EVisibility::Visible;
	}
		/** Slot for this designers content (optional) */
		SLATE_DEFAULT_SLOT(FArguments, Content)

		/**  */
		SLATE_EVENT(FOnDragEnterEvent, OnDragEnter)

		/**  */
		SLATE_EVENT(FOnDragLeaveEvent, OnDragLeave)

		/**  */
		SLATE_EVENT(FOnDragDropEvent, OnDragOver)

		/**  */
		SLATE_EVENT(FOnDragDropEvent, OnDrop)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	
	/** See Content slot */
	void SetContent(TSharedRef<SWidget> InContent);

private:

	FOnDragEnterEvent OnDragEnterDelegate;
	FOnDragLeaveEvent OnDragLeaveDelegate;
	FOnDragDropEvent OnDragOverDelegate;
	FOnDragDropEvent OnDropDelegate;
};
