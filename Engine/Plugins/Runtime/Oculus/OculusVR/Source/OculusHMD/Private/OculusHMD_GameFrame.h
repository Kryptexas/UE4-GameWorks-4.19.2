// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD_Settings.h"
#include "ShowFlags.h"

namespace OculusHMD
{

//-------------------------------------------------------------------------------------------------
// FGameFrame
//-------------------------------------------------------------------------------------------------

class FGameFrame : public TSharedFromThis<FGameFrame, ESPMode::ThreadSafe>
{
public:
	uint32 FrameNumber;				// current frame number. (StartGameFrame_GameThread)
	float WorldToMetersScale;		// World units (UU) to Meters scale. (OnStartGameFrame)
	float MonoCullingDistance;		// Monoscopic camera culling distance (OnStartGameFrame)
	FVector PositionScale;			// (GetCurrentHMDPose)
	FVector2D WindowSize;			// actual window size (StartGameFrame_GameThread)
	FEngineShowFlags ShowFlags;		// (PreRenderViewFamily_RenderThread)
	FQuat PlayerOrientation;		// (CalculateStereoViewOffset, PreRenderViewFamily_RenderThread)
	FVector PlayerLocation;			// (CalculateStereoViewOffset)

	union
	{
		struct
		{
			/** True, if HMD orientation was applied during the game thread */
			uint64			bOrientationChanged : 1; // (ApplyHmdRotation, UpdatePlayerCamera, GetCurrentHMDPose)
			/** True, if HMD position was applied during the game thread */
			uint64			bPositionChanged : 1; // (ApplyHmdRotation, UpdatePlayerCamera, GetCurrentHMDPose)
			/** True, if ApplyHmdRotation was used */
			uint64			bPlayerControllerFollowsHmd : 1; // (UseImplicitHmdPosition, ApplyHmdRotation)
			/** True, if splash is shown */
			uint64			bSplashIsShown : 1;
		};
		uint64 Raw;
	} Flags;

	FPose HeadPose;			// head pose (DoEnableStereo, OnStartGameFrame, PreRenderViewFamily_RenderThread)
	FPose EyePose[3];		// eye poses (DoEnableStereo, OnStartGameFrame, PreRenderViewFamily_RenderThread)
	FPose GameHeadPose;		// head pose used by Game thread (GetCurrentPose)
	FPose GameEyePose[3];	// eye poses used by Game thread (GetCurrentPose)

public:
	FGameFrame();

	TSharedPtr<FGameFrame, ESPMode::ThreadSafe> Clone() const;

	void SaveGamePose();
	void ResetToGamePose();
};

typedef TSharedPtr<FGameFrame, ESPMode::ThreadSafe> FGameFramePtr;

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
