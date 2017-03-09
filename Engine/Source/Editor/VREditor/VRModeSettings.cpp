// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VRModeSettings.h"

UVRModeSettings::UVRModeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bEnableAutoVREditMode = false;
	InteractorHand = EInteractorHand::Right;
	bShowWorldMovementGrid = true;
	bShowWorldMovementPostProcess = true;
	bShowWorldScaleProgressBar = true;
	UIBrightness = 1.5f;
	GizmoScale = 0.8f;
	DoubleClickTime = 0.25f;
	TriggerPressedThreshold_Vive = 0.03f;
	TriggerPressedThreshold_Rift = 0.2f;
}
