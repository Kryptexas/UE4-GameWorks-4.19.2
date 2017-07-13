// Copyright 2017 Google Inc.

#pragma once
#include "TangoPrimitives.h"
#if PLATFORM_ANDROID
#include "tango_support_api.h"
#endif
#include <set>

class FTangoDevice;

class FTangoPointCloud
{
	friend class FTangoDevice;
public:
	FTangoPointCloud();
	bool FitPlane(UWorld* World, const FVector2D& ScreenPoint, FTransform& Pose);
	bool FindFloorPlane(
		UWorld* World,
		TMap<int32, int32> & NumUpPoints,
		std::set<int32>& NonNoiseBuckets,
		double& LastPointCloudTimestamp,
		float& PlaneZ);
	bool GetRawDepthToWorldTransform(double Timestamp, FTransform& RawDepthToWorldTransform, bool bIgnoreDisplayRotation = false) const;
	bool GetRawDepthToWorldMatrix(double Timestamp, FMatrix& RawDepthToWorldMatrix, bool bIgnoreDisplayRotation = false) const
	{
		FTransform RawDepthToWorldTransform;
		if (!GetRawDepthToWorldTransform(Timestamp, RawDepthToWorldTransform, bIgnoreDisplayRotation))
		{
			RawDepthToWorldMatrix = FMatrix::Identity;
			return false;
		}
		RawDepthToWorldMatrix = RawDepthToWorldTransform.ToMatrixWithScale();
		return true;
	}
	void OnPause()
	{

	}
	void OnResume()
	{

	}
#if PLATFORM_ANDROID
	TangoPointCloud* GetLatestPointCloud();
	FTangoPose GetLatestPointCloudPose();

	void EvalPointCloud(TFunction<void(const TangoPointCloud*)> Func);
	void HandleOnPointCloudAvailable(const TangoPointCloud* PointCloud);

#endif
private:
	void UpdateBaseFrame(ETangoReferenceFrame InBaseFrame);
	void UpdatePointCloud();
#if PLATFORM_ANDROID
	bool ConnectPointCloud(TangoConfig Config);
	void DisconnectPointCloud();
	void DoFitPlane(float U, float V, FTransform& Pose, bool& Result);
#endif
	// constants for floor finding
	/// <summary>
	/// The minimum number of points near a world position y to determine that it is a reasonable floor.
	/// </summary>
	const int32 RECOGNITION_THRESHOLD = 1024;

	/// <summary>
	/// The minimum number of points near a world position y to determine that it is not simply noise points.
	/// </summary>
	const int32 NOISE_THRESHOLD = 512;

	/// <summary>
	/// The interval in meters between buckets of points. For example, a high sensitivity of 0.01 will group
	/// points into buckets every 1cm.
	/// </summary>
	const float SENSITIVITY = 2.0f;
private:
	ETangoReferenceFrame BaseFrame;
	FCriticalSection PointCloudLock;
	FDelegateHandle OnTangoDisconnectedHandle;
#if PLATFORM_ANDROID
	TangoPointCloud* LatestPointCloud;
	FTangoPose LatestPointCloudPose;
	TangoSupportPointCloudManager* PointCloudManager;
#endif
	FMatrix TangoToUnrealCameraMatrix;
	FTransform TangoToUnrealCameraTransform;
};
