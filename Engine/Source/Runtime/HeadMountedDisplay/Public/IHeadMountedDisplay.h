// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StereoRendering.h"

/**
 * The family of HMD device.  Register a new class of device here if you need to branch code for PostProcessing until 
 */
namespace EHMDDeviceType
{
	enum Type
	{
		DT_OculusRift
	};
}

/**
 * HMD device interface
 */
class HEADMOUNTEDDISPLAY_API IHeadMountedDisplay : public IModuleInterface, public IStereoRendering
{

public:
	IHeadMountedDisplay();

	/**
	 * Whether or not switching to stereo is enabled; if it is false, then EnableStereo(true) will do nothing.
	 */
	virtual bool IsHMDEnabled() const = 0;

	/**
	 * Enables or disables switching to stereo.
	 */
	virtual void EnableHMD(bool bEnable = true) = 0;

	/**
	 * Returns the family of HMD device implemented 
	 */
	virtual EHMDDeviceType::Type GetHMDDeviceType() const = 0;

	struct MonitorInfo
	{
		FString MonitorName;
		size_t  MonitorId;
		int		DesktopX, DesktopY;
		int		ResolutionX, ResolutionY;
	};

    /**
     * Get the name or id of the display to output for this HMD. 
     */
	virtual bool	GetHMDMonitorInfo(MonitorInfo&) const = 0;
	
    /**
	 * Calculates the FOV, based on the screen dimensions of the device
	 */
	virtual float	GetFieldOfView() const = 0;

	/**
	 * Whether or not the HMD supports positional tracking (either via camera or other means)
	 */
	virtual bool	DoesSupportPositionalTracking() const = 0;

	/**
	 * If the device has positional tracking, whether or not we currently have valid tracking
	 */
	virtual bool	HasValidTrackingPosition() const = 0;

	/**
	 * If the HMD supports positional tracking via a camera, this returns the frustum properties (all in game-world space) of the tracking camera.
	 */
	virtual void	GetPositionalTrackingCameraProperties(FVector& OutOrigin, FRotator& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const = 0;

	/**
	 * Accessors to modify the user-tweakable distance-to-screen modifier
	 */
	virtual void	SetUserDistanceToScreenModifier(float NewUserDistanceToScreenModifier) = 0;
	virtual float	GetUserDistanceToScreenModifier() const = 0;

	/**
	 * Accessors to modify the interpupillary distance (meters)
	 */
	virtual void	SetInterpupillaryDistance(float NewInterpupillaryDistance) = 0;
	virtual float	GetInterpupillaryDistance() const = 0;

    /**
     * Get the current orientation and position reported by the HMD.
     */
    virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) const = 0;

    /**
     * Get the ISceneViewExtension for this HMD, or none.
     */
    virtual class ISceneViewExtension* GetViewExtension() = 0;

	/**
     * Apply the orientation of the headset to the PC's rotation.
     * If this is not done then the PC will face differently than the camera,
     * which might be good (depending on the game).
     */
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) = 0;

	/**
	 * Apply the orientation of the headset to the Camera's rotation.
	 * This method is called for cameras with bFollowHmdOrientation set to 'true'.
	 */
	virtual void UpdatePlayerCameraRotation(APlayerCameraManager* Camera, struct FMinimalViewInfo& POV) = 0;

  	/**
	 * Gets the scaling factor, applied to the post process warping effect
	 */
	virtual float GetDistortionScalingFactor() const = 0;

	/**
	 * Gets the offset (in clip coordinates) from the center of the screen for the lens position
	 */
	virtual float GetLensCenterOffset() const = 0;

	/**
	 * Gets the barrel distortion shader warp values for the device
	 */
	virtual void GetDistortionWarpValues(FVector4& K) const = 0;

	/**
	 * Returns 'false' if chromatic aberration correction is off.
	 */
	virtual bool IsChromaAbCorrectionEnabled() const = 0;

	/**
	 * Gets the chromatic aberration correction shader values for the device. 
	 * Returns 'false' if chromatic aberration correction is off.
	 */
	virtual bool GetChromaAbCorrectionValues(FVector4& K) const = 0;

	/**
	 * Exec handler to allow console commands to be passed through to the HMD for debugging
	 */
    virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) = 0;

	/**
	 * Saves / loads pre-fullscreen rectangle. Could be used to store saved original window position 
	 * before switching to fullscreen mode.
	 */
	virtual void PushPreFullScreenRect(const FSlateRect& InPreFullScreenRect);
	virtual void PopPreFullScreenRect(FSlateRect& OutPreFullScreenRect);

	/**
	 * A callback that is called when screen mode is changed (fullscreen <-> window). 
	 */
	virtual void OnScreenModeChange(bool bFullScreenNow) = 0;

	/** Returns true if positional tracking enabled and working. */
	virtual bool IsPositionalTrackingEnabled() const = 0;

	/** 
	 * Tries to enable positional tracking.
	 * Returns the actual status of positional tracking. 
	 */
	virtual bool EnablePositionalTracking(bool enable) = 0;

	/**
	 * Acquires color info for latency tester rendering. Returns true if rendering should be performed.
	 */
	virtual bool GetLatencyTesterColor_RenderThread(FColor& color, const FSceneView& view) =  0;

	/**
	 * Returns true, if head tracking is allowed. Most common case: it returns true when GEngine->IsStereoscopic3D() is true,
	 * but some overrides are possible.
	 */
	virtual bool IsHeadTrackingAllowed() const = 0;

	/**
	 * Returns true, if HMD is in low persistence mode. 'false' otherwise.
	 */
	virtual bool IsInLowPersistenceMode() const = 0;

	/**
	 * Switches between low and full persistence modes.
	 *
	 * @param Enable			(in) 'true' to enable low persistence mode; 'false' otherwise
	 */
	virtual void EnableLowPersistenceMode(bool Enable = true) = 0;

	/** 
	 * Resets orientation by setting roll and pitch to 0, assuming that current yaw is forward direction and assuming
	 * current position as a 'zero-point' (for positional tracking). 
	 *
	 * @param Yaw				(in) the desired yaw to be set after orientation reset.
	 */
	virtual void ResetOrientationAndPosition(float Yaw = 0.f) = 0;

	/**
	 * This method is able to change screen settings (such as rendering scale) right before
	 * any drawing occurs. It is called at the beginning of UGameViewportClient::Draw() method.
	 */
	virtual void UpdateScreenSettings(const FViewport* InViewport) = 0;
private:
	/** Stores the dimensions of the window before we moved into fullscreen mode, so they can be restored */
	FSlateRect PreFullScreenRect;
};
