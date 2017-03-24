// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "EditorStyleSet.h"

class FPaintModeCommands : public TCommands<FPaintModeCommands>
{
public:
	FPaintModeCommands() : TCommands<FPaintModeCommands> ( "PaintModeCommands", NSLOCTEXT("PaintMode", "CommandsName", "Paint Mode Commands"), NAME_None, FEditorStyle::GetStyleSetName()) {}

	/**
	* Initialize commands
	*/
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> NextTexture;
	TSharedPtr<FUICommandInfo> PreviousTexture;
	TSharedPtr<FUICommandInfo> CommitTexturePainting;

	TArray<TSharedPtr<FUICommandInfo>> Commands;
};