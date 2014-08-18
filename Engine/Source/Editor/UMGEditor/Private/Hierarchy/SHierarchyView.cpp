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

	FilterHandler = MakeShareable(new TreeFilterHandler< TSharedPtr<FHierarchyModel> >());
	FilterHandler->SetFilter(SearchBoxWidgetFilter.Get());
	FilterHandler->SetRootItems(&RootWidgets, &TreeRootWidgets);
	FilterHandler->SetGetChildrenDelegate(TreeFilterHandler< TSharedPtr<FHierarchyModel> >::FOnGetChildren::CreateRaw(this, &SHierarchyView::WidgetHierarchy_OnGetChildren));

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

void SHierarchyView::TransformWidgetToString(TSharedPtr<FHierarchyModel> Item, OUT TArray< FString >& Array)
{
	Array.Add( Item->GetText().ToString() );
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
			TArray< UWidget* > Widgets;
			Widgets.Add(TemplateWidget);

			UWidget* Parent = TemplateWidget->GetParent();
			while ( Parent != NULL )
			{
				Widgets.Add(Parent);
				Parent = Parent->GetParent();
			}

			TSharedPtr<FHierarchyModel> CurrentItem = RootWidgets[0];
			WidgetTreeView->SetItemExpansion(CurrentItem, true);

			for ( int32 WidgetIndex = Widgets.Num() - 1; WidgetIndex >= 0; WidgetIndex-- )
			{
				UWidget* WidgetItem = Widgets[WidgetIndex];

				TArray< TSharedPtr<FHierarchyModel> > Children;
				CurrentItem->GatherChildren(Children);

				for ( auto C : Children )
				{
					if ( C->IsModel(WidgetItem) )
					{
						WidgetTreeView->SetItemExpansion(C, true);
						CurrentItem = C;

						if ( WidgetIndex == 0 )
						{
							WidgetTreeView->SetItemSelection(CurrentItem, true, ESelectInfo::Direct);
						}
					}
				}
			}

			WidgetTreeView->RequestScrollIntoView(CurrentItem);
		}
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

TSharedPtr<SWidget> SHierarchyView::WidgetHierarchy_OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, NULL);

	FWidgetBlueprintEditorUtils::CreateWidgetContextMenu(MenuBuilder, BlueprintEditor.Pin().ToSharedRef(), FVector2D(0, 0));

	return MenuBuilder.MakeWidget();
}

void SHierarchyView::WidgetHierarchy_OnGetChildren(TSharedPtr<FHierarchyModel> InParent, TArray< TSharedPtr<FHierarchyModel> >& OutChildren)
{
	VisibleItems.Add(InParent);

	InParent->GatherChildren(OutChildren);
}

TSharedRef< ITableRow > SHierarchyView::WidgetHierarchy_OnGenerateRow(TSharedPtr<FHierarchyModel> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SHierarchyViewItem, OwnerTable, InItem)
		.HighlightText(this, &SHierarchyView::GetSearchText);
}

void SHierarchyView::WidgetHierarchy_OnSelectionChanged(TSharedPtr<FHierarchyModel> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if ( SelectInfo != ESelectInfo::Direct )
	{
		SelectedItem->OnSelection();
	}
}

FReply SHierarchyView::HandleDeleteSelected()
{
	TSet<FWidgetReference> SelectedWidgets = BlueprintEditor.Pin()->GetSelectedWidgets();

	// Remove the selected items from the filter cache
	for (FWidgetReference& Item : SelectedWidgets)
	{
//		FilterHandler->RemoveCachedItem(Item.GetTemplate());
	}

	FWidgetBlueprintEditorUtils::DeleteWidgets(GetBlueprint(), SelectedWidgets);

	return FReply::Handled();
}

void SHierarchyView::RefreshTree()
{
	VisibleItems.Empty();

	RootWidgets.Empty();
	RootWidgets.Add( MakeShareable(new FHierarchyRoot(BlueprintEditor.Pin())) );

	FilterHandler->RefreshAndFilterTree();
}

void SHierarchyView::RebuildTreeView()
{
	SAssignNew(WidgetTreeView, STreeView< TSharedPtr<FHierarchyModel> >)
		.ItemHeight(20.0f)
		.SelectionMode(ESelectionMode::Single)
		.OnGetChildren(FilterHandler.ToSharedRef(), &TreeFilterHandler< TSharedPtr<FHierarchyModel> >::OnGetFilteredChildren)
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
		for ( TSharedPtr<FHierarchyModel> Widget : VisibleItems )
		{
			//if ( ReplacementMap.Contains(Widget) )
			{
				bRefreshRequested = true;
				bRebuildTreeRequested = true;
			}
		}
	}
}

//@TODO UMG Drop widgets onto the tree, when nothing is present, if there is a root node present, what happens then, let the root node attempt to place it?

#undef LOCTEXT_NAMESPACE
