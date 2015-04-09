// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperStyle.h"

class FSpriteGeometryEditCommands : public TCommands<FSpriteGeometryEditCommands>
{
public:
	FSpriteGeometryEditCommands()
		: TCommands<FSpriteGeometryEditCommands>(
			TEXT("SpriteGeometryEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "PaperEditor", "Sprite Geometry Editor"), // Localized context name for displaying
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
	TSharedPtr<FUICommandInfo> SetShowNormals;

	// Geometry editing commands
	TSharedPtr<FUICommandInfo> DeleteSelection;
	TSharedPtr<FUICommandInfo> SplitEdge;
	TSharedPtr<FUICommandInfo> AddBoxShape;
	TSharedPtr<FUICommandInfo> AddCircleShape;
	TSharedPtr<FUICommandInfo> ToggleAddPolygonMode;
	TSharedPtr<FUICommandInfo> SnapAllVertices;
};


class FSpriteEditorCommands : public TCommands<FSpriteEditorCommands>
{
public:
	FSpriteEditorCommands()
		: TCommands<FSpriteEditorCommands>(
			TEXT("SpriteEditor"), // Context name for fast lookup
			NSLOCTEXT("Contexts", "PaperEditor", "Sprite Editor"), // Localized context name for displaying
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
	TSharedPtr<FUICommandInfo> SetShowGrid;
	TSharedPtr<FUICommandInfo> SetShowSourceTexture;
	TSharedPtr<FUICommandInfo> SetShowBounds;
	TSharedPtr<FUICommandInfo> SetShowCollision;
	
	TSharedPtr<FUICommandInfo> SetShowSockets;

	TSharedPtr<FUICommandInfo> SetShowPivot;
	TSharedPtr<FUICommandInfo> SetShowMeshEdges;

	// Editing modes
	TSharedPtr<FUICommandInfo> EnterViewMode;
	TSharedPtr<FUICommandInfo> EnterSourceRegionEditMode;
	TSharedPtr<FUICommandInfo> EnterCollisionEditMode;
	TSharedPtr<FUICommandInfo> EnterRenderingEditMode;
};