// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EditorUndoClient.h"

FEditorUndoClient::~FEditorUndoClient()
{
	GEditor->UnregisterForUndo(this);
}
