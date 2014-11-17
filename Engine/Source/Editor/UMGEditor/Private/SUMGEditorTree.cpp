// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "SUMGEditorTree.h"
#include "UMGEditorActions.h"
#include "SUMGEditorTreeItem.h"

#include "PreviewScene.h"
#include "SceneViewport.h"

#include "BlueprintEditor.h"
#include "SKismetInspector.h"
#include "BlueprintEditorUtils.h"
#include "WidgetTemplateClass.h"
#include "WidgetBlueprintEditor.h"

#define LOCTEXT_NAMESPACE "UMG"

void SUMGEditorTree::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor, USimpleConstructionScript* InSCS)
{
	BlueprintEditor = InBlueprintEditor;
	bRefreshRequested = false;
	bIsFilterActive = false;

	SearchBoxWidgetFilter = MakeShareable(new WidgetTextFilter(WidgetTextFilter::FItemToStringArray::CreateSP(this, &SUMGEditorTree::TransformWidgetToString)));

	UWidgetBlueprint* Blueprint = GetBlueprint();
	Blueprint->OnChanged().AddSP(this, &SUMGEditorTree::OnBlueprintChanged);

	SAssignNew(WidgetTreeView, STreeView< UWidget* >)
	.ItemHeight(20.0f)
	.SelectionMode(ESelectionMode::Single)
	.OnGetChildren(this, &SUMGEditorTree::WidgetHierarchy_OnGetChildren)
	.OnGenerateRow(this, &SUMGEditorTree::WidgetHierarchy_OnGenerateRow)
	.OnSelectionChanged(this, &SUMGEditorTree::WidgetHierarchy_OnSelectionChanged)
	.OnContextMenuOpening(this, &SUMGEditorTree::WidgetHierarchy_OnContextMenuOpening)
	.TreeItemsSource(&RootWidgets);

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
				.OnTextChanged(this, &SUMGEditorTree::OnSearchChanged)
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SScrollBorder, WidgetTreeView.ToSharedRef())
				[
					WidgetTreeView.ToSharedRef()
				]
			]
		]
	];

	BlueprintEditor.Pin()->OnSelectedWidgetsChanged.AddRaw(this, &SUMGEditorTree::OnEditorSelectionChanged);

	bRefreshRequested = true;
}

SUMGEditorTree::~SUMGEditorTree()
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
}

void SUMGEditorTree::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( bRefreshRequested )
	{
		bRefreshRequested = false;

		RefreshTree();
	}
}

FReply SUMGEditorTree::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	BlueprintEditor.Pin()->PasteDropLocation = FVector2D(0, 0);

	if ( BlueprintEditor.Pin()->WidgetCommandList->ProcessCommandBindings(InKeyboardEvent) )
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SUMGEditorTree::TransformWidgetToString(const UWidget* Widget, OUT TArray< FString >& Array)
{
	Array.Add( Widget->GetLabel() );
}

void SUMGEditorTree::OnSearchChanged(const FText& InFilterText)
{
	bRefreshRequested = true;
	SearchBoxWidgetFilter->SetRawFilterText(InFilterText);
}

FText SUMGEditorTree::GetSearchText() const
{
	return SearchBoxWidgetFilter->GetRawFilterText();
}

void SUMGEditorTree::OnEditorSelectionChanged()
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

void SUMGEditorTree::ExpandPathToWidget(UWidget* TemplateWidget)
{
	// Expand the path leading to this widget in the tree.
	UWidget* Parent = TemplateWidget->GetParent();
	while ( Parent != NULL )
	{
		WidgetTreeView->SetItemExpansion(Parent, true);
		Parent = Parent->GetParent();
	}
}

UWidgetBlueprint* SUMGEditorTree::GetBlueprint() const
{
	if ( BlueprintEditor.IsValid() )
	{
		UBlueprint* BP = BlueprintEditor.Pin()->GetBlueprintObj();
		return CastChecked<UWidgetBlueprint>(BP);
	}

	return NULL;
}

void SUMGEditorTree::OnBlueprintChanged(UBlueprint* InBlueprint)
{
	if ( InBlueprint )
	{
		RefreshTree();
	}
}

void SUMGEditorTree::ShowDetailsForObjects(TArray<UWidget*> TemplateWidgets)
{
	TSet<FWidgetReference> SelectedWidgets;
	for ( UWidget* TemplateWidget : TemplateWidgets )
	{
		FWidgetReference Selection = FWidgetReference::FromTemplate(BlueprintEditor.Pin(), TemplateWidget);
		SelectedWidgets.Add(Selection);
	}

	BlueprintEditor.Pin()->SelectWidgets(SelectedWidgets);
}

TSharedPtr<SWidget> SUMGEditorTree::WidgetHierarchy_OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FWidgetBlueprintEditorUtils::CreateWidgetContextMenu(MenuBuilder, BlueprintEditor.Pin().ToSharedRef(), FVector2D(0, 0));

	return MenuBuilder.MakeWidget();
}

void SUMGEditorTree::WidgetHierarchy_OnGetChildren(UWidget* InParent, TArray< UWidget* >& OutChildren)
{
	UPanelWidget* Widget = Cast<UPanelWidget>(InParent);
	if ( Widget )
	{
		for ( int32 i = 0; i < Widget->GetChildrenCount(); i++ )
		{
			UWidget* Child = Widget->GetChildAt(i);
			if ( Child )
			{
				if ( bIsFilterActive && !WidgetsPassingFilter.Contains(Child) )
				{
					continue;
				}

				OutChildren.Add(Child);
			}
		}
	}
}

TSharedRef< ITableRow > SUMGEditorTree::WidgetHierarchy_OnGenerateRow(UWidget* InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SUMGEditorTreeItem, OwnerTable, BlueprintEditor.Pin(), InItem)
		.HighlightText(this, &SUMGEditorTree::GetSearchText);
}

void SUMGEditorTree::WidgetHierarchy_OnSelectionChanged(UWidget* SelectedItem, ESelectInfo::Type SelectInfo)
{
	if ( SelectInfo != ESelectInfo::Direct )
	{
		TArray<UWidget*> Items;
		Items.Add(SelectedItem);
		ShowDetailsForObjects(Items);
	}
}

FReply SUMGEditorTree::HandleDeleteSelected()
{
	TSet<FWidgetReference> SelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();
	//TArray<UWidget*> SelectedWidgets = WidgetTreeView->GetSelectedItems();

	// Remove the selected items from the widget cache
	for ( FWidgetReference& Item : SelectedWidgets )
	{
		CachedExpandedWidgets.Remove(Item.GetTemplate());
	}

	FWidgetBlueprintEditorUtils::DeleteWidgets(GetBlueprint(), SelectedWidgets);

	return FReply::Handled();
}

bool SUMGEditorTree::FilterWidgetHierarchy(UWidget* CurrentWidget)
{
	bool bAnyChildrenPass = false;

	// Iterate over children and check to see if any of them pass the current filter
	UPanelWidget* Widget = Cast<UPanelWidget>(CurrentWidget);
	if ( Widget )
	{
		for ( int32 i = 0; i < Widget->GetChildrenCount(); i++ )
		{
			UWidget* Child = Widget->GetChildAt(i);
			if ( Child )
			{
				bAnyChildrenPass |= FilterWidgetHierarchy(Child);
			}
		}
	}

	// Check to see if I pass the filter
	const bool bWidgetPass = SearchBoxWidgetFilter->PassesFilter(CurrentWidget);

	// If this particular widget passes the filter, expand every widget leading up to it.
	if ( bWidgetPass )
	{
		ExpandPathToWidget(CurrentWidget);
	}

	// If either the widget or the children pass, add the current widget
	if ( bWidgetPass || bAnyChildrenPass )
	{
		WidgetsPassingFilter.Add(CurrentWidget);
		return true;
	}

	return false;
}

void SUMGEditorTree::RefreshTree()
{
	RootWidgets.Reset();

	UWidgetBlueprint* Blueprint = GetBlueprint();

	if ( Blueprint->WidgetTree->RootWidget )
	{
		bool bRootPassed = true;

		const bool bWillFilterBeActive = !SearchBoxWidgetFilter->GetRawFilterText().IsEmpty();

		// Save the expansion state when the filter becomes active
		if ( !bIsFilterActive && bWillFilterBeActive )
		{
			CachedExpandedWidgets.Empty();
			WidgetTreeView->GetExpandedItems(CachedExpandedWidgets);
		}
		// Restore the expansion state when the filter is removed
		else if ( bIsFilterActive && !bWillFilterBeActive )
		{
			WidgetTreeView->ClearExpandedItems();
			for ( UWidget* ExpandedWidget : CachedExpandedWidgets )
			{
				WidgetTreeView->SetItemExpansion(ExpandedWidget, true);
			}
			CachedExpandedWidgets.Empty();
		}
		
		bIsFilterActive = bWillFilterBeActive;

		WidgetsPassingFilter.Empty();
		if ( bIsFilterActive )
		{
			bRootPassed = FilterWidgetHierarchy(Blueprint->WidgetTree->RootWidget);
		}

		if ( bRootPassed )
		{
			RootWidgets.Add(Blueprint->WidgetTree->RootWidget);
		}
	}

	WidgetTreeView->RequestTreeRefresh();
}


//@TODO UMG Drop widgets onto the tree, when nothing is present, if there is a root node present, what happens then, let the root node attempt to place it?

#undef LOCTEXT_NAMESPACE
