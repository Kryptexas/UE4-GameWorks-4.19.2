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

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);
	virtual ~SUMGEditorTree();

private:
	UWidgetBlueprint* GetBlueprint() const;

	void OnBlueprintChanged(UBlueprint* InBlueprint);
	void OnObjectPropertyChanged(UObject* ObjectBeingModified);

	void ShowDetailsForObjects(TArray<USlateWrapperComponent*> Widgets);

	void WidgetHierarchy_OnGetChildren(USlateWrapperComponent* InParent, TArray< USlateWrapperComponent* >& OutChildren);
	TSharedRef< ITableRow > WidgetHierarchy_OnGenerateRow(USlateWrapperComponent* InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void WidgetHierarchy_OnSelectionChanged(USlateWrapperComponent* SelectedItem, ESelectInfo::Type SelectInfo);

	FReply CreateTestUI();
	void RefreshTree();
	
private:

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;

	TArray< USlateWrapperComponent* > RootWidgets;

	TSharedPtr< STreeView< USlateWrapperComponent* > > WidgetTreeView;
};
