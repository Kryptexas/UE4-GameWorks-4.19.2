// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "TabCommands.h"

#define LOCTEXT_NAMESPACE ""

void FTabCommands::RegisterCommands()
{
	UI_COMMAND(CloseTab, "Close Tab", "Closes the current window's active minor tab or the current focussed major tab if no minor tab is active.)", EUserInterfaceActionType::Button, FInputChord())
}

#undef LOCTEXT_NAMESPACE