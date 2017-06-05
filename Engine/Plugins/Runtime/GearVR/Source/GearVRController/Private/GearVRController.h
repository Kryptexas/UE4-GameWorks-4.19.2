// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IInputDevice.h"
#include "IMotionController.h"
#include "HeadMountedDisplay.h"
#include "../../GearVR/Private/GearVR.h"

#if GEARVR_SUPPORTED_PLATFORMS
#include "VrApi_Input.h"

/**
 * Unreal Engine support for Oculus motion controller devices
 */
class FGearVRController : public IInputDevice, public IMotionController
{
public:

	/** Constructor that takes an initial message handler that will receive motion controller events */
	FGearVRController( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler );

	/** Clean everything up */
	virtual ~FGearVRController();

	/** Loads any settings from the config folder that we need */
	static void LoadConfig();

	// IInputDevice overrides
	virtual void Tick( float DeltaTime ) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler ) override;
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override;

	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override;

	// IMotionController overrides
	virtual bool GetControllerOrientationAndPosition( const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale ) const override;
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override;

	bool isRightHanded;
	bool useArmModel;
private:

	/** The recipient of motion controller input events */
	TSharedPtr< FGenericApplicationMessageHandler > MessageHandler;
	bool RemoteEmulationUnset;
	bool EmulatedDirectionalButtons[4];
	FGamepadKeyNames::Type CorrespondingUE4Buttons[2][4];

#if PLATFORM_ANDROID
	ovrInputStateTrackedRemote LastRemoteState;
	ovrInputTrackedRemoteCapabilities RemoteCapabilities;
	ovrDeviceID CurrentRemoteID;
#endif
};

#endif //OCULUS_INPUT_SUPPORTED_PLATFORMS
