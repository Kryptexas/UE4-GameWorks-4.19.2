// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Persona commands */
class FPersonaCommands : public TCommands<FPersonaCommands>
{
public:
	FPersonaCommands()
		: TCommands<FPersonaCommands>( TEXT("Persona"), NSLOCTEXT("Contexts", "Persona", "Persona"), NAME_None, FEditorStyle::GetStyleSetName() )
	{
	}	

	virtual void RegisterCommands() override;

public:
 	TSharedPtr<FUICommandInfo> RecordAnimation;

	// Command to allow users to set the skeletons preview mesh
	TSharedPtr<FUICommandInfo> ChangeSkeletonPreviewMesh;

	// Command to allow users to remove unused bones (not referenced by any skeletalmesh) from the skeleton
	TSharedPtr<FUICommandInfo> RemoveUnusedBones;
};
