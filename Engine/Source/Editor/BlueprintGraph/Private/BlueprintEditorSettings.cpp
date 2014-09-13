// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEditorSettings.h"

UBlueprintEditorSettings::UBlueprintEditorSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bShowGraphInstructionText = true;
	NodeTemplateCacheCapMB    = 20.f;
}
