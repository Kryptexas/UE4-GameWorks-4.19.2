// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FAdvancedPreviewSceneCommands : public TCommands<FAdvancedPreviewSceneCommands>
{

public:
	FAdvancedPreviewSceneCommands() : TCommands<FAdvancedPreviewSceneCommands>
	(
		"AdvancedPreviewScene",
		NSLOCTEXT("Contexts", "AdvancedPreviewScene", "Advanced Preview Scene"),
		NAME_None,
		FEditorStyle::Get().GetStyleSetName()
	)
	{}
	
	/** Toggles environment (sky sphere) visibility */
	TSharedPtr< FUICommandInfo > ToggleEnvironment;

	/** Toggles floor visibility */
	TSharedPtr< FUICommandInfo > ToggleFloor;

	/** Toggles post processing */
	TSharedPtr< FUICommandInfo > TogglePostProcessing;

	/**
	 * Initialize commands
	 */
	virtual void RegisterCommands() override;
};
