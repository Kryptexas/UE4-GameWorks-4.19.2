// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "SHierarchyView.h"
#include "SHierarchyViewItem.h"

#include "UMGEditorActions.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"
#include "WidgetTemplateClass.h"
#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

void SHierarchyView::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;
	bRebuildTreeRequested = false;

	// register for any objects replaced
	GEditor->OnObjectsReplaced().AddRaw(this, &SHierarchyView::OnObjectsReplaced);

	// Create the filter for searching in the tree
	SearchBoxWidgetFilter = MakeShareable(new WidgetTextFilter(WidgetTextFilter::FItemToStringArray::CreateSP(this, &SHierarchyView::TransformWidgetToString)));

	UWidgetBlueprint* Blueprint = GetBlueprint();
	Blueprint->OnChanged().AddRaw(this, &SHierarchyView::OnBlueprintChanged);

	FilterHandler = MakeShareable(new TreeFilterHandler<UWidget*>());
	FilterHandler->SetFilter(SearchBoxWidgetFilter.Get());
	FilterHandler->SetRootItems(&RootWidgets, &TreeRootWidgets);
	FilterHandler->SetGetChildrenDelegate(TreeFilterHandler<UWidget*>::FOnGetChildren::CreateRaw(this, &SHierarchyView::WidgetHierarchy_OnGetChildren));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.Padding(4)
			.AutoHeight()
			[
				SNew(SSearchBox)
				.HintText(LOCTEXT("SearchWidgets", "Search Widgets"))
				.OnTextChanged(this, &SHierarchyView::OnSearchChanged)
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(TreeViewArea, SBorder)
				.Padding(0)
				.BorderImage( FEditorStyle::GetBrush( "NoBrush" ) )
			]
		]
	];

	RebuildTreeView();

	BlueprintEditor.Pin()->OnSelectedWidgetsChanged.AddRaw(this, &SHierarchyView::OnEditorSelectionChanged);

	bRefreshRequested = true;
}

SHierarchyView::~SHierarchyView()
{
	UWidgetBlueprint* Blueprint = GetBlueprint();
	if ( Blueprint )
	{
		Blueprint->OnChanged().RemoveAll(this);
	}

	if ( BlueprintEditor.IsValid() )
	{
		BlueprintEditor.Pin()->OnSelectedWidgetsChanged.RemoveAll(this);
	}

	GEditor->OnObjectsReplaced().RemoveAll(this);
}

void SHierarchyView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( bRebuildTreeRequested )
	{
		bRebuildTreeRequested = false;
		RebuildTreeView();
	}

	if ( bRefreshRequested )
	{
		bRefreshRequested = false;

		RefreshTree();
	}
}

FReply SHierarchyView::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	BlueprintEditor.Pin()->PasteDropLocation = FVector2D(0, 0);

	if ( BlueprintEditor.Pin()->WidgetCommandList->ProcessCommandBindings(InKeyboardEvent) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SHierarchyView::TransformWidgetToString(UWidget* Widget, OUT TArray< FString >& Array)
{
	Array.Add( Widget->GetLabel() );
}

void SHierarchyView::OnSearchChanged(const FText& InFilterText)
{
	bRefreshRequested = true;
	FilterHandler->SetIsEnabled(!InFilterText.IsEmpty());
	SearchBoxWidgetFilter->SetRawFilterText(InFilterText);
}

FText SHierarchyView::GetSearchText() const
{
	return SearchBoxWidgetFilter->GetRawFilterText();
}

void SHierarchyView::OnEditorSelectionChanged()
{
	WidgetTreeView->ClearSelection();

	bool bFirst = true;

	// Update the selection and expansion in the tree to match the new selection
	const TSet<FWidgetReference>& SelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();
	for ( const FWidgetReference& WidgetRef : SelectedWidgets )
	{
		UWidget* TemplateWidget = WidgetRef.GetTemplate();

		if ( TemplateWidget )
		{
			WidgetTreeView->SetItemSelection(TemplateWidget, true);

			// Attempt to scroll the first widget we find into view.
			if ( bFirst )
			{
				bFirst = false;
				WidgetTreeView->RequestScrollIntoView(TemplateWidget);
			}

			ExpandPathToWidget(TemplateWidget);
		}
	}
}

void SHierarchyView::ExpandPathToWidget(UWidget* TemplateWidget)
{
	// Expand the path leading to this widget in the tree.
	UWidget* Parent = TemplateWidget->GetParent();
	while ( Parent != NULL )
	{
		WidgetTreeView->SetItemExpansion(Parent, true);
		Parent = Parent->GetParent();
	}
}

UWidgetBlueprint* SHierarchyView::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return CastChecked<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SHierarchyView::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		bRefreshRequested = true;
	}
}

void SHierarchyView::ShowDetailsForObjects(TArray<UWidget*> TemplateWidgets)
{
	TSet<FWidgetReference> SelectedWidgets;
	for ( UWidget* TemplateWidget : TemplateWidgets )
	{
		FWidgetReference Selection = FWidgetReference::FromTemplate(BlueprintEditor.Pin(), TemplateWidget);
		SelectedWidgets.Add(Selection);
	}

	BlueprintEditor.Pin()->SelectWidgets(SelectedWidgets);
}

TSharedPtr<SWidget> SHierarchyView::WidgetHierarchy_OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FWidgetBlueprintEditorUtils::CreateWidgetContextMenu(MenuBuilder, BlueprintEditor.Pin().ToSharedRef(), FVector2D(0, 0));

	return MenuBuilder.MakeWidget();
}

void SHierarchyView::WidgetHierarchy_OnGetChildren(UWidget* InParent, TArray< UWidget* >& OutChildren)
{
	VisibleItems.Add(InParent);

	UPanelWidget* Widget = Cast<UPanelWidget>(InParent);
	if ( Widget )
	{
		for ( int32 i = 0; i < Widget->GetChildrenCount(); i++ )
		{
			UWidget* Child = Widget->GetChildAt(i);
			if ( Child )
			{
				OutChildren.Add(Child);
			}
		}
	}
}

TSharedRef< ITableRow > SHierarchyView::WidgetHierarchy_OnGenerateRow(UWidget* InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FWidgetReference WidgetRef = FWidgetReference::FromTemplate(BlueprintEditor.Pin(), InItem);

	return SNew(SHierarchyViewItem, OwnerTable, BlueprintEditor.Pin(), WidgetRef)
		.HighlightText(this, &SHierarchyView::GetSearchText);
}

void SHierarchyView::WidgetHierarchy_OnSelectionChanged(UWidget* SelectedItem, ESelectInfo::Type SelectInfo)
{
	if ( SelectInfo != ESelectInfo::Direct )
	{
		TArray<UWidget*> Items;
		Items.Add(SelectedItem);
		ShowDetailsForObjects(Items);
	}
}

FReply SHierarchyView::HandleDeleteSelected()
{
	TSet<FWidgetReference> SelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();

	// Remove the selected items from the filter cache
	for (FWidgetReference& Item : SelectedWidgets)
	{
		FilterHandler->RemoveCachedItem(Item.GetTemplate());
	}

	FWidgetBlueprintEditorUtils::DeleteWidgets(GetBlueprint(), SelectedWidgets);

	return FReply::Handled();
}

void SHierarchyView::RefreshTree()
{
	VisibleItems.Empty();

	RootWidgets.Empty();
	UWidgetBlueprint* Blueprint = GetBlueprint();
	if (Blueprint->WidgetTree->RootWidget)
	{
		RootWidgets.Add(Blueprint->WidgetTree->RootWidget);
	}
	FilterHandler->RefreshAndFilterTree();
}

void SHierarchyView::RebuildTreeView()
{
	SAssignNew(WidgetTreeView, STreeView< UWidget* >)
		.ItemHeight(20.0f)
		.SelectionMode(ESelectionMode::Single)
		.OnGetChildren(FilterHandler.ToSharedRef(), &TreeFilterHandler<UWidget*>::OnGetFilteredChildren)
		.OnGenerateRow(this, &SHierarchyView::WidgetHierarchy_OnGenerateRow)
		.OnSelectionChanged(this, &SHierarchyView::WidgetHierarchy_OnSelectionChanged)
		.OnContextMenuOpening(this, &SHierarchyView::WidgetHierarchy_OnContextMenuOpening)
		.TreeItemsSource(&TreeRootWidgets);

	FilterHandler->SetTreeView(WidgetTreeView.Get());

	TreeViewArea->SetContent(
		SNew(SScrollBorder, WidgetTreeView.ToSharedRef())
		[
			WidgetTreeView.ToSharedRef()
		]);
}

void SHierarchyView::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	if ( !bRebuildTreeRequested )
	{
		for ( UWidget* Widget : VisibleItems )
		{
			if ( ReplacementMap.Contains(Widget) )
			{
				bRefreshRequested = true;
				bRebuildTreeRequested = true;
			}
		}
	}
}

//@TODO UMG Drop widgets onto the tree, when nothing is present, if there is a root node present, what happens then, let the root node attempt to place it?

#undef LOCTEXT_NAMESPACE
