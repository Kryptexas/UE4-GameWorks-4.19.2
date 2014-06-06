// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

//TODO rename SUMGEditorHierarchy
class SUMGEditorTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTree ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);
	virtual ~SUMGEditorTree();

private:
	UWidgetBlueprint* GetBlueprint() const;

	void OnBlueprintChanged(UBlueprint* InBlueprint);
	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	void ShowDetailsForObjects(TArray<UWidget*> Widgets);

	void BuildWrapWithMenu(FMenuBuilder& Menu);
	TSharedPtr<SWidget> WidgetHierarchy_OnContextMenuOpening();
	void WidgetHierarchy_OnGetChildren(UWidget* InParent, TArray< UWidget* >& OutChildren);
	TSharedRef< ITableRow > WidgetHierarchy_OnGenerateRow(UWidget* InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void WidgetHierarchy_OnSelectionChanged(UWidget* SelectedItem, ESelectInfo::Type SelectInfo);

	FReply CreateTestUI();
	void RefreshTree();

	FReply HandleDeleteSelected();
	void DeleteSelected();

	void WrapSelectedWidgets(UClass* WidgetClass);
	
private:

	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;

	TArray< UWidget* > RootWidgets;

	TSharedPtr< STreeView< UWidget* > > WidgetTreeView;
};
