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

	FCoreDelegates::OnObjectPropertyChanged.AddRaw(this, &SUMGEditorTree::OnObjectPropertyChanged);

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

	FCoreDelegates::OnObjectPropertyChanged.RemoveAll(this);

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

void SUMGEditorTree::TransformWidgetToString(const UWidget* Widget, OUT TArray< FString >& Array)
{
	Array.Add(Widget->GetName());
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

void SUMGEditorTree::OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	if ( !ensure(ObjectBeingModified) )
	{
		return;
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

void SUMGEditorTree::BuildWrapWithMenu(FMenuBuilder& Menu)
{
	Menu.BeginSection("WrapWith", LOCTEXT("WidgetTree_WrapWith", "Wrap With..."));
	{
		for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
		{
			UClass* WidgetClass = *ClassIt;
			if ( WidgetClass->IsChildOf(UPanelWidget::StaticClass()) && WidgetClass->HasAnyClassFlags(CLASS_Abstract) == false )
			{
				Menu.AddMenuEntry(
					WidgetClass->GetDisplayNameText(),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &SUMGEditorTree::WrapSelectedWidgets, WidgetClass),
						FCanExecuteAction()
					)
				);
			}
		}
	}
	Menu.EndSection();
}

TSharedPtr<SWidget> SUMGEditorTree::WidgetHierarchy_OnContextMenuOpening()
{
	FMenuBuilder MenuBuilder(true, NULL);

	MenuBuilder.BeginSection("Actions");
	{
		const TArray<UWidget*> SelectionList = WidgetTreeView->GetSelectedItems();

		MenuBuilder.AddMenuEntry(
			LOCTEXT("WidgetTree_Delete", "Delete"),
			LOCTEXT("WidgetTree_DeleteToolTip", "Deletes the current widget"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP(this, &SUMGEditorTree::DeleteSelected),
			FCanExecuteAction()
			)
		);

		MenuBuilder.AddSubMenu(
			LOCTEXT("WidgetTree_WrapWith", "Wrap With..."),
			LOCTEXT("WidgetTree_WrapWithToolTip", "Wraps the currently selected widgets inside of another container widget"),
			FNewMenuDelegate::CreateSP(this, &SUMGEditorTree::BuildWrapWithMenu)
		);
	}
	MenuBuilder.EndSection();

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
	return
		SNew(STableRow< UWidget* >, OwnerTable)
		.Padding(2.0f)
//		.OnDragDetected(this, &SUMGEditorWidgetTemplates::OnDraggingWidgetTemplateItem)
		[
			SNew(SUMGEditorTreeItem, BlueprintEditor.Pin(), InItem)
			.HighlightText(this, &SUMGEditorTree::GetSearchText)
		];
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

void SUMGEditorTree::WrapSelectedWidgets(UClass* WidgetClass)
{
	const TArray<UWidget*> SelectionList = WidgetTreeView->GetSelectedItems();

	TSharedPtr<FWidgetTemplateClass> Template = MakeShareable(new FWidgetTemplateClass(WidgetClass));

	UWidgetBlueprint* BP = GetBlueprint();
	UPanelWidget* NewWrapperWidget = CastChecked<UPanelWidget>( Template->Create(BP->WidgetTree) );

	int32 OutIndex;
	UPanelWidget* CurrentParent = BP->WidgetTree->FindWidgetParent(SelectionList[0], OutIndex);
	if ( CurrentParent )
	{
		CurrentParent->ReplaceChildAt(OutIndex, NewWrapperWidget);

		NewWrapperWidget->AddChild(SelectionList[0], FVector2D());
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
}

FReply SUMGEditorTree::HandleDeleteSelected()
{
	DeleteSelected();

	return FReply::Handled();
}

void SUMGEditorTree::DeleteSelected()
{
	TArray<UWidget*> SelectedItems = WidgetTreeView->GetSelectedItems();
	if ( SelectedItems.Num() > 0 )
	{
		UWidgetBlueprint* BP = GetBlueprint();

		const FScopedTransaction Transaction(LOCTEXT("RemoveWidget", "Remove Widget"));
		BP->WidgetTree->SetFlags(RF_Transactional);
		BP->WidgetTree->Modify();

		bool bRemoved = false;
		for ( UWidget* Item : SelectedItems )
		{
			bRemoved = BP->WidgetTree->RemoveWidget(Item);

			CachedExpandedWidgets.Remove(Item);
		}

		//TODO UMG There needs to be an event for widget removal so that caches can be updated, and selection

		if ( bRemoved )
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}
	}
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
	TArray<UWidget*>& WidgetTemplates = Blueprint->WidgetTree->WidgetTemplates;

	if ( WidgetTemplates.Num() > 0 )
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

		WidgetsPassingFilter.Empty( WidgetTemplates.Num() );
		if ( bIsFilterActive )
		{
			bRootPassed = FilterWidgetHierarchy(WidgetTemplates[0]);
		}

		if ( bRootPassed )
		{
			RootWidgets.Add(WidgetTemplates[0]);
		}
	}

	WidgetTreeView->RequestTreeRefresh();
}


//@TODO UMG Drop widgets onto the tree, when nothing is present, if there is a root node present, what happens then, let the root node attempt to place it?

#undef LOCTEXT_NAMESPACE
