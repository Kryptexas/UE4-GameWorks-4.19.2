// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ISteamVRPlugin.h"

#if STEAMVR_SUPPORTED_PLATFORMS

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
#elif PLATFORM_MAC
#include <IOSurface/IOSurface.h>
#endif

#if !PLATFORM_MAC // No OpenGL on Mac anymore
#include "OpenGLDrv.h"
#endif

#include "SceneViewExtension.h"
#include "SteamVRAssetManager.h"

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
 * Render target swap chain
 */
class FRHITextureSet2D : public FRHITexture2D
{
public:
	
	FRHITextureSet2D(const uint32 TextureSetSize, EPixelFormat Format, uint32 SizeX, uint32 SizeY, uint32 NumMips, uint32 NumSamples, uint32 Flags, const FClearValueBinding& InClearValue)
	: FRHITexture2D(SizeX, SizeY, NumMips, NumSamples, Format, Flags, InClearValue)
	, TextureIndex(0)
	{
		TextureSet.AddZeroed(TextureSetSize);
	}
	
	virtual ~FRHITextureSet2D()
	{}
	
	void AddTexture(FTexture2DRHIRef& Texture, const uint32 Index)
	{
		check(Index < static_cast<uint32>(TextureSet.Num()));
		// todo: Check texture format to ensure it matches the set
		TextureSet[Index] = Texture;
	}
	
	void Advance()
	{
		TextureIndex = (TextureIndex + 1) % static_cast<uint32>(TextureSet.Num());
	}
	
	virtual void* GetTextureBaseRHI() override
	{
		check(TextureSet[TextureIndex].IsValid());
		return TextureSet[TextureIndex]->GetTextureBaseRHI();
	}
	
	virtual void* GetNativeResource() const override
	{
		check(TextureSet[TextureIndex].IsValid());
		return TextureSet[TextureIndex]->GetNativeResource();
	}
	
private:
	TArray<FTexture2DRHIRef> TextureSet;
	uint32 TextureIndex;
};

/**
 * SteamVR Head Mounted Display
 */
class FSteamVRHMD : public FHeadMountedDisplayBase, public ISceneViewExtension, public FSteamVRAssetManager, public TSharedFromThis<FSteamVRHMD, ESPMode::ThreadSafe>, public TStereoLayerManager<FSteamVRLayer>
{
public:
	/** IHeadMountedDisplay interface */
	virtual FName GetDeviceName() const override
	{
		static FName DefaultName(TEXT("SteamVR"));
		return DefaultName;
	}

	virtual bool OnStartGameFrame( FWorldContext& WorldContext ) override;

	virtual bool IsHMDConnected() override;
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
	
#if PLATFORM_MAC
	virtual bool AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 InTexFlags, uint32 InTargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) override;
#endif

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
	virtual void DrawDebug(UCanvas* Canvas) override;
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

	/** FrameSettings struct contains information about the current render target frame, which is used to coordinate adaptive pixel density between the main and render threads */
	struct FFrameSettings
	{
		/** Whether or not we need to update this next frame */
		bool bNeedsUpdate;

		/** Whether or not adaptive pixel density is enabled */
		bool bAdaptivePixelDensity;

		/** Used for disabling TAA when changing pixel densities, because of incorrect texture look ups in the history buffer.  If other than INDEX_NONE, the r.PostProcessingAAQuality cvar will be set to this value next frame */
		int32 PostProcessAARestoreValue;

		/** Current pixel density, which should match the current setting of r.ScreenPercentage for the frame */
		float CurrentPixelDensity;
		
		/** Min and max bounds for pixel density, which can change from frame to frame potentially */
		float PixelDensityMin;
		float PixelDensityMax;

		/** How many frames to remain locked for, in order to limit adapting too frequently */
		int32 PixelDensityAdaptiveLockedFrames;

		/** The recommended (i.e. PD = 1.0) render target size requested by the device */
		uint32 RecommendedWidth, RecommendedHeight;

		/** The sub-rect of the render target representing the view for each eye, given the CurrentPixelDensity (0 = left, 1 = right) */
		FIntRect EyeViewports[2];
		/** The sub-rect of the render target representing the view for each eye at PixelDensityMax, which is the upper bounds (0 = left, 1 = right) */
		FIntRect MaxViewports[2];

		/** The current render target size.  This should only change on initialization, when adaptive is turned on and off, or when PixelDensityMax changes */
		FIntPoint RenderTargetSize;

		FFrameSettings()
			: bNeedsUpdate(false)
			, bAdaptivePixelDensity(false)
			, PostProcessAARestoreValue(INDEX_NONE)
			, CurrentPixelDensity(1.f)
   			, PixelDensityMin(0.7f)
			, PixelDensityMax(1.f)
			, PixelDensityAdaptiveLockedFrames(0)
			, RecommendedWidth(0)
			, RecommendedHeight(0)
		{
		}
	};
	
	// SpectatorScreen
private:
	void CreateSpectatorScreenController();
public:
	virtual FIntRect GetFullFlatEyeRect(FTexture2DRHIRef EyeTexture) const override;
	virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SrcTexture, FIntRect SrcRect, FTexture2DRHIParamRef DstTexture, FIntRect DstRect, bool bClearBlack) const override;

	class BridgeBaseImpl : public FRHICustomPresent
	{
	public:
		BridgeBaseImpl(FSteamVRHMD* plugin) :
			FRHICustomPresent(nullptr),
			Plugin(plugin),
			bNeedReinitRendererAPI(true),
			bInitialized(false),
			FrameNumber(0),
			LastPresentedFrameNumber(-1)
		{}

		bool IsInitialized() const { return bInitialized; }

		virtual void BeginRendering() = 0;
		virtual void FinishRendering() = 0;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) = 0;
		virtual void SetNeedReinitRendererAPI() { bNeedReinitRendererAPI = true; }

		virtual void Reset() = 0;
		virtual void Shutdown() = 0;

		/** Copies the current FrameSettings for the HMD to the bridge for use on the render thread */
		void UpdateFrameSettings(struct FSteamVRHMD::FFrameSettings& NewSettings);
		FFrameSettings GetFrameSettings(int32 NumBufferedFrames = 0);
		
		void IncrementFrameNumber()
		{
			FScopeLock Lock(&FrameNumberLock);
			++FrameNumber;
		}
		int32 GetFrameNumber() const
		{
			FScopeLock Lock(&FrameNumberLock);
			return FrameNumber;
		}
		bool IsOnLastPresentedFrame() const
		{
			FScopeLock Lock(&FrameNumberLock);
			return (LastPresentedFrameNumber == FrameNumber);
		}
		
	protected:
		FSteamVRHMD*			Plugin;
		bool					bNeedReinitRendererAPI;
		bool					bInitialized;
		int32					FrameNumber;
		int32					LastPresentedFrameNumber;
		mutable FCriticalSection	FrameNumberLock;
		TArray<FFrameSettings>		FrameSettingsStack;
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
		virtual void FinishRendering() override;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:
		ID3D11Texture2D* RenderTargetTexture = NULL;
	};
#endif // PLATFORM_WINDOWS

#if !PLATFORM_MAC
	class VulkanBridge : public BridgeBaseImpl
	{
	public:
		VulkanBridge(FSteamVRHMD* plugin);

		virtual void OnBackBufferResize() override;
		virtual bool Present(int& SyncInterval) override;
		virtual void PostPresent() override;

		virtual void BeginRendering() override;
		virtual void FinishRendering() override;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:

		FTexture2DRHIRef RenderTargetTexture;
	};

	class OpenGLBridge : public BridgeBaseImpl
	{
	public:
		OpenGLBridge(FSteamVRHMD* plugin);

		virtual void OnBackBufferResize() override;
		virtual bool Present(int& SyncInterval) override;
		virtual void PostPresent() override;

		virtual void BeginRendering() override;
		virtual void FinishRendering() override;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) override;
		virtual void Reset() override;
		virtual void Shutdown() override
		{
			Reset();
		}

	protected:
		GLuint RenderTargetTexture = 0;

	};
	
#elif PLATFORM_MAC

	class MetalBridge : public BridgeBaseImpl
	{
	public:
		MetalBridge(FSteamVRHMD* plugin);
		
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
		
		IOSurfaceRef GetSurface(const uint32 SizeX, const uint32 SizeY);
		
		FTexture2DRHIRef TextureSet;
	};
#endif // PLATFORM_MAC

	BridgeBaseImpl* GetActiveRHIBridgeImpl();
	void ShutdownRendering();

	/** Motion Controllers */
	ESteamVRTrackedDeviceType GetTrackedDeviceType(uint32 DeviceId) const;
	void GetTrackedDeviceIds(ESteamVRTrackedDeviceType DeviceType, TArray<int32>& TrackedIds) const;
	bool GetTrackedObjectOrientationAndPosition(uint32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition);
	ETrackingStatus GetControllerTrackingStatus(uint32 DeviceId) const;
	STEAMVR_API bool GetControllerHandPositionAndOrientation( const int32 ControllerIndex, EControllerHand Hand, FVector& OutPosition, FQuat& OutOrientation);
	STEAMVR_API ETrackingStatus GetControllerTrackingStatus(int32 ControllerIndex, EControllerHand DeviceHand) const;
	bool IsTracking(uint32 DeviceId) const;

	/** Chaperone */
	/** Returns whether or not the player is currently inside the bounds */
	bool IsInsideBounds();

	/** Returns an array of the bounds as Unreal-scaled vectors, relative to the HMD calibration point (0,0,0).  The Z will always be at 0.f */
	TArray<FVector> GetBounds() const;

	/** Sets the map from Unreal controller id and hand index, to tracked device id. */
	void SetUnrealControllerIdAndHandToDeviceIdMap(int32 InUnrealControllerIdAndHandToDeviceIdMap[ MAX_STEAMVR_CONTROLLER_PAIRS ][ vr::k_unMaxTrackedDeviceCount ]);
	
public:
	/** Constructor */
	FSteamVRHMD(ISteamVRPlugin* SteamVRPlugin);

	/** Destructor */
	virtual ~FSteamVRHMD();

	/** @return	True if the API was initialized OK */
	bool IsInitialized() const;

	vr::IVRSystem* GetVRSystem() const { return VRSystem; }
	vr::IVRRenderModels* GetRenderModelManager() const { return VRRenderModels; }

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

	/** Command handler for turning on and off adaptive pixel density */
	FAutoConsoleCommand CUseAdaptivePD;
	void AdaptivePixelDensityCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);

	static void ConsoleSinkHandler();
	static FAutoConsoleVariableSink ConsoleVariableSink;

	/** Returns the current pixel density, which is the ratio of the current r.ScreenPercentage to the IdealScreenPercentage provided by the device */
	float GetPixelDensity() const;
	
	/** Sets the current pixel density, which is the ratio of r.ScreenPercentage to the IdealScreenPercentage provided by the device */
	void SetPixelDensity(float NewPD);
	
	/** Determines what the current Pixel Density should be, given the performance of the GPU under its current load.  Ruturns how many buckets should be jumped up or down given the frame history */
	int32 CalculateScalabilityFactor();

	/** Array of floats representing pixel density values to jump to, based on performance.  The list index will be adjusted up and down based on CalculateScalabilityFactor's rules */	
	TArray<float>			AdaptivePixelDensityBuckets;
	
	/** The current AdaptivePixelDensityBucket index that we're at */
	int32					CurrentAdaptiveBucket;
	
	/** An array of buffered frame times, so that we can see how performance is trending with adaptive pixel density adjustments */
	TArray<float>			PreviousFrameTimes;

	const int32				PreviousFrameBufferSize = 4;
	
	/** Index of the current frame timing data in PreviousFrameTimes that we're on */
	int32					CurrentFrameTimesBufferIndex;

	/** Contains the settings for the current frame, including render target size and subrect viewports, given the current PixelDensity */
	FFrameSettings FrameSettings;
	mutable FCriticalSection FrameSettingsLock;

	/** Updates FrameSettings based on the current adaptive pixel density requirements */
	void UpdateStereoRenderingParams();

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

	void UpdateStereoLayers_RenderThread();

	TSharedPtr<FSteamSplashTicker>	SplashTicker;

	uint32 WindowMirrorBoundsWidth;
	uint32 WindowMirrorBoundsHeight;

	/** The screen percentage requested by the current headset that would give a perceived pixel density of 1.0 */
	float IdealScreenPercentage;

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
	int32 UnrealControllerIdAndHandToDeviceIdMap[MAX_STEAMVR_CONTROLLER_PAIRS][vr::k_unMaxTrackedDeviceCount];

	IRendererModule* RendererModule;
	ISteamVRPlugin* SteamVRPlugin;

	vr::IVRSystem* VRSystem;
	vr::IVRCompositor* VRCompositor;
	vr::IVROverlay* VROverlay;
	vr::IVRChaperone* VRChaperone;
	vr::IVRRenderModels* VRRenderModels;

	FString DisplayId;

	FQuat PlayerOrientation;
	FVector PlayerLocation;

	TRefCountPtr<BridgeBaseImpl> pBridge;

//@todo steamvr: Remove GetProcAddress() workaround once we have updated to Steamworks 1.33 or higher
public:
	static pVRIsHmdPresent VRIsHmdPresentFn;
	static pVRGetGenericInterface VRGetGenericInterfaceFn;

	friend class FSteamSplashTicker;
};


#endif //STEAMVR_SUPPORTED_PLATFORMS
