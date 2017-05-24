// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PaintModeCommands.h"

#define LOCTEXT_NAMESPACE "LevelMeshPainterCommands"

void FPaintModeCommands::RegisterCommands()
{
	UI_COMMAND(NextTexture, "NextTexture", "Cycle To Next Texture", EUserInterfaceActionType::Button, FInputChord(EKeys::Period));
	Commands.Add(NextTexture);
	UI_COMMAND(PreviousTexture, "PreviousTexture", "Cycle To Previous Texture", EUserInterfaceActionType::Button, FInputChord(EKeys::Comma));
	Commands.Add(PreviousTexture);

	UI_COMMAND(CommitTexturePainting, "CommitTexturePainting", "Commits Texture Painting Changes", EUserInterfaceActionType::Button, FInputChord(EKeys::C, EModifierKey::Control | EModifierKey::Shift));
	Commands.Add(CommitTexturePainting);
}

#undef LOCTEXT_NAMESPACE

