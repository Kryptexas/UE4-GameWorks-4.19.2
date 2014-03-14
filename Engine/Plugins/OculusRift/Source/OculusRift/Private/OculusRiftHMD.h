// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#if OCULUS_RIFT_SUPPORTED_PLATFORMS

#include "IHeadMountedDisplay.h"
#include "SceneViewExtension.h"


using namespace OVR;

/**
 * Oculus Rift Head Mounted Display
 */
class FOculusRiftHMD : public IHeadMountedDisplay, public ISceneViewExtension
{

public:
	/** IHeadMountedDisplay interface */
	virtual bool IsHMDEnabled() const OVERRIDE;
	virtual void EnableHMD(bool allow = true) OVERRIDE;
	virtual EHMDDeviceType::Type GetHMDDeviceType() const OVERRIDE;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) const OVERRIDE;

	virtual bool DoesSupportPositionalTracking() const OVERRIDE;
	virtual bool HasValidTrackingPosition() const OVERRIDE;
	virtual void GetPositionalTrackingCameraProperties(FVector& OutOrigin, FRotator& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const OVERRIDE;

	virtual void SetUserDistanceToScreenModifier(float NewUserDistanceToScreenModifier) OVERRIDE;
	virtual float GetUserDistanceToScreenModifier() const OVERRIDE;
	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) OVERRIDE;
	virtual float GetInterpupillaryDistance() const OVERRIDE;
    virtual float GetFieldOfView() const OVERRIDE;

    virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) const OVERRIDE;
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) OVERRIDE;
	virtual void UpdatePlayerCameraRotation(APlayerCameraManager*, struct FMinimalViewInfo& POV) OVERRIDE;

	virtual float GetLensCenterOffset() const OVERRIDE;
    virtual float GetDistortionScalingFactor() const OVERRIDE;
    virtual void GetDistortionWarpValues(FVector4& K) const OVERRIDE;
	virtual bool GetChromaAbCorrectionValues(FVector4& K) const OVERRIDE;
	virtual bool IsChromaAbCorrectionEnabled() const OVERRIDE;

	virtual class ISceneViewExtension* GetViewExtension() OVERRIDE;
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE;
	virtual void OnScreenModeChange(bool bFullScreenNow) OVERRIDE;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const OVERRIDE;
	virtual bool EnableStereo(bool stereo = true) OVERRIDE;
    virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const OVERRIDE;
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, 
										   const float MetersToWorld, FVector& ViewLocation) const OVERRIDE;
	virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const OVERRIDE;
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) OVERRIDE;
	virtual void PushViewportCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FViewport *InViewport) const OVERRIDE;
	virtual void PushViewCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FSceneView *InView) const OVERRIDE;

    /** ISceneViewExtension interface */
    virtual void ModifyShowFlags(FEngineShowFlags& ShowFlags) OVERRIDE;
    virtual void SetupView(FSceneView& InView) OVERRIDE;
    virtual void PreRenderView_RenderThread(FSceneView& InView) OVERRIDE;

	/** Positional tracking control methods */
	virtual bool IsPositionalTrackingEnabled() const OVERRIDE;
	virtual bool EnablePositionalTracking(bool enable) OVERRIDE;

	/** A hookup for latency tester (render thread). */
	virtual bool GetLatencyTesterColor_RenderThread(FColor& color, const FSceneView& view) OVERRIDE;

	virtual bool IsHeadTrackingAllowed() const OVERRIDE;

	virtual bool IsInLowPersistenceMode() const OVERRIDE;
	virtual void EnableLowPersistenceMode(bool Enable = true) OVERRIDE;

	/** Resets orientation by setting roll and pitch to 0, 
	    assuming that current yaw is forward direction and assuming
		current position as 0 point. */
	virtual void ResetOrientationAndPosition(float yaw = 0.f) OVERRIDE;

	virtual void UpdateScreenSettings(const FViewport*) OVERRIDE;

#if !UE_BUILD_SHIPPING
    /** Debugging functionality */
	virtual bool HandleInputKey(UPlayerInput*, FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad);
	virtual bool HandleInputAxis(UPlayerInput*, FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);
#endif // !UE_BUILD_SHIPPING

	/** Constructor */
	FOculusRiftHMD();

	/** Destructor */
	virtual ~FOculusRiftHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;

private:
	/**
	 * Starts up the Oculus Rift device
	 */
	void Startup();

	/**
	 * Shuts down Oculus Rift
	 */
	void Shutdown();

	/**
	 * Reads the device configuration, and sets up the stereoscopic rendering parameters
	 */
	void UpdateStereoRenderingParams();

	/**
	 * Calculates the distortion scaling factor used in the distortion postprocess
	 */
    float CalcDistortionScale(float InScale);

    /**
     * Updates the view point reflecting the current HMD orientation. 
     */
	void UpdatePlayerViewPoint(const FQuat& CurrentOrientation, const FVector& CurrentPosition, const FQuat& BaseViewOrientation, const FVector& BaseViewPosition, FRotator& ViewRotation, FVector& ViewLocation);

	/**
	 * Converts quat from Oculus ref frame to Unreal
	 */
	template <typename OVRQuat>
	FORCEINLINE FQuat ToFQuat(const OVRQuat& InQuat) const
	{
		return FQuat(float(-InQuat.z), float(InQuat.x), float(InQuat.y), float(-InQuat.w));
	}
	/**
	 * Converts vector from Oculus to Unreal
	 */
	template <typename OVRVector3>
	FORCEINLINE FVector ToFVector(const OVRVector3& InVec) const
	{
		return FVector(float(-InVec.z), float(InVec.x), float(InVec.y));
	}

	/**
	 * Converts vector from Oculus to Unreal, also converting meters to UU (Unreal Units)
	 */
	template <typename OVRVector3>
	FORCEINLINE FVector ToFVector_M2U(const OVRVector3& InVec) const
	{
		return FVector(float(-InVec.z * WorldToMetersScale), 
			           float(InVec.x  * WorldToMetersScale), 
					   float(InVec.y  * WorldToMetersScale));
	}
	/**
	 * Converts vector from Oculus to Unreal, also converting UU (Unreal Units) to meters.
	 */
	template <typename OVRVector3>
	FORCEINLINE OVRVector3 ToOVRVector_U2M(const FVector& InVec) const
	{
		return OVRVector3(float(InVec.Y * (1.f / WorldToMetersScale)), 
			              float(InVec.Z * (1.f / WorldToMetersScale)), 
					     float(-InVec.X * (1.f / WorldToMetersScale)));
	}
	/**
	 * Converts vector from Oculus to Unreal.
	 */
	template <typename OVRVector3>
	FORCEINLINE OVRVector3 ToOVRVector(const FVector& InVec) const
	{
		return OVRVector3(float(InVec.Y), float(InVec.Z), float(-InVec.X));
	}

	/** Converts vector from meters to UU (Unreal Units) */
	FORCEINLINE FVector MetersToUU(const FVector& InVec) const
	{
		return InVec * WorldToMetersScale;
	}
	/** Converts vector from UU (Unreal Units) to meters */
	FORCEINLINE FVector UUToMeters(const FVector& InVec) const
	{
		return InVec * (1.f / WorldToMetersScale);
	}

	/**
	 * Called when state changes from 'stereo' to 'non-stereo'. Suppose to distribute
	 * the event further to user's code (?).
	 */
	void OnOculusStateChange(bool bIsEnabledNow);

	/** Load/save settings */
	void LoadFromIni();
	void SaveToIni();

	/** Applies overrides on system when switched to stereo (such as VSync and ScreenPercentage) */
	void ApplySystemOverridesOnStereo(bool force = false);
	/** Saves system values before applying overrides. */
	void SaveSystemValues();
	/** Restores system values after overrides applied. */
	void RestoreSystemValues();

	void ProcessLatencyTesterInput() const;
	void ResetControlRotation() const;

	/** Get/set head model. Units are meters, not UU! 
	    Use ToFVector and ToFVector_M2U conversion routines. */
	OVR::Vector3d GetHeadModel() const;
	void		  SetHeadModel(const OVR::Vector3d&);

#if !UE_BUILD_SHIPPING
	void DrawDebugTrackingCameraFrustum(const FVector& ViewLocation);
#endif // #if !UE_BUILD_SHIPPING

private: // data
	friend class FOculusMessageHandler;

	/** Whether or not the Oculus was successfully initialized */
	bool bWasInitialized;

	/** Whether stereo is currently on or off. */
	bool bStereoEnabled;

	/** Whether or not switching to stereo is allowed */
	bool bHMDEnabled;

	/** Debugging:  Whether or not the stereo rendering settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
    bool bOverrideStereo;

	/** Debugging:  Whether or not the IPD setting have been manually overridden by an exec command. */
	bool bOverrideIPD;

	/** Debugging:  Whether or not the distortion settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
	bool bOverrideDistortion;

	/** Debugging: Allows changing internal params, such as screen size, eye-to-screen distance, etc */
	bool bDevSettingsEnabled;

	/** Whether or not to override game VSync setting when switching to stereo */
	bool bOverrideVSync;
	/** Overridden VSync value */
	bool bVSync;

	/** Saved original values for VSync and ScreenPercentage. */
	bool bSavedVSync;
	float SavedScrPerc;

	/** Whether or not to override game ScreenPercentage setting when switching to stereo */
	bool bOverrideScreenPercentage;
	/** Overridden ScreenPercentage value */
	float ScreenPercentage;

	/** Allows renderer to finish current frame. Setting this to 'true' may reduce the total 
	 *  framerate (if it was above vsync) but will reduce latency. */
	bool bAllowFinishCurrentFrame;

	/** Interpupillary distance, in meters (user configurable) */
	float InterpupillaryDistance;

	/** World units (UU) to Meters scale.  Read from the level, and used to transform positional tracking data */
	float WorldToMetersScale;

	/** User-tunable modification to the interpupillary distance */
	float UserDistanceToScreenModifier;

    /** The FOV to render at (radians), based on the physical characteristics of the device */
    float FOV;

    /** Motion prediction (in seconds) */
    float MotionPrediction;

	/** Gain for gravity correction (should not need to be changed) */
	float AccelGain;

	/** Whether or not to use a head model offset to determine view position */
	bool bUseHeadModel;

    /** Distortion on/off */
    bool bHmdDistortion;

	/** Distortion coefficients needed to perform the lens-correction warping */
    FVector4 HmdDistortion;

	/** Scaling factor for the post process distortion effect */
	float DistortionScale;

	/** Offset the post process distortion effect */
	//FVector2D DistortionOffset;

	/** Chromatic aberration correction on/off */
	bool bChromaAbCorrectionEnabled;

	/** Chromatic aberration correction coefficients */
	FVector4 ChromaAbCorrection;

	/** Device constants, needed for stereo rendering calculations, calculated in UpdateStereoRenderingParams() */
	float ProjectionCenterOffset;
    float LensCenterOffset;

	/** Whether or not 2D stereo settings overridden. */
	bool bOverride2D;
	/** HUD stereo offset */
	float HudOffset;
	/** Screen center adjustment for 2d elements */
	float CanvasCenterOffset;

	/** Low persistence mode */
	bool bLowPersistenceMode;

	/** Turns on/off updating view's orientation/position on a RenderThread. When it is on,
	    latency should be significantly lower. 
		See 'HMD UPDATEONRT ON|OFF' console command.
	*/
	bool				bUpdateOnRT;
	mutable OVR::Lock	UpdateOnRTLock;

	/** Enforces headtracking to work even in non-stereo mode (for debugging or screenshots). 
	    See 'MOTION ENFORCE' console command. */
	bool bHeadTrackingEnforced;

#if !UE_BUILD_SHIPPING
	/** Draw tracking camera frustum, for debugging purposes. 
	 *  See 'HMDPOS SHOWCAMERA ON|OFF' console command.
	 */
	bool bDrawTrackingCameraFrustum;
#endif

	/** Player's orientation tracking */
	FQuat					CurHmdOrientation;

	FRotator				DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat					DeltaControlOrientation; // same as DeltaControlRotation but as quat

	FVector					CurHmdPosition;
	FQuat					LastHmdOrientation;

	/** HMD base values, specify forward orientation and zero pos offset */
	OVR::Vector3d			BaseOffset;      // base position, in Oculus coords
	FQuat					BaseOrientation; // base orientation

	/** The device manager, in charge of enumeration */
	Ptr<DeviceManager>		pDevManager;

	/** Oculus orientation sensor device */
	Ptr<SensorDevice>		pSensor;

	/** The Oculus device itself */
	Ptr<HMDDevice>			pHMD;

	/** Optional latency tester */
	Ptr<LatencyTestDevice>	pLatencyTester;
	mutable OVR::Lock       LatencyTestLock;
	Util::LatencyTest*		pLatencyTest;
	OVR::Color				LatencyTestColor;
	OVR::AtomicInt<uint32>	LatencyTestFrameNumber;

	SensorFusion*			pSensorFusion;
	HMDInfo					DeviceInfo;

	class FOculusMessageHandler* pMsgHandler;

	/** True, if pos tracking is enabled */
	bool						bHmdPosTracking;

#ifdef OVR_VISION_ENABLED
	Ptr<OVR::CameraDevice>		pCamera;
	OVR::Vision::PoseTracker*	pPoseTracker;
	float						PositionScaleFactor; // scale factor for pos tracking vector
#if !UE_BUILD_SHIPPING
	// debugging positional tracking
	Vector3d					SimPosition;
	bool						bPosTrackingSim;
	mutable bool				bHaveVisionTracking;
#endif // #if !UE_BUILD_SHIPPING

#else // NO VISION
	OVR::Vector3d				HeadModel_Meters; // in meters
#endif // OCULUS_USE_VISION
};

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS
