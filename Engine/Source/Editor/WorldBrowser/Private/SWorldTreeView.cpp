// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "SWorldTreeView.h"
#include "SWorldTreeItem.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

void SWorldLevelsTreeView::Construct(const FArguments& InArgs)
{
	WorldModel = InArgs._InWorldModel;
	WorldModel->SelectionChanged.AddSP(this, &SWorldLevelsTreeView::OnUpdateSelection);
	WorldModel->HierarchyChanged.AddSP(this, &SWorldLevelsTreeView::RefreshView);
	WorldModel->CollectionChanged.AddSP(this, &SWorldLevelsTreeView::RefreshView);
	
	SearchBoxLevelFilter = MakeShareable(new LevelTextFilter(
			LevelTextFilter::FItemToStringArray::CreateSP(this, &SWorldLevelsTreeView::TransformLevelToString)
			)
		);

	HeaderRowWidget =
			SNew( SHeaderRow )
			.Visibility(EVisibility::Collapsed)

			/** LevelName label column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_LevelLabel )
				.FillWidth( 0.45f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(LOCTEXT("Column_LevelNameLabel", "Level"))
				]

			/** Level lock column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_Lock )
				.FixedWidth( 24.0f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(NSLOCTEXT("WorldBrowser", "Lock", "Lock"))
				]
	
			/** Level visibility column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_Visibility )
				.FixedWidth( 24.0f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(NSLOCTEXT("WorldBrowser", "Visibility", "Visibility"))
				]

			/** Level kismet column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_Kismet )
				.FixedWidth( 24.0f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(NSLOCTEXT("WorldBrowser", "Blueprint", "Open the level blueprint for this Level"))
				]

			/** Level SCC status column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_SCCStatus )
				.FixedWidth( 24.0f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(NSLOCTEXT("WorldBrowser", "SCCStatus", "Status in Source Control"))
				]

			/** Level save column */
			+ SHeaderRow::Column( HierarchyColumns::ColumnID_Save )
				.FixedWidth( 24.0f )
				.HeaderContent()
				[
					SNew(STextBlock)
						.ToolTipText(NSLOCTEXT("WorldBrowser", "Save", "Save this Level"))
				];


	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SSearchBox )
			.ToolTipText(LOCTEXT("FilterSearchToolTip", "Type here to search Levels").ToString())
			.HintText(LOCTEXT("FilterSearchHint", "Search Levels"))
			.OnTextChanged(SearchBoxLevelFilter.Get(), &LevelTextFilter::SetRawFilterText)
		]

		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(TreeWidget, SLevelsTreeWidget)
			.TreeItemsSource(&WorldModel->GetRootLevelList())
			.SelectionMode(ESelectionMode::Multi)
			.OnGenerateRow(this, &SWorldLevelsTreeView::GenerateTreeRow)
			.OnGetChildren( this, &SWorldLevelsTreeView::GetChildrenForTree)
			.OnSelectionChanged(this, &SWorldLevelsTreeView::OnSelectionChanged)
			.OnExpansionChanged(this, &SWorldLevelsTreeView::OnExpansionChanged)
			.OnMouseButtonDoubleClick(this, &SWorldLevelsTreeView::OnTreeViewMouseButtonDoubleClick)
			.OnContextMenuOpening(this, &SWorldLevelsTreeView::ConstructLevelContextMenu)
			.HeaderRow(HeaderRowWidget.ToSharedRef())
		]
	];
	
	WorldModel->AddFilter(SearchBoxLevelFilter.ToSharedRef());
	RefreshView();
}

SWorldLevelsTreeView::SWorldLevelsTreeView()
	: bUpdatingSelection(false)
{
}

SWorldLevelsTreeView::~SWorldLevelsTreeView()
{
	WorldModel->SelectionChanged.RemoveAll(this);
	WorldModel->HierarchyChanged.RemoveAll(this);
	WorldModel->CollectionChanged.RemoveAll(this);
}

void SWorldLevelsTreeView::OnExpansionChanged(TSharedPtr<FLevelModel> Item, bool bIsItemExpanded)
{
	Item->SetLevelExpansionFlag(bIsItemExpanded);
}

void SWorldLevelsTreeView::OnSelectionChanged(const TSharedPtr<FLevelModel> Item, ESelectInfo::Type SelectInfo)
{
	if (bUpdatingSelection)
	{
		return;
	}

	bUpdatingSelection = true;
	WorldModel->SetSelectedLevels(TreeWidget->GetSelectedItems());
	bUpdatingSelection = false;
}

void SWorldLevelsTreeView::OnUpdateSelection()
{
	if (bUpdatingSelection)
	{
		return;
	}

	bUpdatingSelection = true;
	const auto& SelectedItems = WorldModel->GetSelectedLevels();
	TreeWidget->ClearSelection();
	for (auto It = SelectedItems.CreateConstIterator(); It; ++It)
	{
		TreeWidget->SetItemSelection(*It, true);
	}

	if (SelectedItems.Num() == 1)
	{
		TreeWidget->RequestScrollIntoView(SelectedItems[0]);
	}
	
	RefreshView();

	bUpdatingSelection = false;
}

void SWorldLevelsTreeView::OnTreeViewMouseButtonDoubleClick(TSharedPtr<FLevelModel> Item)
{
	Item->MakeLevelCurrent();
}

bool SWorldLevelsTreeView::SupportsKeyboardFocus() const
{
	return true;
}

FReply SWorldLevelsTreeView::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	if (WorldModel->GetCommandList()->ProcessCommandBindings(InKeyboardEvent))
	{
		return FReply::Handled();
	}
	
	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyboardEvent);
}

void SWorldLevelsTreeView::RefreshView()
{
	TreeWidget->RequestTreeRefresh();
	
	// Sync items expansion state
	struct FExpander : public FLevelModelVisitor 
	{
		TSharedPtr<SLevelsTreeWidget> TreeWidget;
		virtual void Visit(FLevelModel& Item) OVERRIDE
		{
			TreeWidget->SetItemExpansion(Item.AsShared(), Item.GetLevelExpansionFlag());
		};
	} Expander;
	Expander.TreeWidget = TreeWidget;
		
	//Apply expansion
	WorldModel->IterateHierarchy(Expander);
}

TSharedRef<ITableRow> SWorldLevelsTreeView::GenerateTreeRow(TSharedPtr<FLevelModel> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Item.IsValid());

	return SNew(SWorldTreeItem, OwnerTable)
			.InWorldModel(WorldModel)
			.InItemModel(Item)
			.IsItemExpanded(this, &SWorldLevelsTreeView::IsTreeItemExpanded, Item)
			.HighlightText(this, &SWorldLevelsTreeView::GetSearchBoxText);
}

void SWorldLevelsTreeView::GetChildrenForTree(TSharedPtr<FLevelModel> Item, FLevelModelList& OutChildren)
{
	OutChildren = Item->GetChildren();
}

bool SWorldLevelsTreeView::IsTreeItemExpanded(TSharedPtr<FLevelModel> Item) const
{
	return Item->GetLevelExpansionFlag();
}

TSharedPtr<SWidget> SWorldLevelsTreeView::ConstructLevelContextMenu() const
{
	if (!WorldModel->IsReadOnly())
	{
		FMenuBuilder MenuBuilder(true, WorldModel->GetCommandList());
		WorldModel->BuildHierarchyMenu(MenuBuilder);
		return MenuBuilder.MakeWidget();
	}

	return SNullWidget::NullWidget;	
}

void SWorldLevelsTreeView::TransformLevelToString(const TSharedPtr<FLevelModel>& Level, TArray<FString>& OutSearchStrings) const
{
	if (Level.IsValid() && Level->HasValidPackage())
	{
		OutSearchStrings.Add(FPackageName::GetShortName(Level->GetLongPackageName()));
	}
}

FText SWorldLevelsTreeView::GetSearchBoxText() const
{
	return SearchBoxLevelFilter->GetRawFilterText();
}


#undef LOCTEXT_NAMESPACE