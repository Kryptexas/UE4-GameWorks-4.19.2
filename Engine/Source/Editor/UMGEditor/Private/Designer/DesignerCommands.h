// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

/**
 * 
 */
class FDesignerCommands : public TCommands<FDesignerCommands>
{
public:
	FDesignerCommands()
		: TCommands<FDesignerCommands>
		(
			TEXT("WidgetDesigner"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "DesignerCommands", "Common Designer Commands"), // Localized context name for displaying
			NAME_None, // Parent
			FEditorStyle::GetStyleSetName() // Icon Style Set
		)
	{
	}
	
	/** Layout Mode */
	TSharedPtr< FUICommandInfo > LayoutTransform;

	/** Render Mode */
	TSharedPtr< FUICommandInfo > RenderTransform;

public:
	/** Registers our commands with the binding system */
	virtual void RegisterCommands() override;
};


