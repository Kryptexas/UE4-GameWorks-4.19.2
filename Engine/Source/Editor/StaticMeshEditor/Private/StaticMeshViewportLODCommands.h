// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

/**
 * Class containing commands for static mesh editor viewport LOD actions
 */
class FStaticMeshViewportLODCommands : public TCommands<FStaticMeshViewportLODCommands>
{
public:
	FStaticMeshViewportLODCommands()
		: TCommands<FStaticMeshViewportLODCommands>
		(
			TEXT("StaticMeshViewportLODCmd"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "StaticMeshViewportLODCmd", "Static Mesh Viewport LOD Command"), // Localized context name for displaying
			NAME_None, // Parent context name. 
			FEditorStyle::GetStyleSetName() // Icon Style Set
		)
	{
	}

	/** LOD Auto */
	TSharedPtr< FUICommandInfo > LODAuto;

	/** LOD 0 */
	TSharedPtr< FUICommandInfo > LOD0;

public:
	/** Registers our commands with the binding system */
	virtual void RegisterCommands() override;
};
