// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

/**
 * An widget item in the hierarchy tree view.
 */
class SUMGEditorTreeItem : public STableRow< UWidget* >
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTreeItem ){}
		
		/** The current text to highlight */
		SLATE_ATTRIBUTE( FText, HighlightText )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, UWidget* InItem);

	// Begin SWidget
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End SWidget
	
private:
	/** Called when text is being committed to check for validity */
	bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

	/* Called when text is committed on the node */
	void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

	/* Gets the font to use for the text item, bold for customized named items */
	FSlateFontInfo GetItemFont() const;

	/* @returns the widget name to use for the tree item */
	FText GetItemText() const;

	/* The blueprint editor this tree view item's tree is associated with. */
	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;

	/* The template widget that this tree item represents */
	UWidget* Item;
};
