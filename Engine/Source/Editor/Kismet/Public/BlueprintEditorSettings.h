// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEditorSettings.generated.h"

UCLASS(config=EditorUserSettings)
class KISMET_API UBlueprintEditorSettings
	:	public UObject
{
	GENERATED_UCLASS_BODY()

// UX Settings
public:
	/** Determines if lightweight tutorial text shows up at the top of empty blueprint graphs */
	UPROPERTY(EditAnywhere, config, Category="User Experience")
	bool bShowGraphInstructionText;
};
 