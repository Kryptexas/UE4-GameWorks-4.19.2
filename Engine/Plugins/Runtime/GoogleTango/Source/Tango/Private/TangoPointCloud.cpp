// Copyright 2017 Google Inc.

#include "TangoPointCloud.h"
#include "TangoPluginPrivate.h"
#include "TangoAndroidHelper.h"
#include "TangoLifecycle.h"
#include "TangoARCamera.h"
#include "TangoPrimitives.h"

#include "Engine/Engine.h" // for GEngine
#include "Misc/ScopeLock.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"

#if PLATFORM_ANDROID
#include "tango_client_api_dynamic.h"

namespace
{
	FTangoPointCloud* TangoPointCloudPtr = nullptr;
	void OnPointCloudAvailableRouter(void* Context, const TangoPointCloud* PointCloud)
	{
		if (TangoPointCloudPtr != nullptr)
		{
			TangoPointCloudPtr->HandleOnPointCloudAvailable(PointCloud);
		}
	}
}
#endif

FTangoPointCloud::FTangoPointCloud()
{
	TangoToUnrealCameraMatrix = FMatrix::Identity;
	TangoToUnrealCameraMatrix.M[0][0] = 0;
	TangoToUnrealCameraMatrix.M[2][0] = 1;
	TangoToUnrealCameraMatrix.M[1][1] = 0;
	TangoToUnrealCameraMatrix.M[0][1] = 1;
	TangoToUnrealCameraMatrix.M[2][2] = 0;
	TangoToUnrealCameraMatrix.M[1][2] = -1;
	TangoToUnrealCameraTransform.SetFromMatrix(TangoToUnrealCameraMatrix);
}

void FTangoPointCloud::UpdateBaseFrame(ETangoReferenceFrame InBaseFrame)
{
	BaseFrame = InBaseFrame;
}

void FTangoPointCloud::UpdatePointCloud()
{
#if PLATFORM_ANDROID
	if (PointCloudManager == nullptr)
	{
		return;
	}
	TangoPointCloud* PointCloud = nullptr;
	TangoPoseData RawPose;
	TangoSupportRotation DisplayRotation = ROTATION_0;
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
		break;
	}
	// @TODO
	// Can't use BaseFrame if it's GLOBAL_WGS84 with getLatestPointCloudWithPose (always fails with invalid pose).
	// So instead we hard code it to start_of_service and only use the timestamp from RawPose.
	// We'll call TangoMotionManager to get the pose which will do the right thing in all cases.
	if (TangoSupport_getLatestPointCloudWithPose(
		PointCloudManager, TANGO_COORDINATE_FRAME_START_OF_SERVICE,
		TANGO_SUPPORT_ENGINE_UNREAL, TANGO_SUPPORT_ENGINE_UNREAL,
		DisplayRotation, &PointCloud, &RawPose) != TANGO_SUCCESS)
	{
		UE_LOG(LogTango, Log, TEXT("getLatestPointCloudWithPose failed"));
		LatestPointCloud = nullptr;
		return;
	}
	double Timestamp = RawPose.timestamp;
	FTangoPose PointCloudPose;
	if (!FTangoDevice::GetInstance()->TangoMotionManager.GetPoseAtTime(
		ETangoReferenceFrame::CAMERA_DEPTH,
		Timestamp,
		PointCloudPose))
	{
		return;
	}
	LatestPointCloud = PointCloud;
	LatestPointCloudPose = PointCloudPose;
#endif
}

bool FTangoPointCloud::FindFloorPlane(
	UWorld* World,
	TMap<int32, int32> & NumUpPoints,
	std::set<int32>& NonNoiseBuckets,
	double& LastPointCloudTimestamp,
	float& PlaneZ
)
{
	if (!GEngine)
	{
		UE_LOG(LogTango, Error, TEXT("FindFloorPlane: No Engine available"));
		return false;
	}
	if (!World)
	{
		UE_LOG(LogTango, Error, TEXT("FindFloorPlane: No World available"));
		return false;
	}
#if PLATFORM_ANDROID
	auto PointCloud = GetLatestPointCloud();
	if (PointCloud == nullptr)
	{
		return false;
	}
	if (PointCloud->timestamp == LastPointCloudTimestamp)
	{
		return false;
	}
	FMatrix DepthToWorldMatrix;
	if (!GetRawDepthToWorldMatrix(PointCloud->timestamp, DepthToWorldMatrix, true))
	{
		return false;
	}
	LastPointCloudTimestamp = PointCloud->timestamp;
	const float UnrealUnitsPerMeter = FTangoDevice::GetInstance()->GetWorldToMetersScale();
	// Count each depth point into a bucket based on its world up value
	for (int32 i = 0; i < PointCloud->num_points; i++)
	{
		// Group similar points into buckets based on sensitivity
		FVector DepthPoint(PointCloud->points[i][0], PointCloud->points[i][1], PointCloud->points[i][2]);
		FVector WorldPoint = DepthToWorldMatrix.TransformPosition(DepthPoint);
		int32 Units = FMath::RoundToInt(WorldPoint.Z / SENSITIVITY) * SENSITIVITY;
		int32* NumPtr = NumUpPoints.Find(Units);
		if (!NumPtr)
		{
			NumPtr = &NumUpPoints.Add(Units, 0);
		}
		int32& Num = *NumPtr;
		Num++;
		// Check if the plane is a non-noise plane.
		if (Num > NOISE_THRESHOLD)
		{
			NonNoiseBuckets.insert(Units);
		}
	}
	if (NonNoiseBuckets.empty())
	{
		return false;
	}
	// Find a plane at the lowest Up value. The Up value must be below the camera's Up position.
	int32 Bucket = *NonNoiseBuckets.begin();
	int32 NumPoints = NumUpPoints[Bucket];
	if (NumPoints > RECOGNITION_THRESHOLD)
	{
		const FVector CameraPosition = GEngine->GetFirstLocalPlayerController(World)->PlayerCameraManager->GetCameraLocation();
		if (Bucket < CameraPosition.Z)
		{
			PlaneZ = Bucket;
			return true;
		}
	}
#endif
	return false;
}

bool FTangoPointCloud::FitPlane(UWorld* World,
	const FVector2D& ScreenPoint,
	FTransform& Pose)
{
#if PLATFORM_ANDROID
	if (World != nullptr)
	{
		if (UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportSize;
			ViewportClient->GetViewportSize(ViewportSize);
			bool bSucceeded;
			DoFitPlane(ScreenPoint.X / ViewportSize.X, ScreenPoint.Y / ViewportSize.Y, Pose, bSucceeded);
			return bSucceeded;
		}
	}
	UE_LOG(LogTango, Error, TEXT("FitPlane: failed to find world from context object"));
#endif
	return false;
}

bool FTangoPointCloud::GetRawDepthToWorldTransform(double Timestamp, FTransform& RawDepthToWorldTransform, bool bIncludeDisplayRotation) const
{
#if PLATFORM_ANDROID
	FTangoPose DepthCameraPose;
	if (!FTangoDevice::GetInstance()->TangoMotionManager.GetPoseAtTime(
		ETangoReferenceFrame::CAMERA_DEPTH, Timestamp, DepthCameraPose, bIncludeDisplayRotation
	))
	{
		return false;
	}
	const float UnrealUnitsPerMeter = FTangoDevice::GetInstance()->GetWorldToMetersScale();
	FTransform UnrealDepthToWorldTransform = DepthCameraPose.Pose;
	FTransform TangoToUnrealScaleTransform(
		FQuat::Identity, FVector(0, 0, 0),
		FVector(UnrealUnitsPerMeter, UnrealUnitsPerMeter, UnrealUnitsPerMeter)
	);
	RawDepthToWorldTransform = TangoToUnrealScaleTransform * TangoToUnrealCameraTransform * UnrealDepthToWorldTransform;
	return true;
#else
	RawDepthToWorldTransform = FTransform::Identity;
	return false;
#endif
}

#if PLATFORM_ANDROID

TangoPointCloud* FTangoPointCloud::GetLatestPointCloud()
{
	return LatestPointCloud;
}

FTangoPose FTangoPointCloud::GetLatestPointCloudPose()
{
	return LatestPointCloudPose;
}

void FTangoPointCloud::DoFitPlane(float U, float V, FTransform& Pose, bool& Result)
{
	const TangoPointCloud* PointCloud = GetLatestPointCloud();
	if (PointCloud == nullptr)
	{
		UE_LOG(LogTango, Error, TEXT("FitPlane: No point cloud available yet"));
		Pose = FTransform::Identity;
		Result = false;
		return;
	}
	else if (PointCloud->num_points < 50)
	{
		UE_LOG(LogTango, Error, TEXT("FitPlane: Point cloud number < 50"));
		Pose = FTransform::Identity;
		Result = false;
		return;
	}
	/// Calculate the conversion from the latest depth camera position to the
	/// position of the most recent color camera image. This corrects for screen
	/// lag between the two systems.

	TangoPoseData pose_color_camera_t0_T_depth_camera_t1;
	double camera_time_stamp = FTangoDevice::GetInstance()->TangoARCameraManager.GetColorCameraImageTimestamp();

	int32 ret = TangoSupport_calculateRelativePose(
		PointCloud->timestamp, TANGO_COORDINATE_FRAME_CAMERA_DEPTH,
		camera_time_stamp, TANGO_COORDINATE_FRAME_CAMERA_COLOR,
		&pose_color_camera_t0_T_depth_camera_t1);
	if (ret != TANGO_SUCCESS)
	{
		UE_LOG(LogTango, Error, TEXT("FitPlane: could not calculate relative pose"));
		Result = false;
		return;
	}
	TangoSupportRotation DisplayRotation = ROTATION_0;
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
		break;
	}
	float UV[2] = { U, V };
	double double_depth_position[3];
	double double_depth_plane_equation[4];
	double identity_translation[3] = { 0.0, 0.0, 0.0 };
	double identity_orientation[4] = { 0.0, 0.0, 0.0, 1.0 };
	if (TangoSupport_fitPlaneModelNearPoint(
		PointCloud, identity_translation, identity_orientation,
		UV,
		DisplayRotation,
		pose_color_camera_t0_T_depth_camera_t1.translation,
		pose_color_camera_t0_T_depth_camera_t1.orientation,
		double_depth_position, double_depth_plane_equation) != TANGO_SUCCESS)
	{
		UE_LOG(LogTango, Log, TEXT("fitPlaneModelNearPoint failed"));
		Result = false;
		return;
	}
	FMatrix DepthToWorldMatrix;
	if (!GetRawDepthToWorldMatrix(PointCloud->timestamp, DepthToWorldMatrix, true))
	{
		return;
	}
	FVector DepthPosition(
		(float)double_depth_position[0],
		(float)double_depth_position[1],
		(float)double_depth_position[2]
	);
	FPlane DepthPlane(
		(float)double_depth_plane_equation[0],
		(float)double_depth_plane_equation[1],
		(float)double_depth_plane_equation[2],
		-(float)double_depth_plane_equation[3]
	);
	FVector WorldForward = DepthPlane.TransformBy(DepthToWorldMatrix);
	WorldForward.Normalize();
	FVector WorldPoint = DepthToWorldMatrix.TransformPosition(DepthPosition);
	FVector WorldUp(0, 0, 1);
	if ((WorldForward | WorldUp) > 0.5f)
	{
		WorldUp = FVector(1, 0, 0);
	}
	FVector WorldRight = WorldForward^WorldUp;
	WorldRight.Normalize();
	WorldUp = WorldForward^WorldRight;
	WorldUp.Normalize();
	Pose = FTransform(WorldForward, WorldRight, WorldUp, WorldPoint);
	Result = true;
}

void FTangoPointCloud::HandleOnPointCloudAvailable(const TangoPointCloud* PointCloud)
{
	FScopeLock ScopeLock(&PointCloudLock);
	if (PointCloudManager != nullptr && PointCloud != nullptr)
	{
		if (TangoSupport_updatePointCloud(PointCloudManager, PointCloud) != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("TangoSupport_updatePointCloud failed"));
		}
	}
}

bool FTangoPointCloud::ConnectPointCloud(TangoConfig Config)
{
	UE_LOG(LogTango, Log, TEXT("Connecting Tango Point Cloud..."));
	if (!OnTangoDisconnectedHandle.IsValid())
	{
		FTangoDevice::GetInstance()->OnTangoServiceUnboundDelegate.AddRaw(this, &FTangoPointCloud::DisconnectPointCloud);
	}

	FScopeLock ScopeLock(&PointCloudLock);
	if (PointCloudManager != nullptr)
	{
		// Aready connected
		return true;
	}
	int32 MaxPointCloudElements = 0;
	if (Config == nullptr)
	{
		return false;
	}
	bool bSuccess = TangoConfig_getInt32_dynamic(Config, "max_point_cloud_elements", &MaxPointCloudElements) == TANGO_SUCCESS;
	if (bSuccess)
	{
		int32 ret = TangoSupport_createPointCloudManager(MaxPointCloudElements, &PointCloudManager);
		if (ret != TANGO_SUCCESS)
		{
			UE_LOG(LogTango, Error, TEXT("createPointCloudManager failed with error code: %d"), ret);
		}
		else
		{
			UE_LOG(LogTango, Log, TEXT("Created point cloud manager"));
			TangoPointCloudPtr = this;
			ret = TangoService_connectOnPointCloudAvailable_dynamic(OnPointCloudAvailableRouter);

			if (ret != TANGO_SUCCESS)
			{
				UE_LOG(LogTango, Error, TEXT("connectOnPointCloudAvailable failed with error code: %d"), ret);
			}
			else
			{
				UE_LOG(LogTango, Log, TEXT("Connected point cloud available callback"));
			}
		}
		UE_LOG(LogTango, Log, TEXT("Tango Point Cloud connected"));
		return true;
	}
	else
	{
		UE_LOG(LogTango, Error, TEXT("TangoPointCloud construction failed because read of max_point_cloud_elements was not successful."));
	}
	return false;
}

void FTangoPointCloud::DisconnectPointCloud()
{
	FScopeLock ScopeLock(&PointCloudLock);
	if (PointCloudManager != nullptr)
	{
		TangoSupport_freePointCloudManager(PointCloudManager);
		PointCloudManager = nullptr;
		UE_LOG(LogTango, Log, TEXT("Tango Point Cloud disconnected"));
	}
}

void FTangoPointCloud::EvalPointCloud(TFunction<void(const TangoPointCloud*)> Func)
{
	FScopeLock ScopeLock(&PointCloudLock);
	Func(GetLatestPointCloud());
}

#endif
