// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "IOculusRiftPlugin.h"
#include "HeadMountedDisplay.h"
#include "IHeadMountedDisplay.h"

#if OCULUS_RIFT_SUPPORTED_PLATFORMS

#include "SceneViewExtension.h"

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif

#define OVR_VISION_ENABLED
//#define OVR_DIRECT_RENDERING
#define OVR_D3D_VERSION 11

#ifdef OVR_VISION_ENABLED
	#ifndef OVR_CAPI_VISIONSUPPORT
		#define OVR_CAPI_VISIONSUPPORT
	#endif
#endif

#if OCULUS_RIFT_SUPPORTED_PLATFORMS
	
	#include "OVRVersion.h"

	#include "../Src/Kernel/OVR_Math.h"
	#include "../Src/Kernel/OVR_Threads.h"
	#include "../Src/OVR_CAPI.h"
	#include "../Src/Kernel/OVR_Color.h"
	#include "../Src/Kernel/OVR_Timer.h"

#ifdef OVR_DIRECT_RENDERING
	#include "AllowWindowsPlatformTypes.h"
	#ifdef OVR_D3D_VERSION
		#include "../Src/OVR_CAPI_D3D.h"
	#endif // OVR_D3D_VERSION
#endif // OVR_DIRECT_RENDERING

	using namespace OVR;

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS

#if PLATFORM_SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif


/**
 * Oculus Rift Head Mounted Display
 */
class FOculusRiftHMD : public IHeadMountedDisplay, public ISceneViewExtension
{

public:
	/** IHeadMountedDisplay interface */
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool allow = true) override;
	virtual EHMDDeviceType::Type GetHMDDeviceType() const override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) const override;

	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() const override;
	virtual void GetPositionalTrackingCameraProperties(FVector& OutOrigin, FRotator& OutOrientation, float& OutHFOV, float& OutVFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;
    //virtual float GetFieldOfViewInRadians() const OVERRIDE;
	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;

    virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) const override;
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
	virtual void UpdatePlayerCameraRotation(APlayerCameraManager*, struct FMinimalViewInfo& POV) override;

	virtual bool IsChromaAbCorrectionEnabled() const override;

	virtual class ISceneViewExtension* GetViewExtension() override;
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override;
	virtual void OnScreenModeChange(EWindowMode::Type WindowMode) override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
    virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, 
										   const float MetersToWorld, FVector& ViewLocation) override;
	virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const override;
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override;
	virtual void PushViewportCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FViewport *InViewport) const override;
	virtual void PushViewCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FSceneView *InView) const override;
	virtual void GetEyeRenderParams_RenderThread(EStereoscopicPass StereoPass, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;

    /** ISceneViewExtension interface */
    virtual void ModifyShowFlags(FEngineShowFlags& ShowFlags) override;
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
    virtual void PreRenderView_RenderThread(FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FSceneViewFamily& InViewFamily) override;

	/** Positional tracking control methods */
	virtual bool IsPositionalTrackingEnabled() const override;
	virtual bool EnablePositionalTracking(bool enable) override;

	/** A hookup for latency tester (render thread). */
	virtual bool GetLatencyTesterColor_RenderThread(FColor& color, const FSceneView& view) override;

	virtual bool IsHeadTrackingAllowed() const override;

	virtual bool IsInLowPersistenceMode() const override;
	virtual void EnableLowPersistenceMode(bool Enable = true) override;

	/** Resets orientation by setting roll and pitch to 0, 
	    assuming that current yaw is forward direction and assuming
		current position as 0 point. */
	virtual void ResetOrientationAndPosition(float yaw = 0.f) override;

	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FSceneView& View, const FIntPoint& TextureSize) override;
	virtual void UpdateScreenSettings(const FViewport*) override;

#ifdef OVR_DIRECT_RENDERING
#if defined(OVR_D3D_VERSION) && (OVR_D3D_VERSION == 11)
	class D3D11Bridge : public ID3D11Bridge
	{
	public:
		D3D11Bridge(FOculusRiftHMD* plugin):Plugin(plugin), bNeedReinitRendererAPI(true), bReadOnly(false) {}

		virtual void Init(void* InD3DDevice, void* InD3DDeviceContext);
		virtual void Reset();

		virtual void InitViewport(void* InRenderTargetView, void* InSwapChain, int ResX, int ResY);
		virtual void ResetViewport();

		virtual bool FinishFrame(int SyncInterval);
	
		virtual void SetRenderTargetTexture(void* TargetableTexture, void* ShaderResourceTexture, int SizeX, int SizeY, int NumSamples);
		virtual FTexture2DRHIRef GetRenderTargetTexture() { return RenderTargetTexture; }

		FOculusRiftHMD*		Plugin;
		ovrD3D11Config		Cfg;
		ovrD3D11Texture		EyeTexture[2];
		FTexture2DRHIRef	RenderTargetTexture;
		bool				bNeedReinitRendererAPI;
		bool				bReadOnly;
	};

	virtual ID3D11Bridge* GetD3D11Bridge() override 
	{ 
		check(IsInGameThread());
		return &mD3D11Bridge; 
	} 

	virtual void UpdateRenderTarget(FRHITexture* TargetableTexture, FRHITexture* ShaderResourceTexture, uint32 SizeX, uint32 SizeY, int NumSamples) override;
	virtual void CalculateRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY) const override;
	virtual bool NeedReAllocateRenderTarget(uint32 InSizeX, uint32 InSizeY) const override;
	virtual bool ShouldUseSeparateRenderTarget() const override 
	{ 
		check(IsInGameThread());
		return IsStereoEnabled(); 
	}
#endif

	void BeginRendering_RenderThread(); //!!AB
	void FinishRendering_RenderThread(); //!!AB
#endif // #ifdef OVR_DIRECT_RENDERING

	virtual void DrawDebug(UCanvas* Canvas) override;

#if 0 // !UE_BUILD_SHIPPING
    /** Debugging functionality */
	virtual bool HandleInputKey(UPlayerInput*, EKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad);
	virtual bool HandleInputAxis(UPlayerInput*, EKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad);
#endif // !UE_BUILD_SHIPPING

	/** Constructor */
	FOculusRiftHMD();

	/** Destructor */
	virtual ~FOculusRiftHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;

private:
	FOculusRiftHMD* getThis() { return this; }

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
	void UpdateHmdRenderInfo();

	/**
	 * Calculates the distortion scaling factor used in the distortion postprocess
	 */
    //float CalcDistortionScale(float InScale);

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

	FORCEINLINE FMatrix ToFMatrix(const OVR::Matrix4f& vtm) const
	{
		// Rows and columns are swapped between OVR::Matrix4f and FMatrix
		return FMatrix(
			FPlane(vtm.M[0][0], vtm.M[1][0], vtm.M[2][0], vtm.M[3][0]),
			FPlane(vtm.M[0][1], vtm.M[1][1], vtm.M[2][1], vtm.M[3][1]),
			FPlane(vtm.M[0][2], vtm.M[1][2], vtm.M[2][2], vtm.M[3][2]),
			FPlane(vtm.M[0][3], vtm.M[1][3], vtm.M[2][3], vtm.M[3][3]));
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
	OVR::Vector3f	GetHeadModel() const;
	void			SetHeadModel(const OVR::Vector3f&);

	void PrecalculatePostProcess_NoLock();

	void UpdateSensorHmdCaps();

	void PoseToOrientationAndPosition(const ovrPosef& InPose, FQuat& OutOrientation, FVector& OutPosition) const;

#if !UE_BUILD_SHIPPING
	void DrawDebugTrackingCameraFrustum(class UWorld* InWorld, const FVector& ViewLocation);
#endif // #if !UE_BUILD_SHIPPING

private: // data
	friend class FOculusMessageHandler;

	/** Whether or not the Oculus was successfully initialized */
	bool bWasInitialized;

	/** Whether stereo is currently on or off. */
	bool bStereoEnabled;

	/** Whether or not switching to stereo is allowed */
	bool bHMDEnabled;

	/** Indicates if it is necessary to update stereo rendering params */
	bool bNeedUpdateStereoRenderingParams;

	/** Debugging:  Whether or not the stereo rendering settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
    bool bOverrideStereo;

	/** Debugging:  Whether or not the IPD setting have been manually overridden by an exec command. */
	bool bOverrideIPD;

	/** Debugging:  Whether or not the distortion settings have been manually overridden by an exec command.  They will no longer be auto-calculated */
	bool bOverrideDistortion;

	/** Debugging: Allows changing internal params, such as screen size, eye-to-screen distance, etc */
	bool bDevSettingsEnabled;

	bool bOverrideFOV;

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

	/** Ideal ScreenPercentage value for the HMD */
	float IdealScreenPercentage;

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
	float HFOVInRadians; // horizontal
	float VFOVInRadians; // vertical

	/** Motion prediction (in seconds). 0 - no prediction */
	double MotionPredictionInSeconds;

	/** Gain for gravity correction (should not need to be changed) */
	float AccelGain;

	/** Whether or not to use a head model offset to determine view position */
	bool bUseHeadModel;

    /** Distortion on/off */
    bool bHmdDistortion;

	/** Chromatic aberration correction on/off */
	bool bChromaAbCorrectionEnabled;

	/** Yaw drift correction on/off */
	bool bYawDriftCorrectionEnabled;

	/** HMD tilt correction on/off */
	bool bTiltCorrectionEnabled;

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
	bool				bDrawTrackingCameraFrustum;

	/** Turns off updating of orientation/position on game thread. See 'hmd updateongt' cmd */
	bool				bDoNotUpdateOnGT;

	/** Show status / statistics on screen. See 'hmd stats' cmd */
	bool				bShowStats;
#endif

	/** Whether timewarp is enabled or not */
	bool					bTimeWarp;

	/** Player's orientation tracking */
	FQuat					CurHmdOrientation;

	FRotator				DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat					DeltaControlOrientation; // same as DeltaControlRotation but as quat

	FVector					CurHmdPosition;
	FQuat					LastHmdOrientation;

	/** HMD base values, specify forward orientation and zero pos offset */
	OVR::Vector3f			BaseOffset;      // base position, in Oculus coords
	FQuat					BaseOrientation; // base orientation

	ovrHmd					Hmd;
	ovrHmdDesc				HmdDesc;
	ovrEyeRenderDesc		EyeRenderDesc[2];			// 0 - left, 1 - right, same as Views
	ovrMatrix4f				EyeProjectionMatrices[2];	// 0 - left, 1 - right, same as Views
	ovrFovPort				EyeFov[2];					// 0 - left, 1 - right, same as Views
	// U,V scale and offset needed for timewarp.
	ovrVector2f				UVScaleOffset[2][2];		// 0 - left, 1 - right, same as Views
	ovrRecti				EyeRenderViewport[2];		// 0 - left, 1 - right, same as Views
	ovrSizei				TextureSize; // texture size (for both eyes)
	unsigned				SensorHmdCaps;
	unsigned				DistortionCaps;
	unsigned				HmdCaps;

	FIntPoint				EyeViewportSize; // size of the viewport (for one eye). At the moment it is a half of RT.

#ifndef OVR_DIRECT_RENDERING
	struct FDistortionMesh : public OVR::RefCountBase<FDistortionMesh>
	{
		struct FDistortionVertex*	pVertices;
		uint16*						pIndices;
		unsigned					NumVertices;
		unsigned					NumIndices;
		unsigned					NumTriangles;

		FDistortionMesh() :pVertices(NULL), pIndices(NULL), NumVertices(0), NumIndices(0), NumTriangles(0) {}
		~FDistortionMesh() { Clear(); }
		void Clear();
	};
	Ptr<FDistortionMesh>	pDistortionMesh[2];		// 0 - left, 1 - right, same as Views
#else // DIRECT_RENDERING
	FIntPoint				RenderTargetSize; // size of the texture (for both eyes)
	D3D11Bridge				mD3D11Bridge;

#endif // OVR_DIRECT_RENDERING
	
	OVR::Lock				StereoParamsLock;

	// Params accessible from rendering thread. Should be filled at the beginning
	// of the rendering thread under the StereoParamsLock.
	struct FRenderParams
	{
		#ifndef OVR_DIRECT_RENDERING
		Ptr<FDistortionMesh>	pDistortionMesh[2]; // 0 - left, 1 - right, same as Views
		ovrVector2f				UVScale[2];			// 0 - left, 1 - right, same as Views
		ovrVector2f				UVOffset[2];		// 0 - left, 1 - right, same as Views
		FQuat					CurHmdOrientation;
		FVector					CurHmdPosition;
		#else
		D3D11Bridge				mD3D11Bridge;
		ovrPosef				EyeRenderPose[2];
		#endif // OVR_DIRECT_RENDERING
		bool					bFrameBegun;
		FEngineShowFlags		ShowFlags; // a copy of showflags

		FRenderParams(FOculusRiftHMD* plugin);

		void Clear() 
		{ 
			#ifndef OVR_DIRECT_RENDERING
			pDistortionMesh[0] = pDistortionMesh[1] = NULL; 
			#endif
		}
	}						RenderParams_RenderThread;


	/** True, if pos tracking is enabled */
	bool						bHmdPosTracking;
	mutable bool				bHaveVisionTracking;

	/** Optional latency tester */
	OVR::Color					LatencyTestColor;
	OVR::AtomicInt<uint32>		LatencyTestFrameNumber;

	FVector						RCFCorrection;
#ifndef OVR_VISION_ENABLED
	OVR::Vector3f				HeadModel_Meters; // in meters
#endif // OCULUS_USE_VISION
};

DEFINE_LOG_CATEGORY_STATIC(LogHMD, Log, All);

#endif //OCULUS_RIFT_SUPPORTED_PLATFORMS
