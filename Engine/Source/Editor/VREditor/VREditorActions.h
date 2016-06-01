// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Actions that can be invoked from this mode
class FVREditorActions : public TCommands<FVREditorActions>
{
public:
	FVREditorActions() : TCommands<FVREditorActions>
	(
		"VREditor",
		NSLOCTEXT("Contexts", "VREditor", "VR Editor"),
		"MainFrame", FEditorStyle::GetStyleSetName()
	)
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;
	// End of TCommands<> interface
public:

	// @todo VREditor: Register any commands we have here
	// TSharedPtr<FUICommandInfo> DoSomething;
	
};
