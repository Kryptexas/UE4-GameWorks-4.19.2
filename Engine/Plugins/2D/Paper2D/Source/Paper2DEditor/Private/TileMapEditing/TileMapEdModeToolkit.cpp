// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "EdModeTileMap.h"
#include "TileMapEdModeToolkit.h"
#include "../TileSetEditor.h"
#include "SContentReference.h"
#include "TileMapEditorCommands.h"
#include "Engine/Selection.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// FTileMapEdModeToolkit

FTileMapEdModeToolkit::FTileMapEdModeToolkit(class FEdModeTileMap* InOwningMode)
{
	TileMapEditor = InOwningMode;
}

void FTileMapEdModeToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

void FTileMapEdModeToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
}

FName FTileMapEdModeToolkit::GetToolkitFName() const
{
	return FName("TileMapToolkit");
}

FText FTileMapEdModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT( "TileMapAppLabel", "Tile Map Editor" );
}

FText FTileMapEdModeToolkit::GetToolkitName() const
{
	if ( CurrentTileSetPtr.IsValid() )
	{
		const bool bDirtyState = CurrentTileSetPtr->GetOutermost()->IsDirty();

		FFormatNamedArguments Args;
		Args.Add(TEXT("TileSetName"), FText::FromString(CurrentTileSetPtr->GetName()));
		Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString( TEXT( "*" ) ) : FText::GetEmpty());
		return FText::Format(LOCTEXT("TileMapEditAppLabel", "{TileSetName}{DirtyState}"), Args);
	}
	return GetBaseToolkitName();
}

FEdMode* FTileMapEdModeToolkit::GetEditorMode() const
{
	return TileMapEditor;
}

TSharedPtr<SWidget> FTileMapEdModeToolkit::GetInlineContent() const
{
	return MyWidget;
}

void FTileMapEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	const float ContentRefWidth = 140.0f;

	BindCommands();

	// Try to determine a good default tile set based on the current selection set
	if (UPaperTileMapComponent* TargetComponent = TileMapEditor->FindSelectedComponent())
	{
		if (TargetComponent->TileMap != nullptr)
		{
			CurrentTileSetPtr = TargetComponent->TileMap->SelectedTileSet.Get();
		}
	}

	TileSetPalette = SNew(STileSetSelectorViewport, CurrentTileSetPtr.Get(), TileMapEditor);

	// Create the contents of the editor mode toolkit
	MyWidget = 
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ))
		.Content()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.Padding(4.0f)
			[
				BuildToolBar()
			]

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				.Visibility(this, &FTileMapEdModeToolkit::GetTileSetSelectorVisibility)

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CurrentTileSetAssetToPaintWith", "Active Set"))
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Right)
					[
						SNew(SContentReference)
						.WidthOverride(ContentRefWidth)
						.AssetReference(this, &FTileMapEdModeToolkit::GetCurrentTileSet)
						.OnSetReference(this, &FTileMapEdModeToolkit::OnChangeTileSet)
						.AllowedClass(UPaperTileSet::StaticClass())
						.AllowSelectingNewAsset(true)
						.AllowClearingReference(false)
					]
				]

				+SVerticalBox::Slot()
				.FillHeight(1.0f)
				.VAlign(VAlign_Fill)
				.Padding(4.0f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						TileSetPalette.ToSharedRef()
					]
				]
			]
		];

	FModeToolkit::Init(InitToolkitHost);
}

void FTileMapEdModeToolkit::OnChangeTileSet(UObject* NewAsset)
{
	if (UPaperTileSet* NewTileSet = Cast<UPaperTileSet>(NewAsset))
	{
		if (CurrentTileSetPtr.Get() != NewTileSet)
		{
			CurrentTileSetPtr = NewTileSet;
			if (TileSetPalette.IsValid())
			{
				TileSetPalette->ChangeTileSet(NewTileSet);
			}

			// Save the newly selected tile set in the asset so it can be restored next time we edit it
			if (UPaperTileMapComponent* TargetComponent = TileMapEditor->FindSelectedComponent())
			{
				if (TargetComponent->TileMap != nullptr)
				{
					TargetComponent->TileMap->SelectedTileSet = NewTileSet;
				}
			}
		}
	}
}

UObject* FTileMapEdModeToolkit::GetCurrentTileSet() const
{
	return CurrentTileSetPtr.Get();
}

void FTileMapEdModeToolkit::BindCommands()
{
	FTileMapEditorCommands::Register();
	const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();

	ToolkitCommands->MapAction(
		Commands.SelectPaintTool,
		FExecuteAction::CreateSP(this, &FTileMapEdModeToolkit::OnSelectTool, ETileMapEditorTool::Paintbrush),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTileMapEdModeToolkit::IsToolSelected, ETileMapEditorTool::Paintbrush) );
	ToolkitCommands->MapAction(
		Commands.SelectEraserTool,
		FExecuteAction::CreateSP(this, &FTileMapEdModeToolkit::OnSelectTool, ETileMapEditorTool::Eraser),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTileMapEdModeToolkit::IsToolSelected, ETileMapEditorTool::Eraser) );
	ToolkitCommands->MapAction(
		Commands.SelectFillTool,
		FExecuteAction::CreateSP(this, &FTileMapEdModeToolkit::OnSelectTool, ETileMapEditorTool::PaintBucket),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &FTileMapEdModeToolkit::IsToolSelected, ETileMapEditorTool::PaintBucket) );

	// Selection actions
	ToolkitCommands->MapAction(
		Commands.FlipSelectionHorizontally,
		FExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::FlipSelectionHorizontally),
		FCanExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::HasValidSelection));
	ToolkitCommands->MapAction(
		Commands.FlipSelectionVertically,
		FExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::FlipSelectionVertically),
		FCanExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::HasValidSelection));
	ToolkitCommands->MapAction(
		Commands.RotateSelectionCW,
		FExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::RotateSelectionCW),
		FCanExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::HasValidSelection));
	ToolkitCommands->MapAction(
		Commands.RotateSelectionCCW,
		FExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::RotateSelectionCCW),
		FCanExecuteAction::CreateSP(TileMapEditor, &FEdModeTileMap::HasValidSelection));
}

void FTileMapEdModeToolkit::OnSelectTool(ETileMapEditorTool::Type NewTool)
{
	TileMapEditor->SetActiveTool(NewTool);
}

bool FTileMapEdModeToolkit::IsToolSelected(ETileMapEditorTool::Type QueryTool) const
{
	return (TileMapEditor->GetActiveTool() == QueryTool);
}

EVisibility FTileMapEdModeToolkit::GetTileSetSelectorVisibility() const
{
	return EVisibility::Visible;
}

TSharedRef<SWidget> FTileMapEdModeToolkit::BuildToolBar() const
{
	const FTileMapEditorCommands& Commands = FTileMapEditorCommands::Get();

	
	//@TODO: Add icons for these commands and force this toolbar to use small icons mode
	FToolBarBuilder SelectionFlipToolsToolbar(ToolkitCommands, FMultiBoxCustomization::None);
	{
		SelectionFlipToolsToolbar.AddToolBarButton(Commands.FlipSelectionHorizontally, NAME_None, LOCTEXT("FlipHorizontalShortLabel", "|X"));
		SelectionFlipToolsToolbar.AddToolBarButton(Commands.FlipSelectionVertically, NAME_None, LOCTEXT("FlipVerticalShortLabel", "|Y"));
		SelectionFlipToolsToolbar.AddToolBarButton(Commands.RotateSelectionCW, NAME_None, LOCTEXT("RotateClockwiseShortLabel", "CW"));
		SelectionFlipToolsToolbar.AddToolBarButton(Commands.RotateSelectionCCW, NAME_None, LOCTEXT("RotateCounterclockwiseShortLabel", "CCW"));
	}

	FToolBarBuilder ToolsToolbar(ToolkitCommands, FMultiBoxCustomization::None);
	{
		ToolsToolbar.AddToolBarButton(Commands.SelectPaintTool);
		ToolsToolbar.AddToolBarButton(Commands.SelectEraserTool);
		ToolsToolbar.AddToolBarButton(Commands.SelectFillTool);
	}

	return
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.FillWidth(1.f)
		.HAlign(HAlign_Left)
		.Padding(4.0f, 0.0f)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
			[
				SelectionFlipToolsToolbar.MakeWidget()
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.Padding(4.0f, 0.0f)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
			[
				ToolsToolbar.MakeWidget()
			]
		];
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
