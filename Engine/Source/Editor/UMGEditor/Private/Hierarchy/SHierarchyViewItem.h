// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

class FHierarchyModel : public TSharedFromThis < FHierarchyModel >
{
public:
	FHierarchyModel();

	/* @returns the widget name to use for the tree item */
	virtual FText GetText() const = 0;

	virtual const FSlateBrush* GetImage() const = 0;

	virtual FSlateFontInfo GetFont() const = 0;

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);

	virtual FReply HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	virtual void HandleDragEnter(const FDragDropEvent& DragDropEvent);
	virtual void HandleDragLeave(const FDragDropEvent& DragDropEvent);
	virtual FReply HandleDrop(FDragDropEvent const& DragDropEvent);

	virtual bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

	virtual void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

	virtual void GatherChildren(TArray< TSharedPtr<FHierarchyModel> >& Children);

	virtual void OnSelection() = 0;

	virtual bool IsModel(UObject* PossibleModel) const { return false; }

protected:
	virtual void GetChildren(TArray< TSharedPtr<FHierarchyModel> >& Children) = 0;

protected:

	bool bInitialized;
	TArray< TSharedPtr<FHierarchyModel> > Models;
};

class FHierarchyRoot : public FHierarchyModel
{
public:
	FHierarchyRoot(TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor);
	
	virtual ~FHierarchyRoot() {}

	/* @returns the widget name to use for the tree item */
	virtual FText GetText() const override;

	virtual const FSlateBrush* GetImage() const override;

	virtual FSlateFontInfo GetFont() const override;

	virtual void OnSelection();

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);

	virtual FReply HandleDrop(FDragDropEvent const& DragDropEvent);

protected:
	virtual void GetChildren(TArray< TSharedPtr<FHierarchyModel> >& Children) override;

private:

	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;
};

class FHierarchyWidget : public FHierarchyModel
{
public:
	FHierarchyWidget(FWidgetReference InItem, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor);

	virtual ~FHierarchyWidget() {}
	
	/* @returns the widget name to use for the tree item */
	virtual FText GetText() const override;

	virtual const FSlateBrush* GetImage() const override;

	virtual FSlateFontInfo GetFont() const override;

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	virtual FReply HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void HandleDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply HandleDrop(FDragDropEvent const& DragDropEvent) override;

	virtual bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage) override;

	virtual void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo) override;

	virtual void OnSelection();

	virtual bool IsModel(UObject* PossibleModel) const override
	{
		return Item.GetTemplate() == PossibleModel;
	}

protected:
	virtual void GetChildren(TArray< TSharedPtr<FHierarchyModel> >& Children) override;

private:
	FWidgetReference Item;

	TWeakPtr<FWidgetBlueprintEditor> BlueprintEditor;
};

/**
 * An widget item in the hierarchy tree view.
 */
class SHierarchyViewItem : public STableRow< TSharedPtr<FHierarchyModel> >
{
public:
	SLATE_BEGIN_ARGS( SHierarchyViewItem ){}
		
		/** The current text to highlight */
		SLATE_ATTRIBUTE( FText, HighlightText )

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef< STableViewBase >& InOwnerTableView, TSharedPtr<FHierarchyModel> InModel);

	// Begin SWidget
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End SWidget

private:
	FReply HandleDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void HandleDragEnter(const FDragDropEvent& DragDropEvent);
	void HandleDragLeave(const FDragDropEvent& DragDropEvent);
	FReply HandleDrop(FDragDropEvent const& DragDropEvent);

	/** Called when text is being committed to check for validity */
	bool OnVerifyNameTextChanged(const FText& InText, FText& OutErrorMessage);

	/* Called when text is committed on the node */
	void OnNameTextCommited(const FText& InText, ETextCommit::Type CommitInfo);

	/* Gets the font to use for the text item, bold for customized named items */
	FSlateFontInfo GetItemFont() const;

	/* @returns the widget name to use for the tree item */
	FText GetItemText() const;

	/* The mode that this tree item represents */
	TSharedPtr<FHierarchyModel> Model;
};
