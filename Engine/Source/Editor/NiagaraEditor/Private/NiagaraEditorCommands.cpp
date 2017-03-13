// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorCommands.h"

#include "Commands.h"

#define LOCTEXT_NAMESPACE "NiagaraEditorCommands"

void FNiagaraEditorCommands::RegisterCommands()
{
	UI_COMMAND(Compile, "Compile", "Compile the current scripts", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(RefreshNodes, "Refresh", "Refreshes the current graph nodes, and updates pins due to external changes.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ResetSimulation, "Reset", "Resets the current simulation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND( TogglePreviewGrid, "Grid", "Toggles the preview pane's grid.", EUserInterfaceActionType::ToggleButton, FInputChord() );
	UI_COMMAND( TogglePreviewBackground, "Background", "Toggles the preview pane's background.", EUserInterfaceActionType::ToggleButton, FInputChord() );
}

#undef LOCTEXT_NAMESPACE // NiagaraEditorCommands
