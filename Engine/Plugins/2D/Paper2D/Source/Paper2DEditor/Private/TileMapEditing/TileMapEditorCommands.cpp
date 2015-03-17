// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "TileMapEditorCommands.h"

//////////////////////////////////////////////////////////////////////////
// FTileMapEditorCommands

#define LOCTEXT_NAMESPACE ""

void FTileMapEditorCommands::RegisterCommands()
{
	UI_COMMAND(EnterTileMapEditMode, "Enable Tile Map Mode", "Enables Tile Map editing mode", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(SelectPaintTool, "Paint", "Paint", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::B));
	UI_COMMAND(SelectEraserTool, "Eraser", "Eraser", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::E));
	UI_COMMAND(SelectFillTool, "Fill", "Paint Bucket", EUserInterfaceActionType::RadioButton, FInputChord(EKeys::F));

	// Show toggles
	UI_COMMAND(SetShowCollision, "Collision", "Toggles display of the simplified collision mesh of the static mesh, if one has been assigned.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::C, EModifierKey::Alt));

	UI_COMMAND(SetShowPivot, "Pivot", "Display the pivot location of the sprite.", EUserInterfaceActionType::ToggleButton, FInputChord());

	// Misc. actions
	UI_COMMAND(FocusOnTileMap, "Focus on tile map", "Centers and zooms the view to focus on the tile map.", EUserInterfaceActionType::Button, FInputChord(EKeys::Home, EModifierKey::None));

	// Selection actions
	UI_COMMAND(FlipSelectionHorizontally, "Flip selection horizontally", "Flips the selection horizontally", EUserInterfaceActionType::Button, FInputChord(EKeys::X));
	UI_COMMAND(FlipSelectionVertically, "Flip selection vertically", "Flips the selection vertically", EUserInterfaceActionType::Button, FInputChord(EKeys::Y));
	UI_COMMAND(RotateSelectionCW, "Rotate selection clockwise", "Rotates the selection clockwise", EUserInterfaceActionType::Button, FInputChord(EKeys::Z));
	UI_COMMAND(RotateSelectionCCW, "Rotate selection counterclockwise", "Rotates the selection counterclockwise", EUserInterfaceActionType::Button, FInputChord(EKeys::Z, EModifierKey::Shift));

	// Layer actions
	UI_COMMAND(AddNewLayerAbove, "Add new layer", "Add a new layer above the current layer", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AddNewLayerBelow, "Add new layer below", "Add a new layer below the current layer", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DeleteLayer, "Delete layer", "Delete the current layer", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DuplicateLayer, "Duplicate layer", "Duplicate the current layer", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MergeLayerDown, "Merge layer down", "Merge the current layer down to the layer below it", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MoveLayerUp, "Move layer up", "Move layer up", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MoveLayerDown, "Move layer down", "Move layer down", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE