// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "EditorMiscSettings.generated.h"


/**
 * Implements the miscellaneous Editor settings.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API UEditorMiscSettings
	: public UObject
{
	GENERATED_BODY()
public:
	UEditorMiscSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

};
