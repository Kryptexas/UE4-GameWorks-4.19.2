// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FTileMapEditorCommands : public TCommands<FTileMapEditorCommands>
{
public:
	FTileMapEditorCommands()
		: TCommands<FTileMapEditorCommands>(
			TEXT("TileMapEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "TileMapEditor", "Tile Map Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FPaperStyle::Get()->GetStyleSetName() // Icon Style Set
			)
	{
	}

	// TCommand<> interface
	virtual void RegisterCommands() override;
	// End of TCommand<> interface

public:
	// Show toggles
	TSharedPtr<FUICommandInfo> SetShowCollision;
	
	TSharedPtr<FUICommandInfo> SetShowPivot;

	// Misc. actions
	TSharedPtr<FUICommandInfo> FocusOnTileMap;
};