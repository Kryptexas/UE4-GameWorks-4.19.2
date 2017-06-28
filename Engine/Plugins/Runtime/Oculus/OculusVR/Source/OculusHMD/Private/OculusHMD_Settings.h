// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS

namespace OculusHMD
{

static const float ClampPixelDensityMin = 0.5f;
static const float ClampPixelDensityMax = 2.0f;


//-------------------------------------------------------------------------------------------------
// FSettings
//-------------------------------------------------------------------------------------------------

class FSettings : public TSharedFromThis<FSettings, ESPMode::ThreadSafe>
{
public:
	union
	{
		struct
		{
			/** Whether stereo is currently on or off. */
			uint64 bStereoEnabled : 1;

			/** Whether or not switching to stereo is allowed */
			uint64 bHMDEnabled : 1;

			/** Chromatic aberration correction on/off */
			uint64 bChromaAbCorrectionEnabled : 1;

			/** Turns on/off updating view's orientation/position on a RenderThread. When it is on,
				latency should be significantly lower.
				See 'HMD UPDATEONRT ON|OFF' console command.
			*/
			uint64 bUpdateOnRT : 1;

			/** Enforces headtracking to work even in non-stereo mode (for debugging or screenshots).
				See 'MOTION ENFORCE' console command. */
			uint64 bHeadTrackingEnforced : 1;

			/** Allocate an high quality OVR_FORMAT_R11G11B10_FLOAT buffer for Rift */
			uint64 bHQBuffer : 1;

			/** True, if Far/Mear clipping planes got overriden */
			uint64				bClippingPlanesOverride : 1;

			/** True, if PlayerCameraManager should follow HMD orientation */
			uint64				bPlayerCameraManagerFollowsHmdOrientation : 1;

			/** True, if PlayerCameraManager should follow HMD position */
			uint64				bPlayerCameraManagerFollowsHmdPosition : 1;

			/** Rendering should be (could be) paused */
			uint64				bPauseRendering : 1;

			/** HQ Distortion */
			uint64				bHQDistortion : 1;

			/* plugin-allocated multiview buffer (GL_TEXTURE_2D_ARRAY) for mobile */
			uint64				bDirectMultiview : 1; 

#if !UE_BUILD_SHIPPING
			/** Turns off updating of orientation/position on game thread. See 'hmd updateongt' cmd */
			uint64				bDoNotUpdateOnGT : 1;

			/** Show status / statistics on screen. See 'hmd stats' cmd */
			uint64				bShowStats : 1;

			/** Draw lens centered grid */
			uint64				bDrawGrid : 1;
#endif
		};
		uint64 Raw;
	} Flags;

	/** Optional far clipping plane for projection matrix */
	float NearClippingPlane;

	/** Optional far clipping plane for projection matrix */
	float FarClippingPlane;

	/** HMD base values, specify forward orientation and zero pos offset */
	FVector BaseOffset; // base position, in meters, relatively to the sensor //@todo hmd: clients need to stop using oculus space
	FQuat BaseOrientation; // base orientation

	/** Viewports for each eye, in render target texture coordinates */
	FIntRect EyeRenderViewport[3];

	/** Maximum adaptive resolution viewports for each eye, in render target texture coordinates */
	FIntRect EyeMaxRenderViewport[3];

	/** Deprecated position offset */
	FVector PositionOffset;

	ovrpMatrix4f EyeProjectionMatrices[3]; // 0 - left, 1 - right, same as Views
	ovrpMatrix4f PerspectiveProjection[3]; // used for calc ortho projection matrices

	FIntPoint RenderTargetSize;
	float PixelDensity;
	float PixelDensityMin;
	float PixelDensityMax;
	bool bPixelDensityAdaptive;

	// Used to convert between screen percentage and pixel density
	float IdealScreenPercentage;
	float CurrentScreenPercentage;

	ovrpSystemHeadset SystemHeadset;

	float VsyncToNextVsync;

public:
	FSettings();
	virtual ~FSettings() {}

	bool IsStereoEnabled() const { return Flags.bStereoEnabled && Flags.bHMDEnabled; }

	void UpdateScreenPercentageFromPixelDensity();
	bool UpdatePixelDensityFromScreenPercentage();

	TSharedPtr<FSettings, ESPMode::ThreadSafe> Clone() const;
};

typedef TSharedPtr<FSettings, ESPMode::ThreadSafe> FSettingsPtr;

} // namespace OculusHMD

#endif //OCULUS_HMD_SUPPORTED_PLATFORMS
