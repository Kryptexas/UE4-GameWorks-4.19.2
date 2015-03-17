// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteEditorCommands.h"

//////////////////////////////////////////////////////////////////////////
// FSpriteEditorCommands

#define LOCTEXT_NAMESPACE ""

void FSpriteEditorCommands::RegisterCommands()
{
	// Show toggles
	UI_COMMAND(SetShowGrid, "Grid", "Displays the viewport grid.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowSourceTexture, "Src Tex", "Toggles display of the source texture (useful when it is an atlas).", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowBounds, "Bounds", "Toggles display of the bounds of the static mesh.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowCollision, "Collision", "Toggles display of the simplified collision mesh of the static mesh, if one has been assigned.", EUserInterfaceActionType::ToggleButton, FInputChord(EKeys::C, EModifierKey::Alt));

	UI_COMMAND(SetShowSockets, "Sockets", "Displays the sprite sockets.", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(SetShowNormals, "Normals", "Toggles display of vertex normals in the Preview Pane.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowPivot, "Pivot", "Display the pivot location of the sprite.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SetShowMeshEdges, "Mesh Edges", "Overlays the mesh edges on top of the view.", EUserInterfaceActionType::ToggleButton, FInputChord());

	// Editing modes
	UI_COMMAND(EnterViewMode, "View", "View the sprite.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(EnterSourceRegionEditMode, "Edit Source Region", "Edit the sprite source region.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(EnterCollisionEditMode, "Edit Collision", "Edit the collision geometry.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(EnterRenderingEditMode, "Edit RenderGeom", "Edit the rendering geometry (useful to reduce overdraw).", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(EnterAddSpriteMode, "Add Sprite", "Create new sprites quickly.", EUserInterfaceActionType::ToggleButton, FInputChord());

	// Misc. actions
	UI_COMMAND(FocusOnSprite, "Focus on sprite", "Centers and zooms the view to focus on the current sprite.", EUserInterfaceActionType::Button, FInputChord(EKeys::Home, EModifierKey::None));

	// Geometry editing commands
	UI_COMMAND(DeleteSelection, "Delete", "Delete the selection.", EUserInterfaceActionType::Button, FInputChord(EKeys::Platform_Delete, EModifierKey::None));
	UI_COMMAND(SplitEdge, "Split", "Split edge.", EUserInterfaceActionType::Button, FInputChord(EKeys::Insert, EModifierKey::None));
	UI_COMMAND(ToggleAddPolygonMode, "Add Polygon", "Adds a new polygon.", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(SnapAllVertices, "Snap to pixel grid", "Snaps all vertices to the pixel grid.", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE