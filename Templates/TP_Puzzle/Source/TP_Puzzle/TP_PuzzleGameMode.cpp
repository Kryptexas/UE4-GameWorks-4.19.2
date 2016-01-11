// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "TP_Puzzle.h"
#include "TP_PuzzleGameMode.h"
#include "TP_PuzzlePlayerController.h"

ATP_PuzzleGameMode::ATP_PuzzleGameMode()
{
	// no pawn by default
	DefaultPawnClass = NULL;
	// use our own player controller class
	PlayerControllerClass = ATP_PuzzlePlayerController::StaticClass();
}
