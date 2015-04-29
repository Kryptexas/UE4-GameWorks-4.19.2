// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TileSetEditorSettings.generated.h"

// Settings for the Paper2D tile set editor
UCLASS(config=EditorPerProjectUserSettings)
class UTileSetEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UTileSetEditorSettings();

	/** Default background color for new tile set assets */
	UPROPERTY(config, EditAnywhere, Category=Background, meta=(HideAlphaChannel))
	FColor DefaultBackgroundColor;

	/** Should the grid be shown by default when the editor is opened? */
	UPROPERTY(config, EditAnywhere, Category="Tile Editor")
	bool bShowGridByDefault;
};
