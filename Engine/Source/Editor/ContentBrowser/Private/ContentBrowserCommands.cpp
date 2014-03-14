// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"
#include "ContentBrowserCommands.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

void FContentBrowserCommands::RegisterCommands()
{
	UI_COMMAND( DirectoryUp, "Up", "Up to parent directory", EUserInterfaceActionType::Button, FInputGesture(EModifierKey::Alt, EKeys::Up) );
}

#undef LOCTEXT_NAMESPACE