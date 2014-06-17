// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

//TODO rename SUMGEditorHierarchy

/**
 * The tree view presenting the widget hierarchy.  This allows users to edit the hierarchy of widgets easily by dragging and 
 * dropping them logically, which in some cases may be significantly easier than doing it visually in the widget designer.
 */
class SUMGEditorTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTree ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);

	virtual ~SUMGEditorTree();

private:
	/** @returns the current blueprint being edited */
	UWidgetBlueprint* GetBlueprint() const;

	/** Called when the blueprint is structurally changed. */
	void OnBlueprintChanged(UBlueprint* InBlueprint);

	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	/** Called when the selected widget has changed.  The treeview then needs to match the new selection. */
	void OnEditorSelectionChanged();

	void ShowDetailsForObjects(TArray<UWidget*> Widgets);

	void BuildWrapWithMenu(FMenuBuilder& Menu);
	TSharedPtr<SWidget> WidgetHierarchy_OnContextMenuOpening();
	void WidgetHierarchy_OnGetChildren(UWidget* InParent, TArray< UWidget* >& OutChildren);
	TSharedRef< ITableRow > WidgetHierarchy_OnGenerateRow(UWidget* InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void WidgetHierarchy_OnSelectionChanged(UWidget* SelectedItem, ESelectInfo::Type SelectInfo);

	void RefreshTree();

	FReply HandleDeleteSelected();
	void DeleteSelected();

	void WrapSelectedWidgets(UClass* WidgetClass);
	
private:

	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;

	TArray< UWidget* > RootWidgets;

	TSharedPtr< STreeView< UWidget* > > WidgetTreeView;
};
