// Copyright 2017 Google Inc.

#pragma once
#include "TangoPrimitives.h"

class FTangoMotion
{
	friend class FTangoDevice;
public:
	FTangoMotion();
	~FTangoMotion();

	bool ConnectOnPoseAvailable(ETangoReferenceFrame PoseBaseFrame);

	void DisconnectOnPoseAvailable();

	bool GetCurrentPose(ETangoReferenceFrame TargetFrame, FTangoPose& OutTangoPose);

	bool IsCurrentPoseValid(ETangoReferenceFrame TargetFrame);

	bool GetPoseAtTime(ETangoReferenceFrame TargetFrame, double Timestamp, FTangoPose& OutTangoPose, bool bIgnoreDisplayRotation = false);
	
	/* This function will force to get the pose at the given timestamp by always retrying when failed, unless it didn't get the PoseAvailable signal for too long
	   Note that the function could block the caller thread so use caution.
	*/
	bool GetPoseAtTime_Blocking(ETangoReferenceFrame TargetFrame, double Timestamp, FTangoPose& OutTangoPose, bool bIgnoreDisplayRotation = false);

#if PLATFORM_ANDROID
	FTransform ConvertTangoPoseToTransform(const struct TangoPoseData* TangoPose);
#endif

	void OnTangoPoseUpdated();

private:

	void UpdateBaseFrame(ETangoReferenceFrame InBaseFrame);
	
	void UpdateTangoPoses();

private:
	ETangoReferenceFrame BaseFrame;

	FTangoPose LatestDevicePose;
	FTangoPose LatestColorCameraPose;

	bool bIsDevicePoseValid;
	bool bIsColorCameraPoseValid;
	bool bIsEcefPoseValid;
	bool bWaitingForNewPose;
	FEvent* NewPoseAvailable;

	FCriticalSection WaitingAvailablePose;
};
