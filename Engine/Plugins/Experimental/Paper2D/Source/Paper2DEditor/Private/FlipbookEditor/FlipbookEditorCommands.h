// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FFlipbookEditorCommands : public TCommands<FFlipbookEditorCommands>
{
public:
	FFlipbookEditorCommands()
		: TCommands<FFlipbookEditorCommands>(
			TEXT("FlipbookEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "FlipbookEditor", "Flipbook Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FEditorStyle::GetStyleSetName() // Icon Style Set
			)
	{
	}

	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	TSharedPtr<FUICommandInfo> SetShowGrid;
	TSharedPtr<FUICommandInfo> SetShowBounds;
	TSharedPtr<FUICommandInfo> SetShowCollision;

	// View Menu Commands
	TSharedPtr<FUICommandInfo> SetShowPivot;

	// Timeline commands
	TSharedPtr<FUICommandInfo> AddNewFrame;
	TSharedPtr<FUICommandInfo> AddNewFrameBefore;
	TSharedPtr<FUICommandInfo> AddNewFrameAfter;
};