// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BaseToolkit.h"
#include "SPlacementModeTools.h"

class FPlacementModeToolkit : public FModeToolkit
{
public:

	FPlacementModeToolkit()
	{
		SAssignNew( PlacementModeTools, SPlacementModeTools );
	}

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE { return FName("PlacementMode"); }
	virtual FText GetBaseToolkitName() const OVERRIDE { return NSLOCTEXT("BuilderModeToolkit", "DisplayName", "Builder"); }
	virtual class FEdMode* GetEditorMode() const OVERRIDE { return GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Default ); }
	virtual TSharedPtr<class SWidget> GetInlineContent() const OVERRIDE { return PlacementModeTools; }

private:

	TSharedPtr<SPlacementModeTools> PlacementModeTools;
};
