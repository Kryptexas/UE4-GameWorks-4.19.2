// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "BlueprintEditorSettings.h"
#include "Editor/UnrealEd/Classes/Settings/EditorExperimentalSettings.h"
#include "Editor/UnrealEd/Classes/Editor/EditorUserSettings.h"

UBlueprintEditorSettings::UBlueprintEditorSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSaveOnCompile = false;
	bShowGraphInstructionText = true;
	NodeTemplateCacheCapMB = 20.f;
	bUseTargetContextForNodeMenu = true;
	bExposeAllMemberComponentFunctions = true;
	bShowContextualFavorites = false;
	bFlattenFavoritesMenus = true;
	bUseLegacyMenuingSystem = false;
	bShowDetailedCompileResults = false;
	CompileEventDisplayThresholdMs = 5;

	// settings that were moved out of experimental...
	UEditorExperimentalSettings const* ExperimentalSettings = GetDefault<UEditorExperimentalSettings>();
	bDrawMidpointArrowsInBlueprints = ExperimentalSettings->bDrawMidpointArrowsInBlueprints;

	// settings that were moved out of editor-user settings...
	UEditorUserSettings const* UserSettings = GetDefault<UEditorUserSettings>();
	bShowActionMenuItemSignatures = UserSettings->bDisplayActionListItemRefIds;
}
