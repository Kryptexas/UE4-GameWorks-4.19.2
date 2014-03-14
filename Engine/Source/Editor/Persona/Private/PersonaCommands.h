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

	virtual void RegisterCommands() OVERRIDE;

public:
 	TSharedPtr<FUICommandInfo> RecordAnimation;

	// Command to allow users to set the skeletons preview mesh
	TSharedPtr<FUICommandInfo> ChangeSkeletonPreviewMesh;
};
