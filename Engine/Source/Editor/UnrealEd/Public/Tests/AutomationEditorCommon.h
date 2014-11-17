// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


//////////////////////////////////////////////////////////////////////////
//Common latent commands used for automated editor testing.


/**
* Creates a latent command which the user can either undo or redo an action.
* True will trigger Undo, False will trigger a redo
*/
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FUndoRedoCommand, bool, bUndo);