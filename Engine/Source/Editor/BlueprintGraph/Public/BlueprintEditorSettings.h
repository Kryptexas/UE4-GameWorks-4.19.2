// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphPin.h" // for EBlueprintPinStyleType
#include "BlueprintEditorSettings.generated.h"


UCLASS(config=EditorUserSettings)
class BLUEPRINTGRAPH_API UBlueprintEditorSettings
	:	public UObject
{
	GENERATED_UCLASS_BODY()

// Style Settings
public:
	/** Should arrows indicating data/execution flow be drawn halfway along wires? */
	UPROPERTY(EditAnywhere, config, Category="Visual Style", meta=(DisplayName="Draw midpoint arrows in Blueprints"))
	bool bDrawMidpointArrowsInBlueprints;

// UX Settings
public:
	/** Determines if lightweight tutorial text shows up at the top of empty blueprint graphs */
	UPROPERTY(EditAnywhere, config, Category="User Experience")
	bool bShowGraphInstructionText;

// Perf Settings
public:
	/** The node template cache is used to speed up blueprint menuing. This determines the peak data size for that cache. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, config, Category="Performance", DisplayName="Node-Template Cache Cap (MB)", meta=(ClampMin="0", UIMin="0"))
	float NodeTemplateCacheCapMB;
};
 