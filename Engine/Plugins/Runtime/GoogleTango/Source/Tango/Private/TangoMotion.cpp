/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "TangoMotion.h"
#include "TangoLifecycle.h"
#include "EcefToolsLibrary.h"
#include "TangoPluginPrivate.h"
#include "TangoAndroidHelper.h"
#include "Misc/ScopeLock.h"

#if PLATFORM_ANDROID
#include "tango_client_api_dynamic.h"

static void OnPoseAvailable(void* Context, const TangoPoseData* Pose)
{
	FTangoDevice* TangoDeviceContext = static_cast<FTangoDevice*> (Context);
	if (TangoDeviceContext == nullptr)
	{
		UE_LOG(LogTango, Error, TEXT("Error: Failed to cast to FTangoDevice!"));
	}
	else
	{
		TangoDeviceContext->TangoMotionManager.OnTangoPoseUpdated();
	}
}
#endif



FTangoMotion::FTangoMotion()
	: bIsDevicePoseValid(false)
	, bIsColorCameraPoseValid(false)
	, bIsEcefPoseValid(false)
	, bWaitingForNewPose(false)
	, NewPoseAvailable(nullptr)
{
}

FTangoMotion::~FTangoMotion()
{
	if (NewPoseAvailable != nullptr)
	{
		FPlatformProcess::ReturnSynchEventToPool(NewPoseAvailable);
	}
}

void FTangoMotion::OnTangoPoseUpdated()
{
	if (bWaitingForNewPose && NewPoseAvailable)
	{

		NewPoseAvailable->Trigger();
	}
}

bool FTangoMotion::ConnectOnPoseAvailable(ETangoReferenceFrame PoseBaseFrame)
{
#if PLATFORM_ANDROID
	TangoCoordinateFramePair PoseFramePair;
	PoseFramePair.base = static_cast<TangoCoordinateFrameType>(PoseBaseFrame);
	PoseFramePair.target = TANGO_COORDINATE_FRAME_DEVICE;
	if (TangoService_connectOnPoseAvailable_dynamic(1, &PoseFramePair, OnPoseAvailable) != TANGO_SUCCESS)
	{
		return false;
	}
#endif
	return true;
}

void FTangoMotion::DisconnectOnPoseAvailable()
{
#if PLATFORM_ANDROID
	TangoCoordinateFramePair PoseFramePair;
	PoseFramePair.base = static_cast<TangoCoordinateFrameType>(BaseFrame);
	PoseFramePair.target = TANGO_COORDINATE_FRAME_DEVICE;
	TangoService_connectOnPoseAvailable_dynamic(1, &PoseFramePair, nullptr);
#endif
}

void FTangoMotion::UpdateBaseFrame(ETangoReferenceFrame InBaseFrame)
{
	BaseFrame = InBaseFrame;
}

void FTangoMotion::UpdateTangoPoses()
{
	bIsDevicePoseValid = GetPoseAtTime(ETangoReferenceFrame::DEVICE, 0.0f, LatestDevicePose);
	bIsColorCameraPoseValid = GetPoseAtTime(ETangoReferenceFrame::CAMERA_COLOR, 0.0, LatestColorCameraPose);
}

bool FTangoMotion::GetCurrentPose(ETangoReferenceFrame TargetFrame, FTangoPose& OutTangoPose)
{
	switch (TargetFrame)
	{
	case ETangoReferenceFrame::DEVICE:
		OutTangoPose = LatestDevicePose;
		return bIsDevicePoseValid;
	case ETangoReferenceFrame::CAMERA_COLOR:
		OutTangoPose = LatestColorCameraPose;
		return bIsColorCameraPoseValid;
	}
	return false;
}

bool FTangoMotion::IsCurrentPoseValid(ETangoReferenceFrame TargetFrame)
{
	switch (TargetFrame)
	{
	case ETangoReferenceFrame::DEVICE:
		return bIsDevicePoseValid;
	case ETangoReferenceFrame::CAMERA_COLOR:
		return bIsColorCameraPoseValid;
	}
	return false;
}

#if PLATFORM_ANDROID
FTransform FTangoMotion::ConvertTangoPoseToTransform(const TangoPoseData* RawPose)
{
	float UnrealUnitsPerMeter = FTangoDevice::GetInstance()->GetWorldToMetersScale();
	float Forward = float(RawPose->orientation[0]);
	float Right = float(RawPose->orientation[1]);
	float Up = float(RawPose->orientation[2]);
	FQuat Orientation(Forward, Right, Up, float(RawPose->orientation[3]));
	Forward = float(UnrealUnitsPerMeter*RawPose->translation[0]);
	Right = float(UnrealUnitsPerMeter*RawPose->translation[1]);
	Up = float(UnrealUnitsPerMeter*RawPose->translation[2]);
	FVector Position(Forward, Right, Up);
	return FTransform(Orientation, Position);
}
#endif

bool FTangoMotion::GetPoseAtTime_Blocking(ETangoReferenceFrame TargetFrame, double Timestamp, FTangoPose& OutTangoPose, bool bIgnoreDisplayRotation)
{
	bool bHasValidPose = false;

	bHasValidPose = GetPoseAtTime(TargetFrame, Timestamp, OutTangoPose, bIgnoreDisplayRotation);

	if (!bHasValidPose)
	{
		FScopeLock ScopeLock(&WaitingAvailablePose);

		if (NewPoseAvailable == nullptr)
		{
			NewPoseAvailable = FPlatformProcess::GetSynchEventFromPool(false);
		}

		bWaitingForNewPose = true;
		int RetryTimes = 0;
		do
		{
			RetryTimes++;
			// We explicitly break the loop after we tried 5 times and in case
			// we still have the NewPoseAvailable triggered but not able to get a valid pose given the time stamp.
			if (RetryTimes > 5 || !NewPoseAvailable->Wait(50))
			{
				// tango core probably disconnected, give up
				UE_LOG(LogTango, Error, TEXT("Timed out waiting for GetPoseAtTime at Timestamp %f"), Timestamp);
				break;
			}

			FPlatformProcess::Sleep(0.001);
			bHasValidPose = GetPoseAtTime(TargetFrame, Timestamp, OutTangoPose, bIgnoreDisplayRotation);
		} while (!bHasValidPose);
		bWaitingForNewPose = false;
	}

	return bHasValidPose;
}

bool FTangoMotion::GetPoseAtTime(ETangoReferenceFrame TargetFrame, double Timestamp, FTangoPose& OutTangoPose, bool bIgnoreDisplayRotation)
{
#if PLATFORM_ANDROID
	TangoCoordinateFrameType Base = static_cast<TangoCoordinateFrameType>(BaseFrame);
	TangoCoordinateFrameType Target = static_cast<TangoCoordinateFrameType>(TargetFrame);
	TangoPoseData RawPose;
	TangoSupportRotation DisplayRotation = ROTATION_IGNORED;
	if (!bIgnoreDisplayRotation)
	{
		switch (FTangoAndroidHelper::GetDisplayRotation())
		{
		case 1:
			DisplayRotation = ROTATION_90;
			break;
		case 2:
			DisplayRotation = ROTATION_180;
			break;
		case 3:
			DisplayRotation = ROTATION_270;
			break;
		default:
			DisplayRotation = ROTATION_0;
			break;
		}
	}
	auto Engine = TANGO_SUPPORT_ENGINE_UNREAL;
	TangoErrorType PoseFetchResult;
	const TangoSupportRotation RequestedDisplayRotation = DisplayRotation;
	// @TODO with the current tango [support library] apparently
	// only certain frames are valid targets when the base is GLOBAL_WGS84 so we'll use
	// one that we know is valid as an intermediate frame and then convert from the actual
	// target as a second step
	const ETangoReferenceFrame IntermediateFrame = ETangoReferenceFrame::AREA_DESCRIPTION;
	if (BaseFrame == ETangoReferenceFrame::GLOBAL_WGS84 && TargetFrame != IntermediateFrame)
	{
		DisplayRotation = ROTATION_IGNORED;
		Target = static_cast<TangoCoordinateFrameType>(IntermediateFrame);
	}
	PoseFetchResult = TangoSupport_getPoseAtTime(
		Timestamp, Base, Target, Engine, Engine, DisplayRotation, &RawPose
	);
	if (PoseFetchResult != TANGO_SUCCESS)
	{
		UE_LOG(LogTango, Error, TEXT("getPoseAtTime failed: timestamp %f, base %d, target %d"), Timestamp, Base, Target);
		return false;
	}
	if (RawPose.status_code != TANGO_POSE_VALID)
	{
		return false;
	}
	if (FTangoDevice::GetInstance()->IsUsingEcef())
	{
		FECEF_Transform Current(RawPose.translation, RawPose.orientation);
		if (!bIsEcefPoseValid)
		{
			FECEF_Transform WorldSpaceOrigin = Current.FromUnreal();
			UE_LOG(LogTango, Log, TEXT("Setting world space origin: %s"), *WorldSpaceOrigin.ToString());
			UEcefToolsLibrary::SetWorldSpaceOrigin(WorldSpaceOrigin);
			bIsEcefPoseValid = true;
		}
		FECEF_Transform WorldSpaceOriginInverse = UEcefToolsLibrary::GetWorldSpaceOriginInverse();
		FECEF_Transform LocalCurrent;
		UEcefToolsLibrary::ComposeECEF_Transforms(WorldSpaceOriginInverse, Current.FromUnreal(), LocalCurrent);
		LocalCurrent.ToUnreal().CopyTo(RawPose.translation, RawPose.orientation);
	}
	FTransform Result;
	Result = ConvertTangoPoseToTransform(&RawPose);
	if (BaseFrame == ETangoReferenceFrame::GLOBAL_WGS84 && TargetFrame != IntermediateFrame)
	{
		FTransform IntermediateFrameToWorldSpaceOriginTransform = Result;
		Base = static_cast<TangoCoordinateFrameType>(IntermediateFrame);
		Target = static_cast<TangoCoordinateFrameType>(TargetFrame);
		PoseFetchResult = TangoSupport_getPoseAtTime(
			Timestamp,
			Base, Target,
			Engine, Engine,
			RequestedDisplayRotation,
			&RawPose
		);
		if (PoseFetchResult != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("Couldn't get pose for target: %d"), Target);
			return false;
		}
		if (RawPose.status_code != TANGO_POSE_VALID)
		{
			return false;
		}
		FTransform TargetToIntermediateFrameTransform = ConvertTangoPoseToTransform(&RawPose);
		Result = TargetToIntermediateFrameTransform * IntermediateFrameToWorldSpaceOriginTransform;
	}
	OutTangoPose.Timestamp.TimestampValue = RawPose.timestamp;
	OutTangoPose.Pose = Result;
	return true;
#else
	return false;
#endif
}
