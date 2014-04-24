// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"
#include "BlueprintEditor.h"

#include "WidgetTemplateDescriptor.h"

class SUMGEditorWidgetTemplates : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorWidgetTemplates ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS);
	virtual ~SUMGEditorWidgetTemplates();

private:
	UWidgetBlueprint* GetBlueprint() const;

	void BuildWidgetList();

	TSharedRef<ITableRow> OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetTemplateDescriptor> Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnDraggingWidgetTemplateItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

private:

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;

	TArray< TSharedPtr<FWidgetTemplateDescriptor> > WidgetTemplates;
	TSharedPtr< SListView< TSharedPtr<FWidgetTemplateDescriptor> > > WidgetsListView;
};
