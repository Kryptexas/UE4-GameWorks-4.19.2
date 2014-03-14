// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSpriteEditorCommands : public TCommands<FSpriteEditorCommands>
{
public:
	FSpriteEditorCommands()
		: TCommands<FSpriteEditorCommands>(
			TEXT("SpriteEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "PaperEditor", "Sprite Editor"), // Localized context name for displaying
			NAME_None, // Parent
			FEditorStyle::GetStyleSetName() // Icon Style Set
			)
	{
	}

	// TCommand<> interface
	virtual void RegisterCommands() OVERRIDE;
	// End of TCommand<> interface

public:
	// Show toggles
	TSharedPtr<FUICommandInfo> SetShowGrid;
	TSharedPtr<FUICommandInfo> SetShowSourceTexture;
	TSharedPtr<FUICommandInfo> SetShowBounds;
	TSharedPtr<FUICommandInfo> SetShowCollision;
	
	TSharedPtr<FUICommandInfo> SetShowSockets;

	TSharedPtr<FUICommandInfo> SetShowNormals;
	TSharedPtr<FUICommandInfo> SetShowPivot;

	// Editing modes
	TSharedPtr<FUICommandInfo> EnterViewMode;
	TSharedPtr<FUICommandInfo> EnterCollisionEditMode;
	TSharedPtr<FUICommandInfo> EnterRenderingEditMode;
	TSharedPtr<FUICommandInfo> EnterAddSpriteMode;

	// Misc. actions
	TSharedPtr<FUICommandInfo> FocusOnSprite;

	// Geometry editing commands
	TSharedPtr<FUICommandInfo> DeleteSelection;
	TSharedPtr<FUICommandInfo> SplitEdge;
	TSharedPtr<FUICommandInfo> AddPolygon;
	TSharedPtr<FUICommandInfo> SnapAllVertices;
};