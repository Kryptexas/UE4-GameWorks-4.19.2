// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OutputLogPrivatePCH.h"
#include "SOutputLog.h"
#include "OutputLogActions.h"

void FOutputLogCommandsImpl::RegisterCommands()
{
	UI_COMMAND( CopyOutputLog, "Copy", "Copies text from the selected message lines to the clipboard", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::C ) );
	UI_COMMAND( SelectAllInOutputLog, "Select All", "Selects all log messages", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control, EKeys::A ) );
	UI_COMMAND( SelectNoneInOutputLog, "Select None", "Deselects all log messages", EUserInterfaceActionType::Button, FInputGesture() );
	UI_COMMAND( ClearOutputLog, "Clear Log", "Clears all log messages", EUserInterfaceActionType::Button, FInputGesture() );
}

void FOutputLogCommands::Register()
{
	return FOutputLogCommandsImpl::Register();
}

const FOutputLogCommandsImpl& FOutputLogCommands::Get()
{
	return FOutputLogCommandsImpl::Get();
}

void FOutputLogCommands::Unregister()
{
	return FOutputLogCommandsImpl::Unregister();
}

