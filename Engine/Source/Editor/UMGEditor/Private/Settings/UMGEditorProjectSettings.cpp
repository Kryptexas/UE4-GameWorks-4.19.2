// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorProjectSettings.h"

UUMGEditorProjectSettings::UUMGEditorProjectSettings(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bShowWidgetsFromEngineContent = false;
	bShowWidgetsFromDeveloperContent = true;
}
