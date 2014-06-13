// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LevelEditorMiscSettings.h: Declares the ULevelEditorMiscSettings class.
=============================================================================*/

#pragma once

#include "LevelEditorMiscSettings.generated.h"


/**
 * Implements the Level Editor's miscellaneous settings.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API ULevelEditorMiscSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Whether to automatically save after a time interval */
	UPROPERTY(EditAnywhere, config, Category=Editing, meta=(DisplayName="Apply Lighting Automatically"))
	uint32 bAutoApplyLightingEnable:1;

	/** If true, BSP will auto-update */
	UPROPERTY(EditAnywhere, config, Category=Editing, meta=( DisplayName = "Update BSP Automatically" ))
	uint32 bBSPAutoUpdate:1;

	/** If true, Navigation will auto-update */
	UPROPERTY(EditAnywhere, config, Category=Editing, meta=( DisplayName = "Update Navigation Automatically" ))
	uint32 bNavigationAutoUpdate:1;

	/** If enabled, replacing actors will respect the scale of the original actor.  Otherwise, the replaced actors will have a scale of 1.0 */
	UPROPERTY(EditAnywhere, config, Category=Editing, meta=( DisplayName = "Preserve Actor Scale on Replace" ))
	uint32 bReplaceRespectsScale:1;

public:

	UPROPERTY(EditAnywhere, config, Category=Sound)
	uint32 bAllowBackgroundAudio:1;

	/** If true audio will be enabled in the editor. Does not affect PIE **/
	UPROPERTY(config)
	uint32 bEnableRealTimeAudio:1;

	/** Global volume setting for the editor */
	UPROPERTY(config)
	float EditorVolumeLevel;

	/** Enables audio feedback for certain operations in Unreal Editor, such as entering and exiting Play mode */
	UPROPERTY(EditAnywhere, config, Category=Sound)
	uint32 bEnableEditorSounds:1;

protected:

	// Begin UObject overrides

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;

	// End UObject overrides
};
