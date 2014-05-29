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

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE;
	
private:
	FText GetItemText() const;
	FString GetItemTooltipText() const;

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;
	UWidget* Item;
};
