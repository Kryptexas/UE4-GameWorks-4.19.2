// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ISteamVRPlugin.h"
#include "HeadMountedDisplay.h"
#include "HeadMountedDisplayBase.h"
#include "SteamVRFunctionLibrary.h"
#include "SteamVRSplash.h"
#include "IStereoLayers.h"
#include "StereoLayerManager.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#include <d3d11.h>
#include "HideWindowsPlatformTypes.h"
#elif PLATFORM_LINUX
#include "OpenGLDrv.h"
#endif

#if STEAMVR_SUPPORTED_PLATFORMS

#include "SceneViewExtension.h"

#define STEAMVR_USE_VULKAN_RHI 0

class IRendererModule;

/** Stores vectors, in clockwise order, to define soft and hard bounds for Chaperone */
struct FBoundingQuad
{
	FVector Corners[4];
};

//@todo steamvr: remove GetProcAddress() workaround once we have updated to Steamworks 1.33 or higher
typedef bool(VR_CALLTYPE *pVRIsHmdPresent)();
typedef void*(VR_CALLTYPE *pVRGetGenericInterface)(const char* pchInterfaceVersion, vr::HmdError* peError);


/** 
 * Translates deprecated SteamVR HMD console commands to the newer ones
 */
class FSteamVRHMDCompat : private FSelfRegisteringExec
{
private:
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
};

/** 
 * Struct for managing stereo layer data.
 */
struct FSteamVRLayer
{
	typedef IStereoLayers::FLayerDesc FLayerDesc;
	FLayerDesc	          LayerDesc;
	vr::VROverlayHandle_t OverlayHandle;
	bool				  bUpdateTexture;

	FSteamVRLayer(const FLayerDesc& InLayerDesc)
		: LayerDesc(InLayerDesc)
		, OverlayHandle(vr::k_ulOverlayHandleInvalid)
		, bUpdateTexture(false)
	{}

	// Required by TStereoLayerManager:
	friend bool GetLayerDescMember(const FSteamVRLayer& Layer, FLayerDesc& OutLayerDesc);
	friend void SetLayerDescMember(FSteamVRLayer& Layer, const FLayerDesc& InLayerDesc);
	friend void MarkLayerTextureForUpdate(FSteamVRLayer& Layer);
};

/**
 * SteamVR Head Mounted Display
 */
class FSteamVRHMD : public FHeadMountedDisplayBase, public ISceneViewExtension, public TSharedFromThis<FSteamVRHMD, ESPMode::ThreadSafe>, public TStereoLayerManager<FSteamVRLayer>
{
public:
	/** IHeadMountedDisplay interface */
	virtual FName GetDeviceName() const override
	{
		static FName DefaultName(TEXT("SteamVR"));
		return DefaultName;
	}

	virtual bool OnStartGameFrame( FWorldContext& WorldContext ) override;

	virtual bool IsHMDConnected() override { return true; }
	virtual bool IsHMDEnabled() const override;
	virtual EHMDWornState::Type GetHMDWornState() override;
	virtual void EnableHMD(bool allow = true) override;
	virtual EHMDDeviceType::Type GetHMDDeviceType() const override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;

	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;

	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() override;
	virtual uint32 GetNumOfTrackingSensors() const override;
	virtual bool GetTrackingSensorProperties(uint8 InSensorIndex, FVector& OutOrigin, FQuat& OutOrientation, float& OutLeftFOV, float& OutRightFOV, float& OutTopFOV, float& OutBottomFOV, float& OutCameraDistance, float& OutNearPlane, float& OutFarPlane) const override;
	virtual void RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;

	virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual class TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;
	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	virtual bool IsChromaAbCorrectionEnabled() const override;

	virtual bool IsPositionalTrackingEnabled() const override { return true; }

	virtual bool IsHeadTrackingAllowed() const override;

	virtual void ResetOrientationAndPosition(float yaw = 0.f) override;
	virtual void ResetOrientation(float Yaw = 0.f) override;
	virtual void ResetPosition() override;

	virtual void SetClippingPlanes(float NCP, float FCP) override;

	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;

	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	virtual bool HasHiddenAreaMesh() const override { return HiddenAreaMeshes[0].IsValid() && HiddenAreaMeshes[1].IsValid(); }
	virtual void DrawHiddenAreaMesh_RenderThread(FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	virtual bool HasVisibleAreaMesh() const override { return VisibleAreaMeshes[0].IsValid() && VisibleAreaMeshes[1].IsValid(); }
	virtual void DrawVisibleAreaMesh_RenderThread(FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

	virtual void UpdateScreenSettings(const FViewport* InViewport) override {}

	virtual void OnEndPlay(FWorldContext& InWorldContext) override;

	virtual FString GetVersionString() const override;

	virtual void SetTrackingOrigin(EHMDTrackingOrigin::Type NewOrigin) override;
	virtual EHMDTrackingOrigin::Type GetTrackingOrigin() override;

	virtual void RecordAnalytics() override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float MetersToWorld, FVector& ViewLocation) override;
	virtual FMatrix GetStereoProjectionMatrix(const EStereoscopicPass StereoPassType, const float FOV) const override;
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) override;
	virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef BackBuffer, FTexture2DRHIParamRef SrcTexture) const override;
	virtual void GetOrthoProjection(int32 RTWidth, int32 RTHeight, float OrthoDistance, FMatrix OrthoProjection[2]) const override;
	virtual void GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override;
	virtual bool NeedReAllocateViewportRenderTarget(const FViewport& Viewport) override;
	virtual bool ShouldUseSeparateRenderTarget() const override
	{
		check(IsInGameThread());
		return IsStereoEnabled();
	}
	virtual void UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& Viewport, SViewport*) override;
	virtual IStereoLayers* GetStereoLayers () override;

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PostInitViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PostInitView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual bool UsePostInitView() const override;

	// IStereoLayers interface
	// Create/Set/Get/Destroy inherited from TStereoLayerManager
	virtual void UpdateSplashScreen() override;

	class BridgeBaseImpl : public FRHICustomPresent
	{
	public:
		BridgeBaseImpl(FSteamVRHMD* plugin) :
			FRHICustomPresent(nullptr),
			Plugin(plugin),
			bNeedReinitRendererAPI(true),
			bInitialized(false)
		{}

		bool IsInitialized() const { return bInitialized; }

		virtual void BeginRendering() = 0;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) = 0;
		virtual void SetNeedReinitRendererAPI() { bNeedReinitRendererAPI = true; }

		virtual void Reset() = 0;
		virtual void Shutdown() = 0;

	protected:
		FSteamVRHMD*		Plugin;
		bool				bNeedReinitRendererAPI;
		bool				bInitialized;
	};

#if PLATFORM_WINDOWS
	class D3D11Bridge : public BridgeBaseImpl
	{
	public:
		D3D11Bridge(FSteamVRHMD* plugin);

		virtual void OnBackBufferResize() override;
		virtual bool Present(int& SyncInterval) override;
		virtual void PostPresent() override;

		virtual void BeginRendering() override;
		void FinishRendering();
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:
		ID3D11Texture2D* RenderTargetTexture = NULL;
	};
#elif PLATFORM_LINUX

#if STEAMVR_USE_VULKAN_RHI
	class VulkanBridge : public BridgeBaseImpl
	{
	public:
		VulkanBridge(FSteamVRHMD* plugin);

		virtual void OnBackBufferResize() override;
		virtual bool Present(int& SyncInterval) override;
		virtual void PostPresent() override;

		virtual void BeginRendering() override;
		void FinishRendering();
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:

		FTexture2DRHIRef RenderTargetTexture;
	};

	#else
	class OpenGLBridge : public BridgeBaseImpl
	{
	public:
		OpenGLBridge(FSteamVRHMD* plugin);

		virtual void OnBackBufferResize() override;
		virtual bool Present(int& SyncInterval) override;
		virtual void PostPresent() override;

		virtual void BeginRendering() override;
		void FinishRendering();
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:
		GLuint RenderTargetTexture = 0;

	};
	#endif
#endif // PLATFORM_WINDOWS

	BridgeBaseImpl* GetActiveRHIBridgeImpl();
	void ShutdownRendering();

	/** Motion Controllers */
	ESteamVRTrackedDeviceType GetTrackedDeviceType(uint32 DeviceId) const;
	void GetTrackedDeviceIds(ESteamVRTrackedDeviceType DeviceType, TArray<int32>& TrackedIds) const;
	bool GetTrackedObjectOrientationAndPosition(uint32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition);
	ETrackingStatus GetControllerTrackingStatus(uint32 DeviceId) const;
	STEAMVR_API bool GetControllerHandPositionAndOrientation( const int32 ControllerIndex, EControllerHand Hand, FVector& OutPosition, FQuat& OutOrientation);
	STEAMVR_API ETrackingStatus GetControllerTrackingStatus(int32 ControllerIndex, EControllerHand DeviceHand) const;


	/** Chaperone */
	/** Returns whether or not the player is currently inside the bounds */
	bool IsInsideBounds();

	/** Returns an array of the bounds as Unreal-scaled vectors, relative to the HMD calibration point (0,0,0).  The Z will always be at 0.f */
	TArray<FVector> GetBounds() const;

	/** Sets the map from Unreal controller id and hand index, to tracked device id. */
	void SetUnrealControllerIdAndHandToDeviceIdMap(int32 InUnrealControllerIdAndHandToDeviceIdMap[ MAX_STEAMVR_CONTROLLER_PAIRS ][ 2 ]);

public:
	/** Constructor */
	FSteamVRHMD(ISteamVRPlugin* SteamVRPlugin);

	/** Destructor */
	virtual ~FSteamVRHMD();

	/** @return	True if the API was initialized OK */
	bool IsInitialized() const;

	vr::IVRSystem* GetVRSystem() const { return VRSystem; }

private:

	enum class EPoseRefreshMode
	{
		None,
		GameRefresh,
		RenderRefresh
	};

	/**
	 * Starts up the OpenVR API. Returns true if initialization was successful, false if not.
	 */
	bool Startup();

	/**
	 * Shuts down the OpenVR API
	 */
	void Shutdown();

	void LoadFromIni();

	bool LoadOpenVRModule();
	void UnloadOpenVRModule();

	void PoseToOrientationAndPosition(const vr::HmdMatrix34_t& Pose, const float WorldToMetersScale, FQuat& OutOrientation, FVector& OutPosition) const;
	void GetCurrentPose(FQuat& CurrentOrientation, FVector& CurrentPosition, uint32 DeviceID = vr::k_unTrackedDeviceIndex_Hmd, EPoseRefreshMode RefreshMode=EPoseRefreshMode::None, float ForceRefreshWorldToMetersScale = 0.0f);
	float GetWorldToMetersScale() const;

	void GetWindowBounds(int32* X, int32* Y, uint32* Width, uint32* Height);

public:
	static FORCEINLINE FMatrix ToFMatrix(const vr::HmdMatrix34_t& tm)
	{
		// Rows and columns are swapped between vr::HmdMatrix34_t and FMatrix
		return FMatrix(
			FPlane(tm.m[0][0], tm.m[1][0], tm.m[2][0], 0.0f),
			FPlane(tm.m[0][1], tm.m[1][1], tm.m[2][1], 0.0f),
			FPlane(tm.m[0][2], tm.m[1][2], tm.m[2][2], 0.0f),
			FPlane(tm.m[0][3], tm.m[1][3], tm.m[2][3], 1.0f));
	}

	static FORCEINLINE FMatrix ToFMatrix(const vr::HmdMatrix44_t& tm)
	{
		// Rows and columns are swapped between vr::HmdMatrix44_t and FMatrix
		return FMatrix(
			FPlane(tm.m[0][0], tm.m[1][0], tm.m[2][0], tm.m[3][0]),
			FPlane(tm.m[0][1], tm.m[1][1], tm.m[2][1], tm.m[3][1]),
			FPlane(tm.m[0][2], tm.m[1][2], tm.m[2][2], tm.m[3][2]),
			FPlane(tm.m[0][3], tm.m[1][3], tm.m[2][3], tm.m[3][3]));
	}

	static FORCEINLINE vr::HmdMatrix34_t ToHmdMatrix34(const FMatrix& tm)
	{
		// Rows and columns are swapped between vr::HmdMatrix34_t and FMatrix
		vr::HmdMatrix34_t out;

		out.m[0][0] = tm.M[0][0];
		out.m[1][0] = tm.M[0][1];
		out.m[2][0] = tm.M[0][2];

		out.m[0][1] = tm.M[1][0];
		out.m[1][1] = tm.M[1][1];
		out.m[2][1] = tm.M[1][2];

		out.m[0][2] = tm.M[2][0];
		out.m[1][2] = tm.M[2][1];
		out.m[2][2] = tm.M[2][2];

		out.m[0][3] = tm.M[3][0];
		out.m[1][3] = tm.M[3][1];
		out.m[2][3] = tm.M[3][2];

		return out;
	}
private:

	void SetupOcclusionMeshes();

	bool bHmdEnabled;
	EHMDWornState::Type HmdWornState;
	bool bStereoDesired;
	bool bStereoEnabled;
	mutable bool bHaveVisionTracking;

 	struct FTrackingFrame
 	{
 		uint32 FrameNumber;

 		bool bDeviceIsConnected[vr::k_unMaxTrackedDeviceCount];
 		bool bPoseIsValid[vr::k_unMaxTrackedDeviceCount];
 		FVector DevicePosition[vr::k_unMaxTrackedDeviceCount];
 		FQuat DeviceOrientation[vr::k_unMaxTrackedDeviceCount];

		/** World units (UU) to Meters scale.  Read from the level, and used to transform positional tracking data */
		float WorldToMetersScale;

		vr::HmdMatrix34_t RawPoses[vr::k_unMaxTrackedDeviceCount];

		FTrackingFrame()
		{
			FrameNumber = 0;

			const uint32 MaxDevices = vr::k_unMaxTrackedDeviceCount;

			FMemory::Memzero(bDeviceIsConnected, MaxDevices * sizeof(bool));
			FMemory::Memzero(bPoseIsValid, MaxDevices * sizeof(bool));
			FMemory::Memzero(DevicePosition, MaxDevices * sizeof(FVector));

			WorldToMetersScale = 100.0f;

			for (uint32 i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
			{
				DeviceOrientation[i] = FQuat::Identity;
			}

			FMemory::Memzero(RawPoses, MaxDevices * sizeof(vr::HmdMatrix34_t));
		}
 	};
	FTrackingFrame GameTrackingFrame;
	FTrackingFrame RenderTrackingFrame;

	const FTrackingFrame& GetTrackingFrame() const;

	/** Coverts a SteamVR-space vector to an Unreal-space vector.  Does not handle scaling, only axes conversion */
	FORCEINLINE static FVector CONVERT_STEAMVECTOR_TO_FVECTOR(const vr::HmdVector3_t InVector)
	{
		return FVector(-InVector.v[2], InVector.v[0], InVector.v[1]);
	}

	FORCEINLINE static FVector RAW_STEAMVECTOR_TO_FVECTOR(const vr::HmdVector3_t InVector)
	{
		return FVector(InVector.v[0], InVector.v[1], InVector.v[2]);
	}

	FHMDViewMesh HiddenAreaMeshes[2];
	FHMDViewMesh VisibleAreaMeshes[2];

	/** Chaperone Support */
	struct FChaperoneBounds
	{
		/** Stores the bounds in SteamVR HMD space, for fast checking.  These will need to be converted to Unreal HMD-calibrated space before being used in the world */
		FBoundingQuad			Bounds;

	public:
		FChaperoneBounds()
		{}

		FChaperoneBounds(vr::IVRChaperone* Chaperone)
			: FChaperoneBounds()
		{
			vr::HmdQuad_t VRBounds;
			Chaperone->GetPlayAreaRect(&VRBounds);
			for (uint8 i = 0; i < 4; ++i)
			{
				const vr::HmdVector3_t Corner = VRBounds.vCorners[i];
				Bounds.Corners[i] = RAW_STEAMVECTOR_TO_FVECTOR(Corner);
			}
		}
	};
	FChaperoneBounds ChaperoneBounds;
	
	virtual void UpdateLayer(struct FSteamVRLayer& Layer, uint32 LayerId, bool bIsValid) const override;

	void UpdateLayerTextures();

	TSharedPtr<FSteamSplashTicker>	SplashTicker;
	
	uint32 WindowMirrorBoundsWidth;
	uint32 WindowMirrorBoundsHeight;
	/** How far the HMD has to move before it's considered to be worn */
	float HMDWornMovementThreshold;

	/** Player's orientation tracking */
	mutable FQuat			CurHmdOrientation;

	FRotator				DeltaControlRotation;    // same as DeltaControlOrientation but as rotator
	FQuat					DeltaControlOrientation; // same as DeltaControlRotation but as quat

	mutable FVector			CurHmdPosition;

	/** used to check how much the HMD has moved for changing the Worn status */
	FVector					HMDStartLocation;

	mutable FQuat			LastHmdOrientation; // contains last APPLIED ON GT HMD orientation
	FVector					LastHmdPosition;	// contains last APPLIED ON GT HMD position

	// HMD base values, specify forward orientation and zero pos offset
	FQuat					BaseOrientation;	// base orientation
	FVector					BaseOffset;

	// State for tracking quit operation
	bool					bIsQuitting;
	double					QuitTimestamp;

	/**  True if the HMD sends an event that the HMD is being interacted with */
	bool					bShouldCheckHMDPosition;

	/** Mapping from Unreal Controller Id and Hand to a tracked device id.  Passed in from the controller plugin */
	int32 UnrealControllerIdAndHandToDeviceIdMap[MAX_STEAMVR_CONTROLLER_PAIRS][2];

	IRendererModule* RendererModule;
	ISteamVRPlugin* SteamVRPlugin;

	vr::IVRSystem* VRSystem;
	vr::IVRCompositor* VRCompositor;
	vr::IVROverlay* VROverlay;
	vr::IVRChaperone* VRChaperone;

	FString DisplayId;

	FSteamVRHMDCompat CompatExec;

#if PLATFORM_WINDOWS
	TRefCountPtr<D3D11Bridge>	pBridge;
#elif PLATFORM_LINUX
	#if STEAMVR_USE_VULKAN_RHI
		TRefCountPtr<VulkanBridge>	pBridge;
	#else
		TRefCountPtr<OpenGLBridge>	pBridge;
	#endif
#endif


//@todo steamvr: Remove GetProcAddress() workaround once we have updated to Steamworks 1.33 or higher
public:
	static pVRIsHmdPresent VRIsHmdPresentFn;
	static pVRGetGenericInterface VRGetGenericInterfaceFn;

	friend class FSteamSplashTicker;
};


#endif //STEAMVR_SUPPORTED_PLATFORMS
