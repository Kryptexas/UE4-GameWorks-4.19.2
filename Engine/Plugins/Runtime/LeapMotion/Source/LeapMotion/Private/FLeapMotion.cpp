// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LeapMotionPrivatePCH.h"

#define LOCTEXT_NAMESPACE "LeapPlugin"
#define PLUGIN_VERSION "1.0.0"

void FLeapMotion::StartupModule()
{
	//Expose all of our input mapping keys to the engine
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapLeftPinch, LOCTEXT("LeapLeftPinch", "Leap Left Pinch"), FKeyDetails::GamepadKey));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapLeftGrab, LOCTEXT("LeapLeftGrab", "Leap Left Grab"), FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapLeftPalmPitch, LOCTEXT("LeapLeftPalmPitch", "Leap Left Palm Pitch"), FKeyDetails::FloatAxis));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapLeftPalmYaw, LOCTEXT("LeapLeftPalmYaw", "Leap Left Palm Yaw"), FKeyDetails::FloatAxis));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapLeftPalmRoll, LOCTEXT("LeapLeftPalmRoll", "Leap Left Palm Roll"), FKeyDetails::FloatAxis));

	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapRightPinch, LOCTEXT("LeapRightPinch", "Leap Right Pinch"), FKeyDetails::GamepadKey));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapRightGrab, LOCTEXT("LeapRightGrab", "Leap Right Grab"), FKeyDetails::GamepadKey));

	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapRightPalmPitch, LOCTEXT("LeapRightPalmPitch", "Leap Right Palm Pitch"), FKeyDetails::FloatAxis));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapRightPalmYaw, LOCTEXT("LeapRightPalmYaw", "Leap Right Palm Yaw"), FKeyDetails::FloatAxis));
	EKeys::AddKey(FKeyDetails(FKeysLeap::LeapRightPalmRoll, LOCTEXT("LeapRightPalmRoll", "Leap Right Palm Roll"), FKeyDetails::FloatAxis));

	UE_LOG(LeapPluginLog, Log, TEXT("Using LeapPlugin version %s"), TEXT(PLUGIN_VERSION));
}

void FLeapMotion::ShutdownModule()
{
    
}

Leap::Controller* FLeapMotion::Controller()
{
	return &LeapController;
}

IMPLEMENT_MODULE(FLeapMotion, LeapMotion)

#undef LOCTEXT_NAMESPACE