// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LevelEditorPlaySettings.h: Declares the ULevelEditorPlaySettings class.
=============================================================================*/

#pragma once


#include "LevelEditorPlaySettings.generated.h"


/**
 * Enumerates label anchor modes.
 */
UENUM()
enum ELabelAnchorMode
{
	LabelAnchorMode_TopLeft UMETA(DisplayName="Top Left"),
	LabelAnchorMode_TopCenter UMETA(DisplayName="Top Center"),
	LabelAnchorMode_TopRight UMETA(DisplayName="Top Right"),
	LabelAnchorMode_CenterLeft UMETA(DisplayName="Center Left"),
	LabelAnchorMode_Centered UMETA(DisplayName="Centered"),
	LabelAnchorMode_CenterRight UMETA(DisplayName="Center Right"),
	LabelAnchorMode_BottomLeft UMETA(DisplayName="Bottom Left"),
	LabelAnchorMode_BottomCenter UMETA(DisplayName="Bottom Center"),
	LabelAnchorMode_BottomRight UMETA(DisplayName="Bottom Right")
};


UENUM()
enum ELaunchModeType
{
	/** Runs the map on a specified device. */
	LaunchMode_OnDevice,
};


UENUM()
enum EPlayModeLocations
{
	/** Spawns the player at the current camera location. */
	PlayLocation_CurrentCameraLocation,

	/** Spawns the player from the default player start. */
	PlayLocation_DefaultPlayerStart,
};


UENUM()
enum EPlayModeType
{
	/** Runs from within the editor. */
	PlayMode_InViewPort = 0,

	/** Runs in a new window. */
	PlayMode_InEditorFloating,

	/** Runs a mobile preview in a new process. */
	PlayMode_InMobilePreview,

	/** Runs in a new process. */
	PlayMode_InNewProcess,

	/** The number of different Play Modes */
	PlayMode_Count,
};


UENUM()
enum EPlayNetMode
{
	PIE_Standalone UMETA(DisplayName="Play As Standalone"),
	PIE_ListenServer UMETA(DisplayName="Play As Listen Server"),
	PIE_Client UMETA(DisplayName="Play As Client"),
};


/**
 * Holds information about a screen resolution to be used for playing.
 */
USTRUCT()
struct FPlayScreenResolution
{
	GENERATED_USTRUCT_BODY()

public:

	/**
	 * The description text for this screen resolution.
	 */
	UPROPERTY(config)
	/*FText*/FString Description;

	/**
	 * The screen resolution's width (in pixels).
	 */
	UPROPERTY(config)
	int32 Width;

	/**
	 * The screen resolution's height (in pixels).
	 */
	UPROPERTY(config)
	int32 Height;

	/**
	 * The screen resolution's aspect ratio (as a string).
	 */
	UPROPERTY(config)
	FString AspectRatio;
};


/**
 * Implements the Editor's play settings.
 */
UCLASS(config=EditorUserSettings)
class UNREALED_API ULevelEditorPlaySettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * The PlayerStart class used when spawning the player at the current camera location.
	 */
	UPROPERTY(globalconfig, EditAnywhere, Category=General, meta=(ToolTip=""))
	FString PlayFromHerePlayerStartClassName;

public:

	/**
	 * Should Play-in-Editor automatically give mouse control to the game on PIE start (default = false).
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInEditor, meta=(ToolTip="Give the game mouse control when PIE starts or require a click in the viewport first"))
	bool GameGetsMouseControl;

	/**
	 * Whether to show a label for mouse control gestures in the PIE view.
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInEditor)
	bool ShowMouseControlLabel;

	/**
	 * Location on screen to anchor the mouse control label when in PIE mode.
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInEditor)
	TEnumAsByte<ELabelAnchorMode> MouseControlLabelPosition;

	/**
	 * Whether to automatically recompile blueprints on PIE
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInEditor, meta=(ToolTip="Automatically recompile blueprints used by the current level when initiating a Play In Editor session"))
	bool AutoRecompileBlueprints;

	/**
	 * True if Play In Editor should only load currently-visible levels in PIE.
	 */
	UPROPERTY(config)
	uint32 bOnlyLoadVisibleLevelsInPIE:1;

public:

	/**
	 * The width of the new view port window in pixels (0 = use the desktop's screen resolution).
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInNewWindow)
	int32 NewWindowWidth;

	/**
	 * The height of the new view port window in pixels (0 = use the desktop's screen resolution).
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInNewWindow)
	int32 NewWindowHeight;

	/**
	 * The position of the new view port window on the screen in pixels.
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInNewWindow)
	FIntPoint NewWindowPosition;

	/**
	 * Whether the new window should be centered on the screen.
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInNewWindow)
	bool CenterNewWindow;

public:

	/**
	 * The width of the standalone game window in pixels (0 = use the desktop's screen resolution).
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInStandaloneGame)
	int32 StandaloneWindowWidth;

	/**
	 * The height of the standalone game window in pixels (0 = use the desktop's screen resolution).
	 */
	UPROPERTY(config, EditAnywhere, Category=PlayInStandaloneGame)
	int32 StandaloneWindowHeight;

	/**
	 * Whether sound should be disabled when playing standalone games.
	 */
	UPROPERTY(config , EditAnywhere, Category=PlayInStandaloneGame, AdvancedDisplay)
	uint32 DisableStandaloneSound:1;

	/**
	 * Extra parameters to be include as part of the command line for the standalone game.
	 */
	UPROPERTY(config , EditAnywhere, Category=PlayInStandaloneGame, AdvancedDisplay)
	FString AdditionalLaunchParameters;

public:

	/**
	 * NetMode to use for Play In Editor.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	TEnumAsByte<EPlayNetMode> PlayNetMode;

	/**
     * Spawn multiple player windows in a single instance of UE4. This will load much faster, but has potential to have more issues. 
     */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	bool RunUnderOneProcess;

	/**
	 * If checked, a separate dedicated server will be launched. Otherwise the first player window will act as a listen server that all other player windows connect to.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	bool PlayNetDedicated;

	/**
	 * Number of player windows that should be opened. Listen server will count as a player, a dedicated server will not.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions, meta=(ClampMin = "1", UIMin = "1", UIMax = "64"))
	int32 PlayNumberOfClients;

	/**
	 * Window width to use when spawning additional clients.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	int32 ClientWindowWidth;

	/**
	 * When running multiple player windows in a single process, this option determines how the game pad input gets routed.
	 *
	 * If unchecked (default) then the 1st game pad is attached to the 1st window, 2nd to the 2nd window, and so on.
	 *
	 * If it is checked, the 1st game pad goes the 2nd window. The 1st window can then be controlled by keyboard/mouse, which is convenient if two people are testing on the same computer.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	bool RouteGamepadToSecondWindow;

	/**
	 * Window height to use when spawning additional clients.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	int32 ClientWindowHeight;

	/**
	 * Additional options that will be passed to the server as URL parameters.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	FString AdditionalServerGameOptions;

	/**
	 * Additional command line options that will be passed to standalone game instances.
	 */
	UPROPERTY(config, EditAnywhere, Category=MultiplayerOptions)
	FString AdditionalLaunchOptions;

public:

	/**
	 * The last used height for multiple instance windows (in pixels).
	 */
	UPROPERTY(config)
	int32 MultipleInstanceLastHeight;

	/**
	 * The last used width for multiple instance windows (in pixels).
	 */
	UPROPERTY(config)
	int32 MultipleInstanceLastWidth;

	/**
	 * The last known screen positions of multiple instance windows (in pixels).
	 */
	UPROPERTY(config)
	TArray<FIntPoint> MultipleInstancePositions;

public:

	/**
	 * The name of the last platform that the user ran a play session on.
	 */
	UPROPERTY(config)
	FString LastExecutedLaunchDevice;

	/**
	 * The name of the last device that the user ran a play session on.
	 */
	UPROPERTY(config)
	FString LastExecutedLaunchName;

	/**
	 * The last type of play-on session the user ran.
	 */
	UPROPERTY(config)
	TEnumAsByte<ELaunchModeType> LastExecutedLaunchModeType;

	/**
	 * The last type of play location the user ran.
	 */
	UPROPERTY(config)
	TEnumAsByte<EPlayModeLocations> LastExecutedPlayModeLocation;

	/**
	 * The last type of play session the user ran.
	 */
	UPROPERTY(config)
	TEnumAsByte<EPlayModeType> LastExecutedPlayModeType;

public:

	/**
	 * Collection of common screen resolutions on mobile phones.
	 */
	UPROPERTY(config)
	TArray<FPlayScreenResolution> LaptopScreenResolutions;

	/**
	 * Collection of common screen resolutions on desktop monitors.
	 */
	UPROPERTY(config)
	TArray<FPlayScreenResolution> MonitorScreenResolutions;

	/**
	 * Collection of common screen resolutions on mobile phones.
	 */
	UPROPERTY(config)
	TArray<FPlayScreenResolution> PhoneScreenResolutions;

	/**
	 * Collection of common screen resolutions on tablet devices.
	 */
	UPROPERTY(config)
	TArray<FPlayScreenResolution> TabletScreenResolutions;

	/**
	 * Collection of common screen resolutions on television screens.
	 */
	UPROPERTY(config)
	TArray<FPlayScreenResolution> TelevisionScreenResolutions;
};
