// Copyright 2017 Google Inc.

#include "TangoMotionController.h"
#include "TangoPluginPrivate.h"

#include "TangoMotion.h"
#include "TangoLifecycle.h"

static bool bLastRenderThreadPoseWasValid = false;

FTangoMotionController::FTangoMotionController()
	: TangoDeviceInstance(nullptr)
{
	TangoDeviceInstance = FTangoDevice::GetInstance();
}

void FTangoMotionController::RegisterController()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
}

void FTangoMotionController::UnregisterController()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

bool FTangoMotionController::GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
{
	if (IsInGameThread())
	{
		FTangoPose OutPose;
		bool bIsValidPose = TangoDeviceInstance->TangoMotionManager.GetCurrentPose(ETangoReferenceFrame::DEVICE, OutPose);

		OutOrientation = FRotator(OutPose.Pose.GetRotation());
		OutPosition = OutPose.Pose.GetTranslation();
		return bIsValidPose;
	}
	else // presumed render thread.
	{
		if(FTangoDevice::GetInstance()->GetIsTangoRunning())
		{
			FTangoPose OutPose;
			bool bIsValidPose = TangoDeviceInstance->TangoMotionManager.GetPoseAtTime(ETangoReferenceFrame::DEVICE, 0, OutPose);

			bLastRenderThreadPoseWasValid = bIsValidPose;

			OutOrientation = FRotator(OutPose.Pose.GetRotation());
			OutPosition = OutPose.Pose.GetTranslation();
			return bIsValidPose;
		}
		else
		{
			return false;
		}
	}
}

ETrackingStatus FTangoMotionController::GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const
{
	bool isTracked;

	if (IsInGameThread())
	{
		isTracked = TangoDeviceInstance->TangoMotionManager.IsCurrentPoseValid(ETangoReferenceFrame::DEVICE);
	}
	else
	{
		// TODO:
		// When called on the render thread, this assumes very blatantly that GetControllerOrientationAndPosition will be
		// called immediately preceeding this, as is the behaviour of UMotionControllerComponent::PollControllerState().
		//
		// The essential problem is how to get the 'most current and up to date' tracking during rendering but still use
		// the same pose or all associated calls to prevent inconsistencies.
		//
		// There is surely a cleaner way to do this.

		isTracked = bLastRenderThreadPoseWasValid;
	}

	return isTracked ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;
}

FName FTangoMotionController::GetMotionControllerDeviceTypeName() const
{
	static const FName DeviceName(TEXT("TangoMotionController"));
	return DeviceName;
}
