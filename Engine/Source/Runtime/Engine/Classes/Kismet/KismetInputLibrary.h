// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateCore.h"

#include "KismetInputLibrary.generated.h"

UCLASS(MinimalAPI)
class UKismetInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Calibrate the tilt for the input device */
	UFUNCTION(BlueprintCallable, Category="Input")
	static void CalibrateTilt();

	/**
	 * Returns the key for this event.
	 *
	 * @return  Key name
	 */
	UFUNCTION(BlueprintCallable, Category="Keyboard")
	static FKey GetKey(const FKeyboardEvent& Input);

	/**
	 * Test if the input key are equal (A == B)
	 * @param A - The key to compare against
	 * @param B - The key to compare
	 * @returns True if the key are equal, false otherwise
	 */
	UFUNCTION(BlueprintPure, meta=(FriendlyName = "Equal (Key)", CompactNodeTitle = "=="), Category="Utilities|Key")
	static bool EqualEqual_KeyKey(FKey A, FKey B);

	/**
	 * Returns whether or not this character is an auto-repeated keystroke
	 *
	 * @return  True if this character is a repeat
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsRepeat" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsRepeat(const FInputEvent& Input);

	/**
	 * Returns true if either shift key was down when this event occurred
	 *
	 * @return  True if shift is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsShiftDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsShiftDown(const FInputEvent& Input);

	/**
	 * Returns true if left shift key was down when this event occurred
	 *
	 * @return True if left shift is pressed.
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsLeftShiftDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsLeftShiftDown(const FInputEvent& Input);

	/**
	 * Returns true if right shift key was down when this event occurred
	 *
	 * @return True if right shift is pressed.
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsRightShiftDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsRightShiftDown(const FInputEvent& Input);

	/**
	 * Returns true if either control key was down when this event occurred
	 *
	 * @return  True if control is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsControlDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsControlDown(const FInputEvent& Input);

	/**
	 * Returns true if left control key was down when this event occurred
	 *
	 * @return  True if left control is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsLeftControlDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsLeftControlDown(const FInputEvent& Input);

	/**
	 * Returns true if left control key was down when this event occurred
	 *
	 * @return  True if left control is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsRightControlDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsRightControlDown(const FInputEvent& Input);

	/**
	 * Returns true if either alt key was down when this event occurred
	 *
	 * @return  True if alt is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsAltDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsAltDown(const FInputEvent& Input);

	/**
	 * Returns true if left alt key was down when this event occurred
	 *
	 * @return  True if left alt is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsLeftAltDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsLeftAltDown(const FInputEvent& Input);

	/**
	 * Returns true if right alt key was down when this event occurred
	 *
	 * @return  True if right alt is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsRightAltDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsRightAltDown(const FInputEvent& Input);

	/**
	 * Returns true if either command key was down when this event occurred
	 *
	 * @return  True if command is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsCommandDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsCommandDown(const FInputEvent& Input);

	/**
	 * Returns true if left command key was down when this event occurred
	 *
	 * @return  True if left command is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsLeftCommandDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsLeftCommandDown(const FInputEvent& Input);

	/**
	 * Returns true if right command key was down when this event occurred
	 *
	 * @return  True if right command is pressed
	 */
	UFUNCTION(BlueprintPure, meta=( FriendlyName = "IsRightCommandDown" ), Category="Utilities|InputEvent")
	static bool InputEvent_IsRightCommandDown(const FInputEvent& Input);
};
