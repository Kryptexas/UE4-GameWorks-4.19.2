// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorWidgetTemplates.h"
#include "UMGEditor.h"
#include "UMGEditorViewportClient.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"

#include "DecoratedDragDropOp.h"
#include "WidgetTemplateDragDropOp.h"

#include "WidgetTemplate.h"
#include "WidgetTemplateClass.h"
#include "WidgetTemplateButton.h"

class SWidgetTemplateItem : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SWidgetTemplateItem) {}
	SLATE_END_ARGS()

	/**
	* Constructs this widget
	*
	* @param InArgs    Declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetTemplate> InTemplate)
	{
		Template = InTemplate;

		ChildSlot
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SImage)
					.Image(Template->Icon.GetIcon())
				]
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(STextBlock)
					.Text(Template->Name)
				]
			];
	}

private:
	TSharedPtr<FWidgetTemplate> Template;
};

void SUMGEditorWidgetTemplates::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;

	UBlueprint* BP = InBlueprintEditor->GetBlueprintObj();

	BuildWidgetList();

	//FCoreDelegates::OnObjectPropertyChanged.Add( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorWidgetTemplates::OnObjectPropertyChanged) );

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(WidgetsListView, SListView< TSharedPtr<FWidgetTemplate> >)
			.OnGenerateRow(this, &SUMGEditorWidgetTemplates::OnGenerateWidgetTemplateItem)
			.ListItemsSource(&WidgetTemplates)
			.SelectionMode(ESelectionMode::Single)
		]
	];
}

SUMGEditorWidgetTemplates::~SUMGEditorWidgetTemplates()
{
	//FCoreDelegates::OnObjectPropertyChanged.Remove( FCoreDelegates::FOnObjectPropertyChanged::FDelegate::CreateRaw(this, &SUMGEditorWidgetTemplates::OnObjectPropertyChanged) );
}

UWidgetBlueprint* SUMGEditorWidgetTemplates::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return Cast<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SUMGEditorWidgetTemplates::BuildWidgetList()
{
	WidgetTemplates.Reset();
	WidgetTemplateCategories.Reset();

	BuildClassWidgetList();
	BuildSpecialWidgetList();

	for ( auto& Entry : WidgetTemplateCategories )
	{
		for ( auto& Template : Entry.Value )
		{
			WidgetTemplates.Add(Template);
		}
	}
}

void SUMGEditorWidgetTemplates::BuildClassWidgetList()
{
	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		UClass* WidgetClass = *ClassIt;
		if ( WidgetClass->IsChildOf(USlateWrapperComponent::StaticClass()) )
		{
			TSharedPtr<FWidgetTemplateClass> Template = MakeShareable(new FWidgetTemplateClass(WidgetClass));
			Template->Name = WidgetClass->GetDisplayNameText();
			Template->ToolTip = WidgetClass->GetDisplayNameText();

			AddWidgetTemplate(Template);
		}
	}
}

void SUMGEditorWidgetTemplates::BuildSpecialWidgetList()
{
	AddWidgetTemplate(MakeShareable(new FWidgetTemplateButton()));

	//TODO Make this pluggable.
}

void SUMGEditorWidgetTemplates::AddWidgetTemplate(TSharedPtr<FWidgetTemplate> Template)
{
	FString Category = Template->GetCategory().ToString();
	WidgetTemplateArray& Group = WidgetTemplateCategories.FindOrAdd(Category);
	Group.Add(Template);
}

TSharedRef<ITableRow> SUMGEditorWidgetTemplates::OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetTemplate> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow< TSharedPtr<FWidgetTemplate> >, OwnerTable)
		.Padding(2.0f)
		.OnDragDetected(this, &SUMGEditorWidgetTemplates::OnDraggingWidgetTemplateItem)
		[
			SNew(SWidgetTemplateItem, Item)
		];
}

FReply SUMGEditorWidgetTemplates::OnDraggingWidgetTemplateItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( WidgetsListView->GetNumItemsSelected() > 0 )
	{
		TSharedPtr<FWidgetTemplate> Template = WidgetsListView->GetSelectedItems()[0];
		return FReply::Handled().BeginDragDrop(FWidgetTemplateDragDropOp::New(Template));
	}

	return FReply::Unhandled();
}
