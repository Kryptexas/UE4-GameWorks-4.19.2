// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"
#include "PersonaCommands.h"

void FPersonaCommands::RegisterCommands()
{
	// this is going to show twice if we have blueprint
	UI_COMMAND( RecordAnimation, "Export to Animation", "Export current graph to animation", EUserInterfaceActionType::Button, FInputGesture() );

	UI_COMMAND( ChangeSkeletonPreviewMesh, "Set Preview Mesh as Default", "Changes the skeletons default preview mesh to the current open preview mesh. The skeleton will require saving after this action.", EUserInterfaceActionType::Button, FInputGesture() );
}
