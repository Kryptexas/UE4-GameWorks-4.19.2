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
	typedef TTextFilter< const UWidget* > WidgetTextFilter;

public:
	SLATE_BEGIN_ARGS( SUMGEditorTree ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);

	virtual ~SUMGEditorTree();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

private:
	void BuildWrapWithMenu(FMenuBuilder& Menu);
	TSharedPtr<SWidget> WidgetHierarchy_OnContextMenuOpening();
	void WidgetHierarchy_OnGetChildren(UWidget* InParent, TArray< UWidget* >& OutChildren);
	TSharedRef< ITableRow > WidgetHierarchy_OnGenerateRow(UWidget* InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void WidgetHierarchy_OnSelectionChanged(UWidget* SelectedItem, ESelectInfo::Type SelectInfo);

private:
	/** @returns the current blueprint being edited */
	UWidgetBlueprint* GetBlueprint() const;

	/** Expands every item in the tree leading to this widget */
	void ExpandPathToWidget(UWidget* TemplateWidget);

	/** Called when the blueprint is structurally changed. */
	void OnBlueprintChanged(UBlueprint* InBlueprint);

	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	/** Called when the selected widget has changed.  The treeview then needs to match the new selection. */
	void OnEditorSelectionChanged();

	/** Notifies the details panel that new widgets have been selected. */
	void ShowDetailsForObjects(TArray<UWidget*> Widgets);

	/** Rebuilds the tree structure based on the current filter options */
	void RefreshTree();

	/** Handles the menu clicking to delete the selected widgets. */
	FReply HandleDeleteSelected();

	/** Deletes the selected widgets from the hierarchy */
	void DeleteSelected();

	/** Wraps the selected widget using a user selected panel */
	void WrapSelectedWidgets(UClass* WidgetClass);
	
	/** Called when the filter text is changed. */
	void OnSearchChanged(const FText& InFilterText);

	/** Gets the search text currently being used to filter the list, also used to highlight text */
	FText GetSearchText() const;

	/** Transforms the widget into a searchable string */
	void TransformWidgetToString(const UWidget* Widget, OUT TArray< FString >& Array);

	/** Builds a filterable set by recursing through the widget hierarchy */
	bool FilterWidgetHierarchy(UWidget* CurrentWidget);

private:

	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;

	TArray< UWidget* > RootWidgets;

	TSharedPtr< STreeView< UWidget* > > WidgetTreeView;

	TSharedPtr<WidgetTextFilter> SearchBoxWidgetFilter;

	TSet< UWidget* > WidgetsPassingFilter;

	TSet< UWidget* > CachedExpandedWidgets;

	bool bIsFilterActive;

	bool bRefreshRequested;
};
