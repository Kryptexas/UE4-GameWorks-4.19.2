// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SPaletteView.h"
#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"

#include "DecoratedDragDropOp.h"
#include "WidgetTemplateDragDropOp.h"

#include "WidgetTemplate.h"
#include "WidgetTemplateClass.h"

#define LOCTEXT_NAMESPACE "UMG"

class SPaletteViewItem : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPaletteViewItem) {}

		/** The current text to highlight */
		SLATE_ATTRIBUTE(FText, HighlightText)

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
			.ToolTip(Template->GetToolTip())

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
				.Image(Template->GetIcon())
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2, 0, 0, 0)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Template->Name)
				.HighlightText(InArgs._HighlightText)
			]
		];
	}

private:
	TSharedPtr<FWidgetTemplate> Template;
};

class FWidgetTemplateViewModel : public FWidgetViewModel
{
public:
	virtual ~FWidgetTemplateViewModel()
	{
	}

	virtual FText GetName() const
	{
		return Template->Name;
	}

	virtual FString GetFilterString() const
	{
		return Template->Name.ToString();
	}

	virtual TSharedRef<ITableRow> BuildRow(const TSharedRef<STableViewBase>& OwnerTable) override
	{
		return SNew(STableRow< TSharedPtr<FWidgetViewModel> >, OwnerTable)
			.Padding(2.0f)
			.OnDragDetected(this, &FWidgetTemplateViewModel::OnDraggingWidgetTemplateItem)
			[
				SNew(SPaletteViewItem, Template)
				.HighlightText(OwnerView, &SPaletteView::GetSearchText)
			];
	}

	FReply OnDraggingWidgetTemplateItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		return FReply::Handled().BeginDragDrop(FWidgetTemplateDragDropOp::New(Template));
	}

	SPaletteView* OwnerView;
	TSharedPtr<FWidgetTemplate> Template;
};

class FWidgetHeaderViewModel : public FWidgetViewModel
{
public:
	virtual ~FWidgetHeaderViewModel()
	{
	}

	virtual FText GetName() const
	{
		return GroupName;
	}

	virtual FString GetFilterString() const
	{
		// Headers should never be included in filtering to avoid showing a header with all of
		// it's widgets filtered out, so return an empty filter string.
		return TEXT("");
	}
	
	virtual TSharedRef<ITableRow> BuildRow(const TSharedRef<STableViewBase>& OwnerTable) override
	{
		return SNew(STableRow< TSharedPtr<FWidgetViewModel> >, OwnerTable)
		.Style( FEditorStyle::Get(), "UMGEditor.PaletteHeader" )
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

void SPaletteView::Construct(const FArguments& InArgs, TSharedPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditor = InBlueprintEditor;

	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddRaw(this, &SPaletteView::OnObjectsReplaced);

	UBlueprint* BP = InBlueprintEditor->GetBlueprintObj();

	WidgetFilter = MakeShareable(new WidgetViewModelTextFilter(
		WidgetViewModelTextFilter::FItemToStringArray::CreateSP(this, &SPaletteView::TransformWidgetViewModelToString)));

	FilterHandler = MakeShareable(new PaletteFilterHandler());
	FilterHandler->SetFilter(WidgetFilter.Get());
	FilterHandler->SetRootItems(&WidgetViewModels, &TreeWidgetViewModels);
	FilterHandler->SetGetChildrenDelegate(PaletteFilterHandler::FOnGetChildren::CreateRaw(this, &SPaletteView::OnGetChildren));

	SAssignNew(WidgetTemplatesView, STreeView< TSharedPtr<FWidgetViewModel> >)
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &SPaletteView::OnGenerateWidgetTemplateItem)
	.OnGetChildren(FilterHandler.ToSharedRef(), &PaletteFilterHandler::OnGetFilteredChildren)
	.TreeItemsSource(&TreeWidgetViewModels);

	FilterHandler->SetTreeView(WidgetTemplatesView.Get());

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.Padding(4)
		.AutoHeight()
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("SearchTemplates", "Search Palette"))
			.OnTextChanged(this, &SPaletteView::OnSearchChanged)
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBorder, WidgetTemplatesView.ToSharedRef())
			[
				WidgetTemplatesView.ToSharedRef()
			]
		]
	];

	bRefreshRequested = true;

	BuildWidgetList();
	LoadItemExpanssion();

	bRebuildRquested = false;
}

SPaletteView::~SPaletteView()
{
	// If the filter is enabled, disable it before saving the expanded items since
	// filtering expands all items by default.
	if (FilterHandler->GetIsEnabled())
	{
		FilterHandler->SetIsEnabled(false);
		FilterHandler->RefreshAndFilterTree();
	}

	GEditor->OnObjectsReplaced().RemoveAll( this );

	SaveItemExpansion();
}

void SPaletteView::OnSearchChanged(const FText& InFilterText)
{
	bRefreshRequested = true;
	FilterHandler->SetIsEnabled(!InFilterText.IsEmpty());
	WidgetFilter->SetRawFilterText(InFilterText);
	SearchText = InFilterText;
}

FText SPaletteView::GetSearchText() const
{
	return SearchText;
}

void SPaletteView::LoadItemExpanssion()
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

void SPaletteView::SaveItemExpansion()
{
	// Restore the expansion state of the widget groups.
	for ( TSharedPtr<FWidgetViewModel>& ViewModel : WidgetViewModels )
	{
		const bool IsExpanded = WidgetTemplatesView->IsItemExpanded(ViewModel);
		GConfig->SetBool(TEXT("WidgetTemplatesExpanded"), *ViewModel->GetName().ToString(), IsExpanded, GEditorUserSettingsIni);
	}
}

UWidgetBlueprint* SPaletteView::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return Cast<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SPaletteView::BuildWidgetList()
{
	// Clear the current list of view models and categories
	WidgetViewModels.Reset();
	WidgetTemplateCategories.Reset();

	// Generate a list of templates
	BuildClassWidgetList();
	BuildSpecialWidgetList();

	// For each entry in the category create a view model for the widget template
	for ( auto& Entry : WidgetTemplateCategories )
	{
		TSharedPtr<FWidgetHeaderViewModel> Header = MakeShareable(new FWidgetHeaderViewModel());
		Header->GroupName = FText::FromString(Entry.Key);

		for ( auto& Template : Entry.Value )
		{
			TSharedPtr<FWidgetTemplateViewModel> TemplateViewModel = MakeShareable(new FWidgetTemplateViewModel());
			TemplateViewModel->Template = Template;
			TemplateViewModel->OwnerView = this;
			Header->Children.Add(TemplateViewModel);
		}

		Header->Children.Sort([] (TSharedPtr<FWidgetViewModel> L, TSharedPtr<FWidgetViewModel> R) { return R->GetName().CompareTo(L->GetName()) > 0; });

		WidgetViewModels.Add(Header);
	}

	// Sort the view models by name
	WidgetViewModels.Sort([] (TSharedPtr<FWidgetViewModel> L, TSharedPtr<FWidgetViewModel> R) { return R->GetName().CompareTo(L->GetName()) > 0; });
}

void SPaletteView::BuildClassWidgetList()
{
	static const FName DevelopmentStatusKey(TEXT("DevelopmentStatus"));

	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		UClass* WidgetClass = *ClassIt;
		if ( WidgetClass->IsChildOf(UWidget::StaticClass()) )
		{
			// We don't want to include experimental widget classes, so filter those out.
			bool bIsExperimental, bIsEarlyAccess;
			FObjectEditorUtils::GetClassDevelopmentStatus(WidgetClass, bIsExperimental, bIsEarlyAccess);

			// Don't include skeleton classes of widgets.
			const bool bIsSkeletonClass = WidgetClass->HasAnyFlags(RF_Transient) && WidgetClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
			const bool bIsValidClass =
				!( WidgetClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) || bIsSkeletonClass || bIsExperimental );
			const bool bIsSameClass = WidgetClass->GetFName() == GetBlueprint()->GeneratedClass->GetFName();

			//TODO UMG does not prevent deep nested circular references

			// Only use non-(abstract/deprecated/old) classes and don't include the current widget blueprint class.
			if ( bIsValidClass && !bIsSameClass )
			{
				TSharedPtr<FWidgetTemplateClass> Template = MakeShareable(new FWidgetTemplateClass(WidgetClass));

				AddWidgetTemplate(Template);
			}
		}
	}
}

void SPaletteView::BuildSpecialWidgetList()
{
	//AddWidgetTemplate(MakeShareable(new FWidgetTemplateButton()));
	//AddWidgetTemplate(MakeShareable(new FWidgetTemplateCheckBox()));

	//TODO UMG Make this pluggable.
}

void SPaletteView::AddWidgetTemplate(TSharedPtr<FWidgetTemplate> Template)
{
	FString Category = Template->GetCategory().ToString();
	WidgetTemplateArray& Group = WidgetTemplateCategories.FindOrAdd(Category);
	Group.Add(Template);
}

void SPaletteView::OnGetChildren(TSharedPtr<FWidgetViewModel> Item, TArray< TSharedPtr<FWidgetViewModel> >& Children)
{
	return Item->GetChildren(Children);
}

TSharedRef<ITableRow> SPaletteView::OnGenerateWidgetTemplateItem(TSharedPtr<FWidgetViewModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return Item->BuildRow(OwnerTable);
}

void SPaletteView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( bRebuildRquested )
	{
		// Save the old expanded items temporarily
		TSet<TSharedPtr<FWidgetViewModel>> ExpandedItems;
		WidgetTemplatesView->GetExpandedItems(ExpandedItems);

		BuildWidgetList();

		// Restore the expansion state
		for ( TSharedPtr<FWidgetViewModel>& ExpandedItem : ExpandedItems )
		{
			for ( TSharedPtr<FWidgetViewModel>& ViewModel : WidgetViewModels )
			{
				if ( ViewModel->GetName().EqualTo(ExpandedItem->GetName()) )
				{
					WidgetTemplatesView->SetItemExpansion(ViewModel, true);
				}
			}
		}
	}

	if (bRefreshRequested)
	{
		bRefreshRequested = false;
		FilterHandler->RefreshAndFilterTree();
	}
}

void SPaletteView::TransformWidgetViewModelToString(TSharedPtr<FWidgetViewModel> WidgetViewModel, OUT TArray< FString >& Array)
{
	Array.Add(WidgetViewModel->GetFilterString());
}

void SPaletteView::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	//bRefreshRequested = true;
	//bRebuildRquested = true;
}

#undef LOCTEXT_NAMESPACE
