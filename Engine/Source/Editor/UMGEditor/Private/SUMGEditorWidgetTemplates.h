// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "IUMGEditor.h"
#include "BlueprintEditor.h"

#include "WidgetTemplate.h"

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
	void BuildClassWidgetList();
	void BuildSpecialWidgetList();

	TSharedRef<ITableRow> OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetTemplate> Item, const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnDraggingWidgetTemplateItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

private:
	void AddWidgetTemplate(TSharedPtr<FWidgetTemplate> Template);

	TWeakPtr<class FBlueprintEditor> BlueprintEditor;

	typedef TArray< TSharedPtr<FWidgetTemplate> > WidgetTemplateArray;
	TMap< FString, WidgetTemplateArray > WidgetTemplateCategories;
	WidgetTemplateArray WidgetTemplates;

	TSharedPtr< SListView< TSharedPtr<FWidgetTemplate> > > WidgetsListView;
};
