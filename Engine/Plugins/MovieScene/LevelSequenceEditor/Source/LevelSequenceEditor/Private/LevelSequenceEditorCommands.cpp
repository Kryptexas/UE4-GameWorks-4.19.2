// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "LevelSequenceEditorCommands.h"

#define LOCTEXT_NAMESPACE "LevelSequenceEditorCommands"

FLevelSequenceEditorCommands::FLevelSequenceEditorCommands()
	: TCommands<FLevelSequenceEditorCommands>("LevelSequenceEditor", LOCTEXT("LevelSequenceEditor", "Level Sequence Editor"), NAME_None, "LevelSequenceEditorStyle")
{
}

void FLevelSequenceEditorCommands::RegisterCommands()
{
	UI_COMMAND(CreateNewLevelSequenceInLevel, "Add Level Sequence", "Create a new level sequence asset, and place an instance of it in this level", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ToggleImmersive, "Immersive Mode", "Switches this cinematic viewport between immersive mode and regular mode", EUserInterfaceActionType::ToggleButton, FInputChord( EKeys::F11 ) );
}

#undef LOCTEXT_NAMESPACE