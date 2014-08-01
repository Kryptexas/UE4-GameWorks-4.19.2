// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


void FMediaAssetEditorCommands::RegisterCommands()
{
	UI_COMMAND(ForwardMedia, "Forward", "Fast forwards media playback", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(PauseMedia, "Pause", "Pause media playback", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(PlayMedia, "Play", "Start media playback", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ReverseMedia, "Reverse", "Reverses media playback", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(RewindMedia, "Rewind", "Rewinds the media to the beginning", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(StopMedia, "Stop", "Stop media playback", EUserInterfaceActionType::Button, FInputGesture());
}
