// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BspModePrivatePCH.h"
#include "BspModeActions.h"

DEFINE_LOG_CATEGORY_STATIC(BspModeActions, Log, All);

#define LOCTEXT_NAMESPACE "BspModeActions"

bool FBspModeActionCallbacks::DefaultCanExecuteAction()
{
	return FSlateApplication::Get().IsNormalExecution();
}

void FBspModeActionCallbacks::ExecuteExecCommand( FString Command )
{
	GUnrealEd->Exec( GEditor->GetEditorWorldContext().World(), *Command );
}

/** UI_COMMAND takes long for the compile to optimize */
PRAGMA_DISABLE_OPTIMIZATION
void FBspModeCommands::RegisterCommands()
{
	UI_COMMAND( CSGAdd, "CSG Add", "Creates an additive bsp brush from the builder brush", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control | EModifierKey::Alt, EKeys::A ) );
	UI_COMMAND( CSGSubtract, "CSG Subtract", "Creates a subtractive bsp brush from the builder brush", EUserInterfaceActionType::Button, FInputGesture( EModifierKey::Control | EModifierKey::Alt, EKeys::S ) );

	MapCommands();
}
PRAGMA_ENABLE_OPTIMIZATION

void FBspModeCommands::MapCommands()
{
	check(!ActionList.IsValid());
	ActionList = MakeShareable( new FUICommandList );

	ActionList->MapAction(
		CSGAdd,
		FExecuteAction::CreateStatic( &FBspModeActionCallbacks::ExecuteExecCommand, FString( TEXT("BRUSH ADD") ) )
		);

	ActionList->MapAction(
		CSGSubtract,
		FExecuteAction::CreateStatic( &FBspModeActionCallbacks::ExecuteExecCommand, FString( TEXT("BRUSH SUBTRACT") ) )
		);
}

#undef LOCTEXT_NAMESPACE
