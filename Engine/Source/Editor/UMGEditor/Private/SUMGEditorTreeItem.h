// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "BlueprintEditor.h"

//TODO rename SUMGEditorHierarchy
class SUMGEditorTreeItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorTreeItem ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, UWidget* InItem);

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	
private:
	FText GetItemText() const;
	FString GetItemTooltipText() const;

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;
	UWidget* Item;
};
