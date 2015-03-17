// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetDesignerSettings.generated.h"

/**
 * Implements the settings for the Widget Blueprint Designer.
 */
UCLASS(config=EditorUserSettings)
class UMGEDITOR_API UWidgetDesignerSettings
	: public UObject
{
	GENERATED_BODY()
public:
	UWidgetDesignerSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** If enabled, actor positions will snap to the grid. */
	UPROPERTY(EditAnywhere, config, Category=GridSnapping, meta=(DisplayName = "Enable Grid Snapping"))
	uint32 GridSnapEnabled:1;

	UPROPERTY(config)
	int32 GridSnapSize;
};
