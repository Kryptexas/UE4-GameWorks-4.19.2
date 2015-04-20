// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FlipbookEditorSettings.generated.h"

// Settings for the Paper2D sprite editor
UCLASS(config=EditorUserSettings)
class UFlipbookEditorSettings : public UObject
{
	GENERATED_BODY()

public:
	UFlipbookEditorSettings();

	/** Background color in the flipbook editor */
	UPROPERTY(config, EditAnywhere, Category=Background, meta=(HideAlphaChannel))
	FColor BackgroundColor;
};
