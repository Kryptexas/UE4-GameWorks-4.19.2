// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEditorSettings.h"
#include "Editor/UnrealEd/Classes/Settings/EditorExperimentalSettings.h"
#include "Editor/UnrealEd/Classes/Editor/EditorUserSettings.h"

UBlueprintEditorSettings::UBlueprintEditorSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bShowGraphInstructionText = true;
	NodeTemplateCacheCapMB    = 20.f;

	// settings that were moved out of experimental...
	UEditorExperimentalSettings const* ExperimentalSettings = GetDefault<UEditorExperimentalSettings>();
	bDrawMidpointArrowsInBlueprints = ExperimentalSettings->bDrawMidpointArrowsInBlueprints;

	// settings that were moved out of editor-user settings...
	UEditorUserSettings const* UserSettings = GetDefault<UEditorUserSettings>();
	bShowActionMenuItemSignatures = UserSettings->bDisplayActionListItemRefIds;
}
