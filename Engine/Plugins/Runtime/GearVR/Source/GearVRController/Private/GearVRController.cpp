// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GearVRController.h"
#include "IGearVRControllerPlugin.h"
#include "IInputDevice.h"
#include "IMotionController.h"
#include "IGearVRPlugin.h"
#include "GenericPlatform/IInputInterface.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "../../GearVR/Private/GearVR.h"

class FGearVRControllerPlugin : public IGearVRControllerPlugin
{
	virtual TSharedPtr< class IInputDevice > CreateInputDevice(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override
	{
#if GEARVR_SUPPORTED_PLATFORMS
		return TSharedPtr< class IInputDevice >(new FGearVRController(InMessageHandler));
#else
		return NULL;
#endif
	}
};

IMPLEMENT_MODULE(FGearVRControllerPlugin, GearVRController)

#if GEARVR_SUPPORTED_PLATFORMS

#define LOCTEXT_NAMESPACE "GearVRController"

FGearVR* GetGearVRHMD()
{
	if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_GearVR))
	{
		return static_cast<FGearVR*>(GEngine->HMDDevice.Get());
	}

	return nullptr;
}

FGearVRController::FGearVRController( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
	: MessageHandler( InMessageHandler ), RemoteEmulationUnset(false), isRightHanded(true), useArmModel(true)

{
#if PLATFORM_ANDROID
	CurrentRemoteID = ovrDeviceIdType_Invalid;
#endif
	for (int i = 0;i < 4;i++)
	{
		EmulatedDirectionalButtons[i] = false;
	}
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	UE_LOG(LogHMD, Log, TEXT("Starting GearVRController plugin"));

	CorrespondingUE4Buttons[0][0] = FGamepadKeyNames::MotionController_Left_FaceButton1;
	CorrespondingUE4Buttons[1][0] = FGamepadKeyNames::MotionController_Right_FaceButton1;
	CorrespondingUE4Buttons[0][1] = FGamepadKeyNames::MotionController_Left_FaceButton2;
	CorrespondingUE4Buttons[1][1] = FGamepadKeyNames::MotionController_Right_FaceButton2;
	CorrespondingUE4Buttons[0][2] = FGamepadKeyNames::MotionController_Left_FaceButton3;
	CorrespondingUE4Buttons[1][2] = FGamepadKeyNames::MotionController_Right_FaceButton3;
	CorrespondingUE4Buttons[0][3] = FGamepadKeyNames::MotionController_Left_FaceButton4;
	CorrespondingUE4Buttons[1][3] = FGamepadKeyNames::MotionController_Right_FaceButton4;
}


FGearVRController::~FGearVRController()
{
	IModularFeatures::Get().UnregisterModularFeature( GetModularFeatureName(), this );
}

void FGearVRController::LoadConfig()
{

}

void FGearVRController::Tick( float DeltaTime )
{
#if PLATFORM_ANDROID

	FGearVR* GearVR = GetGearVRHMD();
	if (GearVR)
	{
		FOvrMobileSynced OvrMobile = GearVR->GetMobileSynced();
		if (OvrMobile.IsValid())
		{
			if (!RemoteEmulationUnset)
			{
				vrapi_SetRemoteEmulation(*OvrMobile, false);
				RemoteEmulationUnset = true;
			}
			if (CurrentRemoteID == ovrDeviceIdType_Invalid)
			{
				ovrInputCapabilityHeader capsHeader;
				memset(&capsHeader, 0, sizeof(ovrInputCapabilityHeader));

				for (int enumerationIndex = 0; vrapi_EnumerateInputDevices(*OvrMobile, enumerationIndex, &capsHeader) >= 0; ++enumerationIndex)
				{
					if (capsHeader.Type == ovrControllerType_TrackedRemote) {
						CurrentRemoteID = capsHeader.DeviceID;

						RemoteCapabilities.Header = capsHeader;

						vrapi_GetInputDeviceCapabilities(*OvrMobile, &RemoteCapabilities.Header);
						UE_LOG(LogHMD, Log, TEXT("Found a GearVR Remote with device ID %d"), CurrentRemoteID);

						isRightHanded = RemoteCapabilities.ControllerCapabilities & ovrControllerCaps_RightHand;

						LastRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
						if (vrapi_GetCurrentInputState(*OvrMobile, CurrentRemoteID, &LastRemoteState.Header) < 0)
						{
							UE_LOG(LogHMD, Log, TEXT("Error while fetching input data from GearVR Remote %d in Tick"), CurrentRemoteID);
						}
					}
				}
			}
		}
	}
#endif

}


void FGearVRController::SendControllerEvents()
{
#if PLATFORM_ANDROID

	FGearVR* GearVR = GetGearVRHMD();
	if (GearVR && CurrentRemoteID != ovrDeviceIdType_Invalid)
	{
		FOvrMobileSynced OvrMobile = GearVR->GetMobileSynced();
		if (OvrMobile.IsValid())
		{
			ovrInputStateTrackedRemote CurrentRemoteState;
			CurrentRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;

			if (vrapi_GetCurrentInputState(*OvrMobile, CurrentRemoteID, &CurrentRemoteState.Header) < 0)
			{
				UE_LOG(LogHMD, Log, TEXT("Error while fetching input data from GearVR Remote %d"), CurrentRemoteID);
				CurrentRemoteID = ovrDeviceIdType_Invalid;
				return;
			}

			if ((RemoteCapabilities.ButtonCapabilities & ovrButton_A) && (CurrentRemoteState.Buttons & ovrButton_A) != (LastRemoteState.Buttons & ovrButton_A))
			{
				if (CurrentRemoteState.Buttons & ovrButton_A)
				{
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Left_Trigger, 0, false);
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Right_Trigger, 0, false);
				}
				else
				{
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Left_Trigger, 0, false);
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Right_Trigger, 0, false);
				}
			}

			if ((RemoteCapabilities.ButtonCapabilities & ovrButton_Enter) && (CurrentRemoteState.Buttons & ovrButton_Enter) != (LastRemoteState.Buttons & ovrButton_Enter))
			{
				if (CurrentRemoteState.Buttons & ovrButton_Enter)
				{
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Left_Thumbstick, 0, false);
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Right_Thumbstick, 0, false);
				}
				else
				{
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Left_Thumbstick, 0, false);
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Right_Thumbstick, 0, false);
				}
			}

			if ((RemoteCapabilities.ButtonCapabilities & ovrButton_Back) && (CurrentRemoteState.Buttons & ovrButton_Back) != (LastRemoteState.Buttons & ovrButton_Back))
			{
				if (CurrentRemoteState.Buttons & ovrButton_Back)
				{
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Left_FaceButton5, 0, false);
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Right_FaceButton5, 0, false);
				}
				else
				{
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Left_FaceButton5, 0, false);
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Right_FaceButton5, 0, false);
				}
			}

			float TouchPadX = 0, TouchPadY = 0;
			if (CurrentRemoteState.TrackpadStatus != 0)
			{
				TouchPadX = ((CurrentRemoteState.TrackpadPosition.x / (float)RemoteCapabilities.TrackpadMaxX) - 0.5f) * 2.0f;
				TouchPadY = ((CurrentRemoteState.TrackpadPosition.y / (float)RemoteCapabilities.TrackpadMaxY) - 0.5f) * 2.0f * -1.0f;
			}

			if (CurrentRemoteState.TrackpadStatus != LastRemoteState.TrackpadStatus)
			{
				if (CurrentRemoteState.TrackpadStatus != 0)
				{
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Left_FaceButton6, 0, false);
					MessageHandler->OnControllerButtonPressed(FGamepadKeyNames::MotionController_Right_FaceButton6, 0, false);
				}
				else
				{
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Left_FaceButton6, 0, false);
					MessageHandler->OnControllerButtonReleased(FGamepadKeyNames::MotionController_Right_FaceButton6, 0, false);
				}
			}
				
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Left_Thumbstick_X, 0, TouchPadX);
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Right_Thumbstick_X, 0, TouchPadX);

			MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Left_Thumbstick_Y, 0, TouchPadY);
			MessageHandler->OnControllerAnalog(FGamepadKeyNames::MotionController_Right_Thumbstick_Y, 0, TouchPadY);

			bool touchpadPressed = ((RemoteCapabilities.ButtonCapabilities & ovrButton_Enter) && (CurrentRemoteState.Buttons & ovrButton_Enter));
			bool currentEmulatedDirectionalButtons[4];
			currentEmulatedDirectionalButtons[0] = touchpadPressed && TouchPadY > 0.75;
			currentEmulatedDirectionalButtons[1] = touchpadPressed && TouchPadX > 0.75;
			currentEmulatedDirectionalButtons[2] = touchpadPressed && TouchPadY < -0.75;
			currentEmulatedDirectionalButtons[3] = touchpadPressed && TouchPadX < -0.75;

			for (int i = 0; i < 4; i++)
			{
				if (currentEmulatedDirectionalButtons[i] != EmulatedDirectionalButtons[i])
				{
					if (currentEmulatedDirectionalButtons[i])
					{
						MessageHandler->OnControllerButtonPressed(CorrespondingUE4Buttons[0][i], 0, false);
						MessageHandler->OnControllerButtonPressed(CorrespondingUE4Buttons[1][i], 0, false);
					} 
					else
					{
						MessageHandler->OnControllerButtonReleased(CorrespondingUE4Buttons[0][i], 0, false);
						MessageHandler->OnControllerButtonReleased(CorrespondingUE4Buttons[1][i], 0, false);
					}
				}

				EmulatedDirectionalButtons[i] = currentEmulatedDirectionalButtons[i];
			}

			LastRemoteState = CurrentRemoteState;
		}
	}
#endif

}


void FGearVRController::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	MessageHandler = InMessageHandler;
}


bool FGearVRController::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	// No exec commands supported, for now.
	return false;
}

bool FGearVRController::GetControllerOrientationAndPosition( const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale ) const
{
	bool bHaveControllerData = false;
#if PLATFORM_ANDROID
	if (CurrentRemoteID != ovrDeviceIdType_Invalid)
	{
		FGearVR* GearVR = GetGearVRHMD();
		if (GearVR)
		{
			FOvrMobileSynced OvrMobile = GearVR->GetMobileSynced();
			if (OvrMobile.IsValid())
			{
				ovrTracking currentRemoteTracking;
				if (vrapi_GetInputTrackingState(*OvrMobile, CurrentRemoteID, 0, &currentRemoteTracking) < 0)
				{
					UE_LOG(LogHMD, Log, TEXT("Error while fetching orientation data from GearVR Remote %d"), CurrentRemoteID);
					return false;
				}

				FGameFrame *CurrentFrame = nullptr;
				if (IsInRenderingThread())
				{
					FViewExtension* RenderContext = GearVR->GetCustomPresent_Internal()->GetRenderContext();
					if (RenderContext)
					{
						CurrentFrame = RenderContext->GetRenderFrame();
					}
				}
				else
				{
					CurrentFrame = GearVR->GetFrame();
				}

				if (CurrentFrame)
				{

					FQuat Orient;
					FVector Pos;
					CurrentFrame->PoseToOrientationAndPosition(currentRemoteTracking.HeadPose.Pose, Orient, Pos);

					OutPosition = useArmModel ? Pos : FVector::ZeroVector;

					OutOrientation = Orient.Rotator();
					bHaveControllerData = true;
				}
			}
		}

	}
#endif
	return bHaveControllerData;
}

ETrackingStatus FGearVRController::GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const
{
	ETrackingStatus TrackingStatus = ETrackingStatus::NotTracked;
#if PLATFORM_ANDROID
	if (CurrentRemoteID != ovrDeviceIdType_Invalid)
	{
		return ETrackingStatus::InertialOnly;
	}
#endif
	return TrackingStatus;
}

void FGearVRController::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
}

void FGearVRController::SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values)
{
}

#undef LOCTEXT_NAMESPACE
#endif	 // GEARVR_SUPPORTED_PLATFORMS
