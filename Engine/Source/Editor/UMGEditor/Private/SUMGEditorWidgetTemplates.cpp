// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorWidgetTemplates.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"

#include "DecoratedDragDropOp.h"
#include "WidgetTemplateDragDropOp.h"

#include "WidgetTemplate.h"
#include "WidgetTemplateClass.h"
#include "WidgetTemplateButton.h"
#include "WidgetTemplateCheckBox.h"

#define LOCTEXT_NAMESPACE "UMG"

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

class FWidgetTemplateViewModel : public FWidgetViewModel
{
public:
	virtual FText GetName() const
	{
		return Template->Name;
	}

	virtual TSharedRef<ITableRow> BuildRow(const TSharedRef<STableViewBase>& OwnerTable) override
	{
		return SNew(STableRow< TSharedPtr<FWidgetViewModel> >, OwnerTable)
			.Padding(2.0f)
			.OnDragDetected(this, &FWidgetTemplateViewModel::OnDraggingWidgetTemplateItem)
			[
				SNew(SWidgetTemplateItem, Template)
			];
	}

	FReply OnDraggingWidgetTemplateItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		return FReply::Handled().BeginDragDrop(FWidgetTemplateDragDropOp::New(Template));
	}

	TSharedPtr<FWidgetTemplate> Template;
};

class FWidgetHeaderViewModel : public FWidgetViewModel
{
public:
	virtual FText GetName() const
	{
		return GroupName;
	}
	
	virtual TSharedRef<ITableRow> BuildRow(const TSharedRef<STableViewBase>& OwnerTable) override
	{
		return SNew(STableRow< TSharedPtr<FWidgetViewModel> >, OwnerTable)
        .Style( FEditorStyle::Get(), "UMGEditor.ToolboxHeader" )
			.Padding(2.0f)
			.ShowSelection(false)
			[
                SNew(STextBlock)
                .Text(GroupName)
			];
	}

	virtual void GetChildren(TArray< TSharedPtr<FWidgetViewModel> >& OutChildren) override
	{
		for ( TSharedPtr<FWidgetViewModel>& Child : Children )
		{
			OutChildren.Add(Child);
		}
	}

	FText GroupName;
	TArray< TSharedPtr<FWidgetViewModel> > Children;
};

void SUMGEditorWidgetTemplates::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;

	UBlueprint* BP = InBlueprintEditor->GetBlueprintObj();

	BuildWidgetList();

	//FCoreDelegates::OnObjectPropertyChanged.AddRaw(this, &SUMGEditorWidgetTemplates::OnObjectPropertyChanged);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(WidgetTemplatesView, STreeView< TSharedPtr<FWidgetViewModel> >)
			.OnGenerateRow(this, &SUMGEditorWidgetTemplates::OnGenerateWidgetTemplateItem)
			.OnGetChildren(this, &SUMGEditorWidgetTemplates::OnGetChildren)
			.TreeItemsSource(&WidgetViewModels)
			.SelectionMode(ESelectionMode::Single)
		]
	];

	LoadItemExpanssion();
}

SUMGEditorWidgetTemplates::~SUMGEditorWidgetTemplates()
{
	//FCoreDelegates::OnObjectPropertyChanged.RemoveAll(this);
	SaveItemExpansion();
}

void SUMGEditorWidgetTemplates::LoadItemExpanssion()
{
	// Restore the expansion state of the widget groups.
	for ( TSharedPtr<FWidgetViewModel>& ViewModel : WidgetViewModels )
	{
		bool IsExpanded;
		if ( GConfig->GetBool(TEXT("WidgetTemplatesExpanded"), *ViewModel->GetName().ToString(), IsExpanded, GEditorUserSettingsIni) && IsExpanded )
		{
			WidgetTemplatesView->SetItemExpansion(ViewModel, true);
		}
	}
}

void SUMGEditorWidgetTemplates::SaveItemExpansion()
{
	// Restore the expansion state of the widget groups.
	for ( TSharedPtr<FWidgetViewModel>& ViewModel : WidgetViewModels )
	{
		const bool IsExpanded = WidgetTemplatesView->IsItemExpanded(ViewModel);
		GConfig->SetBool(TEXT("WidgetTemplatesExpanded"), *ViewModel->GetName().ToString(), IsExpanded, GEditorUserSettingsIni);
	}
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
	WidgetViewModels.Reset();
	WidgetTemplateCategories.Reset();

	BuildClassWidgetList();
	BuildSpecialWidgetList();

	for ( auto& Entry : WidgetTemplateCategories )
	{
		TSharedPtr<FWidgetHeaderViewModel> Header = MakeShareable(new FWidgetHeaderViewModel());
		Header->GroupName = FText::FromString(Entry.Key);

		for ( auto& Template : Entry.Value )
		{
			TSharedPtr<FWidgetTemplateViewModel> TemplateViewModel = MakeShareable(new FWidgetTemplateViewModel());
			TemplateViewModel->Template = Template;

			Header->Children.Add(TemplateViewModel);
		}

		Header->Children.Sort([] (TSharedPtr<FWidgetViewModel> L, TSharedPtr<FWidgetViewModel> R) { return R->GetName().CompareTo(L->GetName()) > 0; });

		WidgetViewModels.Add(Header);
	}

	WidgetViewModels.Sort([] (TSharedPtr<FWidgetViewModel> L, TSharedPtr<FWidgetViewModel> R) { return R->GetName().CompareTo(L->GetName()) > 0; });
}

void SUMGEditorWidgetTemplates::BuildClassWidgetList()
{
	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		UClass* WidgetClass = *ClassIt;
		if ( WidgetClass->IsChildOf(UWidget::StaticClass()) )
		{
			const bool bIsSkeletonClass = WidgetClass->HasAnyFlags(RF_Transient) && WidgetClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
			const bool bIsValidClass =
				!( WidgetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) || bIsSkeletonClass );
			const bool bIsSameClass = WidgetClass->GetFName() == GetBlueprint()->GeneratedClass->GetFName();

			if ( bIsValidClass && !bIsSameClass )
			{
				TSharedPtr<FWidgetTemplateClass> Template = MakeShareable(new FWidgetTemplateClass(WidgetClass));
				Template->Name = WidgetClass->GetDisplayNameText();
				Template->ToolTip = WidgetClass->GetDisplayNameText();

				AddWidgetTemplate(Template);
			}
		}
	}
}

void SUMGEditorWidgetTemplates::BuildSpecialWidgetList()
{
	AddWidgetTemplate(MakeShareable(new FWidgetTemplateButton()));
	AddWidgetTemplate(MakeShareable(new FWidgetTemplateCheckBox()));

	//TODO Make this pluggable.
}

void SUMGEditorWidgetTemplates::AddWidgetTemplate(TSharedPtr<FWidgetTemplate> Template)
{
	FString Category = Template->GetCategory().ToString();
	WidgetTemplateArray& Group = WidgetTemplateCategories.FindOrAdd(Category);
	Group.Add(Template);
}

void SUMGEditorWidgetTemplates::OnGetChildren(TSharedPtr<FWidgetViewModel> Item, TArray< TSharedPtr<FWidgetViewModel> >& Children)
{
	return Item->GetChildren(Children);
}

TSharedRef<ITableRow> SUMGEditorWidgetTemplates::OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetViewModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return Item->BuildRow(OwnerTable);
}

#undef LOCTEXT_NAMESPACE
