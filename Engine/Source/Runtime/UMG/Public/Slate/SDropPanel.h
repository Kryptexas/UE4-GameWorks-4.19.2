// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

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

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	
	/** See Content slot */
	void SetContent(TSharedRef<SWidget> InContent);
};
