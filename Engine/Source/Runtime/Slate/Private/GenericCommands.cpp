// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

void FGenericCommands::RegisterCommands()
{
	UI_COMMAND( Cut, "Cut", "Cut selection", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::X ) )
	UI_COMMAND( Copy, "Copy", "Copy selection", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::C ) )
	UI_COMMAND( Paste, "Paste", "Paste clipboard contents", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::V ) )
	UI_COMMAND( Duplicate, "Duplicate", "Duplicate selection", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::W ) )
	UI_COMMAND( Undo, "Undo", "Undo last action", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::Z ) )
	UI_COMMAND( Redo, "Redo", "Redo last action", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::Y ) )

	UI_COMMAND(Delete, "Delete", "Delete current selection", EUserInterfaceActionType::Button, FInputGesture(EKeys::Platform_Delete))
	
	UI_COMMAND( Rename, "Rename", "Rename current selection", EUserInterfaceActionType::Button, FInputGesture( EKeys::F2 ) )
	UI_COMMAND( SelectAll, "Select All", "Select everything in the current scope", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::A ) )
}
