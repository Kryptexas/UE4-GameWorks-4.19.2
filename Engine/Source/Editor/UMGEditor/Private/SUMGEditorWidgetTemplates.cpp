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
	void Construct(const FArguments& InArgs, TSharedPtr<FWidgetTemplateDescriptor> InTemplate)
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
	TSharedPtr<FWidgetTemplateDescriptor> Template;
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
			SAssignNew(WidgetsListView, SListView< TSharedPtr<FWidgetTemplateDescriptor> >)
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

	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		UClass* WidgetClass = *ClassIt;
		if ( WidgetClass->IsChildOf(USlateWrapperComponent::StaticClass()) )
		{
			TSharedPtr<FWidgetTemplateDescriptor> Template = MakeShareable(new FWidgetTemplateDescriptor());
			Template->Name = WidgetClass->GetDisplayNameText();
			Template->ToolTip = WidgetClass->GetDisplayNameText();
			Template->WidgetClass = WidgetClass;

			WidgetTemplates.Add(Template);
		}
	}
}

TSharedRef<ITableRow> SUMGEditorWidgetTemplates::OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetTemplateDescriptor> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow< TSharedPtr<FWidgetTemplateDescriptor> >, OwnerTable)
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
		TSharedPtr<FWidgetTemplateDescriptor> Template = WidgetsListView->GetSelectedItems()[0];
		return FReply::Handled().BeginDragDrop(FWidgetTemplateDragDropOp::New(Template));
	}

	return FReply::Unhandled();
}
