// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "VREditorUISystem.h"
#include "SlateTypes.h"
#include "InputCoreTypes.h"

namespace EVREditorActions
{
	//Gizmo coordinate button text starts blank before it is initialized
	static FText GizmoCoordinateSystemText;

	//Gizmo mode button text starts blank before it is initialized
	static FText GizmoModeText;
}

/**
* Implementation of various VR editor action callback functions
*/
class FVREditorActionCallbacks
{
public:

	/** Returns the checked state of the translation snap enable/disable button */
	static ECheckBoxState GetTranslationSnapState();

	/** Rotates through the available translation snap sizes */
	static void OnTranslationSnapSizeButtonClicked();

	/** Returns the translation snap size as text to use as the button display text */
	static FText GetTranslationSnapSizeText();

	/** Returns the checked state of the rotation snap enable/disable button */
	static ECheckBoxState GetRotationSnapState();

	/** Rotates through the available rotation snap sizes */
	static void OnRotationSnapSizeButtonClicked();

	/** Returns the rotation snap size as text to use as the button display text */
	static FText GetRotationSnapSizeText();

	/** Returns the checked state of the scale snap enable/disable button */
	static ECheckBoxState GetScaleSnapState();

	/** Rotates through the available scale snap sizes */
	static void OnScaleSnapSizeButtonClicked();

	/** Returns the scale snap size as text to use as the button display text */
	static FText GetScaleSnapSizeText();

	/**
	 * Toggles the gizmo coordinate system between local and world space
	 * @param InVRMode Currently active VRMode
	 */
	static void OnGizmoCoordinateSystemButtonClicked(class UVREditorMode* InVRMode);

	/** Returns the gizmo coordinate system as text to use as the button display text */
	static FText GetGizmoCoordinateSystemText();

	/**
	 * Updates the gizmo coordinate system text if the coordinate system or gizmo type is changed
	 * @param InVRMode Currently active VRMode
	 */
	static void UpdateGizmoCoordinateSystemText(class UVREditorMode* InVRMode);

	/**
	 * Rotates the gizmo type through universal, translate, rotate, and scale
	 * @param InVRMode Currently active VRMode
	 */
	static void OnGizmoModeButtonClicked(class UVREditorMode* InVRMode);

	/** Returns the gizmo type as text to use as the button display text */
	static FText GetGizmoModeText();

	/**
	 * Updates the gizmo coordinate type text if the coordinate system or gizmo type is changed
	 * @param InVRMode Currently active VRMode
	 */
	static void UpdateGizmoModeText(class UVREditorMode* InVRMode);

	/**
	* Cycles the gizmo through all coordinate spaces and Actor manipulation types
	* @param InVRMode Currently active VRMode
	*/
	static void OnGizmoCycle(class UVREditorMode* InVRMode);

	/**
	 * Toggles a VR UI panel's state between visible and invisible
	 * @param InVRMode Currently active VRMode
	 * @param PanelToToggle UI panel to change the state of
	 */
	static void OnUIToggleButtonClicked(class UVREditorMode* InVRMode, const UVREditorUISystem::EEditorUIPanel PanelToToggle);

	/**
	 * Returns a VR UI panel's visibility - used for check boxes on the menu button
	 * @param InVRMode Currently active VRMode
	 * @param PanelToToggle UI panel to read the state of
	 */
	static ECheckBoxState GetUIToggledState(class UVREditorMode* InVRMode, const UVREditorUISystem::EEditorUIPanel PanelToCheck);

	/**
	 * Toggles a flashlight on and off on the interactor that clicked on the UI button
	 * @param InVRMode Currently active VRMode
	 */
	static void OnLightButtonClicked(class UVREditorMode* InVRMode);

	/**
	 * Returns whether or not the flashlight is enabled - used for check box on the flashlight button
	 * @param InVRMode Currently active VRMode
	 */
	static ECheckBoxState GetFlashlightState(class UVREditorMode* InVRMode);

	/**
	 * Takes a screenshot of the mirror viewport
	 * @param InVRMode Currently active VRMode
	 */
	static void OnScreenshotButtonClicked(class UVREditorMode* InVRMode);

	/**
	 * Enters Play In Editor mode for testing of gameplay
	 * @param InVRMode Currently active VRMode
	 */
	static void OnPlayButtonClicked(class UVREditorMode* InVRMode);

	/**
	 * Snaps currently selected Actors to the ground
	 * @param InVRMode Currently active VRMode
	 */
	static void OnSnapActorsToGroundClicked(class UVREditorMode* InVRMode);

	/**
	 * Simulates the user entering characters with a keyboard for data entry
	 * @param InChar String of characters to enter
	 */
	static void SimulateCharacterEntry(const FString InChar);

	/** Send a backspace event. Slate editable text fields handle backspace as a TCHAR(8) character entry */
	static void SimulateBackspace();

	/**
	 * Simulates the user pressing a key down
	 * @param Key Key to press
	 * @param bRepeat Whether or not to repeat
	 */
	static void SimulateKeyDown( FKey Key, bool bRepeat );

	/**
	 * Simulates the user releasing a key
	 * @param Key Key to release
	 */
	static void SimulateKeyUp( FKey Key );

}
;



