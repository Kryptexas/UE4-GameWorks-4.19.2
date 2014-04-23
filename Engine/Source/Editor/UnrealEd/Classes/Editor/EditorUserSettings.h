// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorUserSettings.generated.h"

UENUM()
enum EWASDType
{
	WASD_Always UMETA(DisplayName="Use WASD for Camera Controls"),
	WASD_RMBOnly UMETA(DisplayName="Use WASD only when a Mouse Button is Pressed"),
	WASD_Never UMETA(DisplayName="Never use WASD for Camera Controls"),
	WASD_MAX,
};


UENUM()
enum ERotationGridMode
{
	/** Using Divisions of 360 degrees (e.g 360/2. 360/3, 360/4, ... ) */
	GridMode_DivisionsOf360,

	/** Uses the user defined grid values */
	GridMode_Common,
};

UCLASS(minimalapi, autoexpandcategories=(ViewportControls, ViewportLookAndFeel, LevelEditing, SourceControl, Content, Startup),
	   hidecategories=(Object, Options, Grid, RotationGrid),
	   config=EditorUserSettings)
class UEditorUserSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	// =====================================================================
	// The following options are exposed in the Preferences Editor 

	/** If enabled, any newly opened UI menus, menu bars, and toolbars will show the developer hooks that would accept extensions */
	UPROPERTY(EditAnywhere, config, Category=DeveloperTools)
	uint32 bDisplayUIExtensionPoints:1;

	/** If enabled, tooltips linked to documentation will show the developer the link bound to that UI item */
	UPROPERTY(EditAnywhere, config, Category=DeveloperTools)
	uint32 bDisplayDocumentationLink:1;

	/** If enabled, tooltips on SGraphPaletteItems will show the associated action's string id */
	UPROPERTY(EditAnywhere, config, Category=DeveloperTools)
	uint32 bDisplayActionListItemRefIds:1;
	
	/** When enabled, the application frame rate, memory and Unreal object count will be displayed in the main editor UI */
	UPROPERTY(EditAnywhere, config, Category=DeveloperTools)
	uint32 bShowFrameRateAndMemory:1;

	/** Select to make Distributions use the curves, not the baked lookup tables. */
	UPROPERTY(config)
	uint32 bUseCurvesForDistributions:1; //(GDistributionType == 0)

	/** Controls the minimum value at which the property matrix editor will display a loading bar when pasting values */
	UPROPERTY(config)
	int32 PropertyMatrix_NumberOfPasteOperationsBeforeWarning;

	/** The visual styling to use for graph editor pins (in Blueprints, materials, etc...) */
	UPROPERTY(config, EditAnywhere, Category = Blueprint)
	TEnumAsByte<EBlueprintPinStyleType> DataPinStyle;

	/**
	* Blueprint editor settings; These are not config/visible/editable unless we need to tweak them.  Make ones you want to adjust 'config, EditAnywhere, Category=Blueprint'
	*/

	/** The default color is used only for types not specifically defined below.  Generally if it's seen, it means another type needs to be defined so that the wire in question can have an appropriate color. */
	UPROPERTY()
	FLinearColor DefaultPinTypeColor;
	
	/** Execution pin type color */
	UPROPERTY()
	FLinearColor ExecutionPinTypeColor;

	/** Boolean pin type color */
	UPROPERTY()
	FLinearColor BooleanPinTypeColor;

	/** Byte pin type color */
	UPROPERTY()
	FLinearColor BytePinTypeColor;

	/** Class pin type color */
	UPROPERTY()
	FLinearColor ClassPinTypeColor;

	/** Integer pin type color */
	UPROPERTY()
	FLinearColor IntPinTypeColor;

	/** Floating-point pin type color */
	UPROPERTY()
	FLinearColor FloatPinTypeColor;

	/** Name pin type color */
	UPROPERTY()
	FLinearColor NamePinTypeColor;

	/** Delegate pin type color */
	UPROPERTY()
	FLinearColor DelegatePinTypeColor;

	/** Object pin type color */
	UPROPERTY()
	FLinearColor ObjectPinTypeColor;

	/** String pin type color */
	UPROPERTY()
	FLinearColor StringPinTypeColor;

	/** Text pin type color */
	UPROPERTY()
	FLinearColor TextPinTypeColor;

	/** Struct pin type color */
	UPROPERTY()
	FLinearColor StructPinTypeColor;

	/** Wildcard pin type color */
	UPROPERTY()
	FLinearColor WildcardPinTypeColor;

	/** Vector pin type color */
	UPROPERTY()
	FLinearColor VectorPinTypeColor;

	/** Rotator pin type color */
	UPROPERTY()
	FLinearColor RotatorPinTypeColor;

	/** Transform pin type color */
	UPROPERTY()
	FLinearColor TransformPinTypeColor;

	/** Index pin type color */
	UPROPERTY()
	FLinearColor IndexPinTypeColor;

	/** Event node title color */
	UPROPERTY()
	FLinearColor EventNodeTitleColor;

	/** CallFunction node title color */
	UPROPERTY()
	FLinearColor FunctionCallNodeTitleColor;

	/** Pure function call node title color */
	UPROPERTY()
	FLinearColor PureFunctionCallNodeTitleColor;

	/** Parent class function call node title color */
	UPROPERTY()
	FLinearColor ParentFunctionCallNodeTitleColor;

	/** Function Terminator node title color */
	UPROPERTY()
	FLinearColor FunctionTerminatorNodeTitleColor;

	/** Exec Branch node title color */
	UPROPERTY()
	FLinearColor ExecBranchNodeTitleColor;

	/** Exec Sequence node title color */
	UPROPERTY()
	FLinearColor ExecSequenceNodeTitleColor;

	/** Result node title color */
	UPROPERTY()
	FLinearColor ResultNodeTitleColor;

	UPROPERTY()
	FLinearColor TraceAttackColor;

	UPROPERTY()
	float TraceAttackWireThickness;

	/** How long is the attack color fully visible */
	UPROPERTY()
	float TraceAttackHoldPeriod;

	/** How long does it take to fade from the attack to the sustain color */
	UPROPERTY()
	float TraceDecayPeriod;

	UPROPERTY()
	float TraceDecayExponent;

	UPROPERTY()
	FLinearColor TraceSustainColor;

	UPROPERTY()
	float TraceSustainWireThickness;

	/** How long is the sustain color fully visible */
	UPROPERTY()
	float TraceSustainHoldPeriod;

	UPROPERTY()
	FLinearColor TraceReleaseColor;

	UPROPERTY()
	float TraceReleaseWireThickness;

	/** How long does it take to fade from the sustain to the release color */
	UPROPERTY()
	float TraceReleasePeriod;

	UPROPERTY()
	float TraceReleaseExponent;

	UPROPERTY(config)
	bool bSCSEditorShowGrid;

	UPROPERTY(config)
	bool bSCSEditorShowFloor;

	// Color curve:
	//   Release->Attack happens instantly
	//   Attack holds for AttackHoldPeriod, then
	//   Decays from Attack to Sustain for DecayPeriod with DecayExponent, then
	//   Sustain holds for SustainHoldPeriod, then
	//   Releases from Sustain to Release for ReleasePeriod with ReleaseExponent
	//
	// The effective time at which an event occurs is it's most recent exec time plus a bonus based on the position in the execution trace

	/** How much of a bonus does an exec get for being near the top of the trace stack, and how does that fall off with position? */
	UPROPERTY()
	float TracePositionBonusPeriod;

	UPROPERTY()
	float TracePositionExponent;

	// =====================================================================
	// The following options are NOT exposed in the preferences Editor
	// (usually because there is a different way to set them interactively!)

	/** Controls whether packages which are checked-out are automatically fully loaded at startup */
	UPROPERTY(config)
	uint32 bAutoloadCheckedOutPackages:1;

	/** If this is true, the user will not be asked to fully load a package before saving or before creating a new object */
	UPROPERTY(config)
	uint32 bSuppressFullyLoadPrompt:1;

	/** True if user should be allowed to select translucent objects in perspective viewports */
	UPROPERTY(config)
	uint32 bAllowSelectTranslucent:1;

	UPROPERTY(EditAnywhere, config, Category=Sound)
	uint32 bAllowBackgroundAudio:1;

	/** If true audio will be enabled in the editor. Does not affect PIE **/
	UPROPERTY(config)
	uint32 bEnableRealTimeAudio:1;

	/** Global volume setting for the editor */
	UPROPERTY(config)
	float EditorVolumeLevel;

	/** True if the actor count is displayed in the slate level browser */
	UPROPERTY(config)
	uint32 bDisplayActorCountInLevelBrowser:1;

	/** True if the Lightmass Size is displayed in the slate level browser */
	UPROPERTY(config)
	uint32 bDisplayLightmassSizeInLevelBrowser:1;

	/** True if the File Size is displayed in the slate level browser */
	UPROPERTY(config)
	uint32 bDisplayFileSizeInLevelBrowser:1;

	/** True if Level Paths are displayed in the slate level browser */
	UPROPERTY(config)
	uint32 bDisplayPathsInLevelBrowser:1;

	/** True if the Editor Offset is displayed in the slate level browser */
	UPROPERTY(config)
	uint32 bDisplayEditorOffsetInLevelBrowser:1;

	/** Enables audio feedback for certain operations in Unreal Editor, such as entering and exiting Play mode */
	UPROPERTY(EditAnywhere, config, Category=Sound)
	uint32 bEnableEditorSounds:1;

	UPROPERTY()
	class UBlueprintPaletteFavorites* BlueprintFavorites;

	/** Whether to automatically save after a time interval */
	UPROPERTY( EditAnywhere, config, Category = Build, meta = ( DisplayName = "Enable auto apply lighting" ) )
	uint32 bAutoApplyLightingEnable : 1;

public:

	/**
	 * Scene Outliner settings
	 */
	/** True when the Scene Outliner is only displaying selected Actors */
	UPROPERTY(config)
	uint32 bShowOnlySelectedActors:1;

	/** True when the Scene Outliner is hiding temporary/run-time Actors */
	UPROPERTY(config)
	uint32 bHideTemporaryActors:1;

	UPROPERTY(config)
	int32 MaterialQualityLevel;

public:
	/** Delegate for when a user setting has changed */
	DECLARE_EVENT_OneParam(UEditorUserSettings, FUserSettingChangedEvent, FName /*PropertyName*/);
	FUserSettingChangedEvent& OnUserSettingChanged() { return UserSettingChangedEvent; }

	// Begin UObject Interface
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	// End UObject Interface

private:

	FUserSettingChangedEvent UserSettingChangedEvent;
};

