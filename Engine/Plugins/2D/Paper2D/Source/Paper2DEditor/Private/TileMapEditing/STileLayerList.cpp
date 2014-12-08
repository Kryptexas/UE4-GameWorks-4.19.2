// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "STileLayerList.h"
#include "STileLayerItem.h"
#include "PaperStyle.h"

#include "ScopedTransaction.h"

#include "TileMapEditorCommands.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// STileLayerList

void STileLayerList::Construct(const FArguments& InArgs, UPaperTileMap* TileMap)
{
	TileMapPtr = TileMap;

	FTileMapEditorCommands::Register();
	const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();

	CommandList = MakeShareable(new FUICommandList);

	CommandList->MapAction(
		Commands.AddNewLayerAbove,
		FExecuteAction::CreateSP(this, &STileLayerList::AddNewLayerAbove));

	CommandList->MapAction(
		Commands.AddNewLayerBelow,
		FExecuteAction::CreateSP(this, &STileLayerList::AddNewLayerBelow));

	CommandList->MapAction(
		Commands.DeleteLayer,
		FExecuteAction::CreateSP(this, &STileLayerList::DeleteLayer),
		FCanExecuteAction::CreateSP(this, &STileLayerList::CanExecuteActionNeedingSelectedLayer));

	CommandList->MapAction(
		Commands.DuplicateLayer,
		FExecuteAction::CreateSP(this, &STileLayerList::DuplicateLayer),
		FCanExecuteAction::CreateSP(this, &STileLayerList::CanExecuteActionNeedingSelectedLayer));

	CommandList->MapAction(
		Commands.MergeLayerDown,
		FExecuteAction::CreateSP(this, &STileLayerList::MergeLayerDown),
		FCanExecuteAction::CreateSP(this, &STileLayerList::CanExecuteActionNeedingLayerBelow));

	CommandList->MapAction(
		Commands.MoveLayerUp,
		FExecuteAction::CreateSP(this, &STileLayerList::MoveLayerUp),
		FCanExecuteAction::CreateSP(this, &STileLayerList::CanExecuteActionNeedingLayerAbove));

	CommandList->MapAction(
		Commands.MoveLayerDown,
		FExecuteAction::CreateSP(this, &STileLayerList::MoveLayerDown),
		FCanExecuteAction::CreateSP(this, &STileLayerList::CanExecuteActionNeedingLayerBelow));
	
	FToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization("TileLayerBrowserToolbar"), TSharedPtr<FExtender>(), Orient_Horizontal, /*InForceSmallIcons=*/ true);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	ToolbarBuilder.AddToolBarButton(Commands.AddNewLayerAbove);
	ToolbarBuilder.AddToolBarButton(Commands.MoveLayerUp);
	ToolbarBuilder.AddToolBarButton(Commands.MoveLayerDown);
	ToolbarBuilder.AddToolBarButton(Commands.DuplicateLayer);
	ToolbarBuilder.AddToolBarButton(Commands.DeleteLayer);

	TSharedRef<SWidget> Toolbar = ToolbarBuilder.MakeWidget();

	ListViewWidget = SNew(SPaperLayerListView)
		.SelectionMode(ESelectionMode::Single)
		.ClearSelectionOnClick(false)
		.ListItemsSource(&(TileMap->TileLayers))
		.OnSelectionChanged(this, &STileLayerList::OnSelectionChanged)
		.OnGenerateRow(this, &STileLayerList::OnGenerateLayerListRow)
		.OnContextMenuOpening(this, &STileLayerList::OnConstructContextMenu);

	// Select the top item by default
	if (TileMap->TileLayers.Num() > 0)
	{
		SetSelectedLayer(TileMap->TileLayers[TileMap->TileLayers.Num() - 1]);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SNew(SBox)
			.HeightOverride(115.0f)
			[
				ListViewWidget.ToSharedRef()
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			Toolbar
		]
	];
}

TSharedRef<ITableRow> STileLayerList::OnGenerateLayerListRow(class UPaperTileLayer* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	typedef STableRow<class UPaperTileLayer*> RowType;

	TSharedRef<RowType> NewRow = SNew(RowType, OwnerTable)
		.Style(&FPaperStyle::Get()->GetWidgetStyle<FTableRowStyle>("TileMapEditor.LayerBrowser.TableViewRow"));

	FIsSelected IsSelectedDelegate = FIsSelected::CreateSP(NewRow, &RowType::IsSelectedExclusively);
	NewRow->SetContent(SNew(STileLayerItem, Item, IsSelectedDelegate));

	return NewRow;
}

UPaperTileLayer* STileLayerList::GetSelectedLayer() const
{
	return (ListViewWidget->GetNumItemsSelected() > 0) ? ListViewWidget->GetSelectedItems()[0] : nullptr;
}

FText STileLayerList::GenerateNewLayerName(UPaperTileMap* TileMap)
{
	// Create a set of existing names
	TSet<FString> ExistingNames;
	for (UPaperTileLayer* ExistingLayer : TileMap->TileLayers)
	{
		ExistingNames.Add(ExistingLayer->LayerName.ToString());
	}

	// Find a good name
	FText TestLayerName;
	do
	{
		TileMap->LayerNameIndex++;

		FNumberFormattingOptions NoGroupingFormat;
		NoGroupingFormat.SetUseGrouping(false);

		TestLayerName = FText::Format(LOCTEXT("NewLayerNameFormatString", "Layer {0}"), FText::AsNumber(TileMap->LayerNameIndex, &NoGroupingFormat));
	} while (ExistingNames.Contains(TestLayerName.ToString()));

	return TestLayerName;
}

FText STileLayerList::GenerateDuplicatedLayerName(const FString& InputNameRaw, UPaperTileMap* TileMap)
{
	// Create a set of existing names
	TSet<FString> ExistingNames;
	for (UPaperTileLayer* ExistingLayer : TileMap->TileLayers)
	{
		ExistingNames.Add(ExistingLayer->LayerName.ToString());
	}

	FString BaseName = InputNameRaw;
	int32 TestIndex = 0;
	bool bAddNumber = false;

	// See if this is the result of a previous duplication operation, and change the desired name accordingly
	int32 SpaceIndex;
	if (InputNameRaw.FindLastChar(' ', /*out*/ SpaceIndex))
	{
		FString PossibleDuplicationSuffix = InputNameRaw.Mid(SpaceIndex + 1);

		if (PossibleDuplicationSuffix == TEXT("copy"))
		{
			bAddNumber = true;
			BaseName = InputNameRaw.Left(SpaceIndex);
			TestIndex = 2;
		}
		else
		{
			int32 ExistingIndex = FCString::Atoi(*PossibleDuplicationSuffix);

			const FString TestSuffix = FString::Printf(TEXT(" copy %d"), ExistingIndex);

			if (InputNameRaw.EndsWith(TestSuffix))
			{
				bAddNumber = true;
				BaseName = InputNameRaw.Left(InputNameRaw.Len() - TestSuffix.Len());
				TestIndex = ExistingIndex + 1;
			}
		}
	}

	// Find a good name
	FString TestLayerName = BaseName + TEXT(" copy");

	if (bAddNumber || ExistingNames.Contains(TestLayerName))
	{
		do
		{
			TestLayerName = FString::Printf(TEXT("%s copy %d"), *BaseName, TestIndex++);
		} while (ExistingNames.Contains(TestLayerName));
	}

	return FText::FromString(TestLayerName);
}

UPaperTileLayer* STileLayerList::AddLayer(bool bCollisionLayer, int32 InsertionIndex)
{
	UPaperTileLayer* NewLayer = NULL;

	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const FScopedTransaction Transaction(LOCTEXT("TileMapAddLayer", "Add New Layer"));
		TileMap->SetFlags(RF_Transactional);
		TileMap->Modify();

		// Create the new layer
		NewLayer = NewObject<UPaperTileLayer>(TileMap);
		NewLayer->SetFlags(RF_Transactional);

		NewLayer->LayerWidth = TileMap->MapWidth;
		NewLayer->LayerHeight = TileMap->MapHeight;
		NewLayer->DestructiveAllocateMap(NewLayer->LayerWidth, NewLayer->LayerHeight);
		NewLayer->LayerName = GenerateNewLayerName(TileMap);
		NewLayer->bCollisionLayer = bCollisionLayer;

		// Insert the new layer
		if (TileMap->TileLayers.IsValidIndex(InsertionIndex))
		{
			TileMap->TileLayers.Insert(NewLayer, InsertionIndex);
		}
		else
		{
			TileMap->TileLayers.Add(NewLayer);
		}

		ListViewWidget->RequestListRefresh();
		TileMap->PostEditChange();

		// Change the selection set to select it
		SetSelectedLayer(NewLayer);
	}

	return NewLayer;
}

void STileLayerList::ChangeLayerOrdering(int32 OldIndex, int32 NewIndex)
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		if (TileMap->TileLayers.IsValidIndex(OldIndex) && TileMap->TileLayers.IsValidIndex(NewIndex))
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapReorderLayer", "Reorder Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			UPaperTileLayer* LayerToMove = TileMap->TileLayers[OldIndex];
			TileMap->TileLayers.RemoveAt(OldIndex);
			TileMap->TileLayers.Insert(LayerToMove, NewIndex);

			ListViewWidget->RequestListRefresh();
			TileMap->PostEditChange();
		}
	}
}

void STileLayerList::AddNewLayerAbove()
{
	AddLayer(/*bCollisionLayer=*/ false, GetSelectionIndex());
}

void STileLayerList::AddNewLayerBelow()
{
	AddLayer(/*bCollisionLayer=*/ false, GetSelectionIndex() + 1);
}

int32 STileLayerList::GetSelectionIndex() const
{
	int32 SelectionIndex = INDEX_NONE;

	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		if (UPaperTileLayer* SelectedLayer = GetSelectedLayer())
		{
			TileMap->TileLayers.Find(SelectedLayer, /*out*/ SelectionIndex);
		}
		else
		{
			SelectionIndex = TileMap->TileLayers.Num() - 1;
		}
	}

	return SelectionIndex;
}

void STileLayerList::DeleteLayer()
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const int32 DeleteIndex = GetSelectionIndex();
		if (DeleteIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapDeleteLayer", "Delete Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			TileMap->TileLayers.RemoveAt(DeleteIndex);

			TileMap->PostEditChange();
			ListViewWidget->RequestListRefresh();

			// Select the item below the one that just got deleted
			const int32 NewSelectionIndex = FMath::Min<int32>(DeleteIndex, TileMap->TileLayers.Num() - 1);
			if (TileMap->TileLayers.IsValidIndex(NewSelectionIndex))
			{
				SetSelectedLayer(TileMap->TileLayers[NewSelectionIndex]);
			}
		}
	}
}

void STileLayerList::DuplicateLayer()
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		const int32 DuplicateIndex = GetSelectionIndex();
		if (DuplicateIndex != INDEX_NONE)
		{
			const FScopedTransaction Transaction(LOCTEXT("TileMapDuplicateLayer", "Duplicate Layer"));
			TileMap->SetFlags(RF_Transactional);
			TileMap->Modify();

			UPaperTileLayer* NewLayer = DuplicateObject<UPaperTileLayer>(TileMap->TileLayers[DuplicateIndex], TileMap);
			TileMap->TileLayers.Insert(NewLayer, DuplicateIndex);
			NewLayer->LayerName = GenerateDuplicatedLayerName(NewLayer->LayerName.ToString(), TileMap);

			TileMap->PostEditChange();
			ListViewWidget->RequestListRefresh();

			// Select the duplicated layer
			SetSelectedLayer(NewLayer);
		}
	}
}

void STileLayerList::MergeLayerDown()
{
	//@TODO: TILEMAPS: Support merging layers down
}

void STileLayerList::MoveLayerUp()
{
	const int32 SelectedIndex = GetSelectionIndex();
	const int32 NewIndex = SelectedIndex - 1;
	ChangeLayerOrdering(SelectedIndex, NewIndex);
}

void STileLayerList::MoveLayerDown()
{
	const int32 SelectedIndex = GetSelectionIndex();
	const int32 NewIndex = SelectedIndex + 1;
	ChangeLayerOrdering(SelectedIndex, NewIndex);
}

int32 STileLayerList::GetNumLayers() const
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		return TileMap->TileLayers.Num();
	}
	return 0;
}

bool STileLayerList::CanExecuteActionNeedingLayerAbove() const
{
	return GetSelectionIndex() > 0;
}

bool STileLayerList::CanExecuteActionNeedingLayerBelow() const
{
	const int32 SelectedLayer = GetSelectionIndex();
	return (SelectedLayer != INDEX_NONE) && (SelectedLayer + 1 < GetNumLayers());
}

bool STileLayerList::CanExecuteActionNeedingSelectedLayer() const
{
	return GetSelectionIndex() != INDEX_NONE;
}

void STileLayerList::SetSelectedLayer(UPaperTileLayer* SelectedLayer)
{
	ListViewWidget->SetSelection(SelectedLayer);
}

void STileLayerList::OnSelectionChanged(UPaperTileLayer* ItemChangingState, ESelectInfo::Type SelectInfo)
{
	if (UPaperTileMap* TileMap = TileMapPtr.Get())
	{
		TileMap->SelectedLayerIndex = GetSelectionIndex();
	}
}

TSharedPtr<SWidget> STileLayerList::OnConstructContextMenu()
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, CommandList);

	const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();

 	MenuBuilder.BeginSection("BasicOperations");
 	{
		MenuBuilder.AddMenuEntry(Commands.DuplicateLayer);
		MenuBuilder.AddMenuEntry(Commands.DeleteLayer);
		MenuBuilder.AddMenuEntry(Commands.MergeLayerDown);
 	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE