// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PaintModeCommands.h"

#define LOCTEXT_NAMESPACE "LevelMeshPainterCommands"

void FPaintModeCommands::RegisterCommands()
{
	UI_COMMAND(NextTexture, "Next Texture", "Cycle To Next Texture", EUserInterfaceActionType::Button, FInputChord(EKeys::Period));
	Commands.Add(NextTexture);
	UI_COMMAND(PreviousTexture, "Previous Texture", "Cycle To Previous Texture", EUserInterfaceActionType::Button, FInputChord(EKeys::Comma));
	Commands.Add(PreviousTexture);

	UI_COMMAND(CommitTexturePainting, "Commit Texture Painting", "Commits Texture Painting Changes", EUserInterfaceActionType::Button, FInputChord(EKeys::C, EModifierKey::Control | EModifierKey::Shift));
	Commands.Add(CommitTexturePainting);

	UI_COMMAND(Copy, "Copy Vertex Colors", "Copies Vertex Colors from the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Copy);

	UI_COMMAND(Paste, "Paste Vertex Colors", "Tried to Paste Vertex Colors on the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Paste);

	UI_COMMAND(Remove, "Remove Vertex Colors", "Removes Vertex Colors from the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Remove);

	UI_COMMAND(Fix, "Fix Vertex Colors", "If necessary fixes Vertex Colors applied to the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Fix);
	
	UI_COMMAND(Fill, "Fill Vertex Colors", "Fills the selected Meshes with the Paint Color", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Fill);

	UI_COMMAND(Propagate, "Propagate Vertex Colors", "Propagates Instance Vertex Colors to the Source Meshes", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Propagate);

	UI_COMMAND(Import, "Import Vertex Colors", "Imports Vertex Colors from a TGA Texture File to the Selected Meshes", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Import);

	UI_COMMAND(Save, "Save Vertex Colors", "Saves the Source Meshes for the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(Save);
	
	UI_COMMAND(PropagateTexturePaint, "Propagate Texture Paint", "Propagates Modifications to the Textures", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(PropagateTexturePaint);

	UI_COMMAND(SaveTexturePaint, "Save Texture Paint", "Saves the Modified Textures for the selected Mesh Components", EUserInterfaceActionType::Button, FInputChord());
	Commands.Add(SaveTexturePaint);

	UI_COMMAND(SwitchForeAndBackgroundColor, "Switch Fore and Background Color", "Switches the Fore and Background Colors used for Vertex Painting", EUserInterfaceActionType::None, FInputChord(EKeys::X));
	Commands.Add(SaveTexturePaint);

}

#undef LOCTEXT_NAMESPACE

