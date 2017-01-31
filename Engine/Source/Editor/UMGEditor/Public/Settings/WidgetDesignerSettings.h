// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "WidgetDesignerSettings.generated.h"

/**
 * Implements the settings for the Widget Blueprint Designer.
 */
UCLASS(config=EditorPerProjectUserSettings)
class UMGEDITOR_API UWidgetDesignerSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** If enabled, actor positions will snap to the grid. */
	UPROPERTY(EditAnywhere, config, Category = GridSnapping, meta = (DisplayName = "Enable Grid Snapping"))
	uint32 GridSnapEnabled:1;

	/**
	 * 
	 */
	UPROPERTY(config)
	int32 GridSnapSize;

	/**
	 * 
	 */
	UPROPERTY(EditAnywhere, config, Category = Dragging)
	bool bLockToPanelOnDragByDefault;

	/**
	 * Should the designer show outlines by default?
	 */
	UPROPERTY(EditAnywhere, config, Category = Visuals, meta = ( DisplayName = "Show Dashed Outlines By Default" ))
	bool bShowOutlines;

	/**
	 * Should the designer run the design event?  Disable this if you're seeing crashes in the designer,
	 * you may have some unsafe code running in the designer.
	 */
	UPROPERTY(EditAnywhere, config, Category = Visuals)
	bool bExecutePreConstructEvent;
};
