// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "TP_Puzzle.h"
#include "TP_PuzzlePlayerController.h"

ATP_PuzzlePlayerController::ATP_PuzzlePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableTouchEvents = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}
