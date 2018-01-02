// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ISteamVRPlugin.h"

#if STEAMVR_SUPPORTED_PLATFORMS

#include "HeadMountedDisplay.h"
#include "HeadMountedDisplayBase.h"
#include "SteamVRFunctionLibrary.h"
#include "SteamVRSplash.h"
#include "IStereoLayers.h"
#include "StereoLayerManager.h"
#include "XRRenderTargetManager.h"

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
class FSteamVRHMD : public FHeadMountedDisplayBase, public FXRRenderTargetManager, public FSteamVRAssetManager, public TSharedFromThis<FSteamVRHMD, ESPMode::ThreadSafe>, public TStereoLayerManager<FSteamVRLayer>, public IHeadMountedDisplayVulkanExtensions
{
public:
	static const FName SteamSystemName;

	/** IXRTrackingSystem interface */
	virtual FName GetSystemName() const override
	{
		return SteamSystemName;
	}
	virtual FString GetVersionString() const override;


	virtual class IHeadMountedDisplay* GetHMDDevice() override
	{
		return this;
	}

	virtual class TSharedPtr< class IStereoRendering, ESPMode::ThreadSafe > GetStereoRenderingDevice() override
	{
		return AsShared();
	}

	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;
	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() override;
	virtual bool EnumerateTrackedDevices(TArray<int32>& TrackedIds, EXRTrackedDeviceType DeviceType = EXRTrackedDeviceType::Any) override;

	virtual bool GetTrackingSensorProperties(int32 InDeviceId, FQuat& OutOrientation, FVector& OutOrigin, FXRSensorProperties& OutSensorProperties) override;
	virtual bool GetCurrentPose(int32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual bool GetRelativeEyePose(int32 DeviceId, EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition) override;
	virtual bool IsTracking(int32 DeviceId) override;

	virtual void ResetOrientationAndPosition(float yaw = 0.f) override;
	virtual void ResetOrientation(float Yaw = 0.f) override;
	virtual void ResetPosition() override;

	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;
	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	virtual void OnEndPlay(FWorldContext& InWorldContext) override;
	virtual void RecordAnalytics() override;
public:
	/** IHeadMountedDisplay interface */

	virtual bool IsHMDConnected() override;
	virtual bool IsHMDEnabled() const override;
	virtual EHMDWornState::Type GetHMDWornState() override;
	virtual void EnableHMD(bool allow = true) override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;

	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;

	virtual bool IsChromaAbCorrectionEnabled() const override;

	virtual bool HasHiddenAreaMesh() const override { return HiddenAreaMeshes[0].IsValid() && HiddenAreaMeshes[1].IsValid(); }
	virtual void DrawHiddenAreaMesh_RenderThread(FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	virtual bool HasVisibleAreaMesh() const override { return VisibleAreaMeshes[0].IsValid() && VisibleAreaMeshes[1].IsValid(); }
	virtual void DrawVisibleAreaMesh_RenderThread(FRHICommandList& RHICmdList, EStereoscopicPass StereoPass) const override;

	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

	virtual void UpdateScreenSettings(const FViewport* InViewport) override {}

	virtual void SetTrackingOrigin(EHMDTrackingOrigin::Type NewOrigin) override;
	virtual EHMDTrackingOrigin::Type GetTrackingOrigin() override;
	
	virtual bool AllocateRenderTargetTexture(uint32 Index, uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 InTexFlags, uint32 InTargetableTextureFlags, FTexture2DRHIRef& OutTargetableTexture, FTexture2DRHIRef& OutShaderResourceTexture, uint32 NumSamples = 1) override;

	virtual bool GetHMDDistortionEnabled(EShadingPath ShadingPath) const override;

	virtual void OnBeginRendering_GameThread() override;
	virtual void OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) override;

	virtual float GetPixelDenity() const override { return PixelDensity; }
	virtual void SetPixelDensity(const float NewDensity) override { PixelDensity = NewDensity; }
	virtual FIntPoint GetIdealRenderTargetSize() const override { return IdealRenderTargetSize; }

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual void CalculateStereoViewOffset(const EStereoscopicPass StereoPassType, FRotator& ViewRotation, const float MetersToWorld, FVector& ViewLocation) override;
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override;
	virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef BackBuffer, FTexture2DRHIParamRef SrcTexture, FVector2D WindowSize) const override;
	virtual void GetOrthoProjection(int32 RTWidth, int32 RTHeight, float OrthoDistance, FMatrix OrthoProjection[2]) const override;
	virtual void GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;
	virtual IStereoRenderTargetManager* GetRenderTargetManager() override { return this; }
	virtual IStereoLayers* GetStereoLayers() override;

	/** FXRRenderTargetManager interface */
	virtual void UpdateViewportRHIBridge(bool bUseSeparateRenderTarget, const class FViewport& Viewport, FRHIViewport* const ViewportRHI) override;
	virtual bool ShouldUseSeparateRenderTarget() const override
	{
		check(IsInGameThread());
		return IsStereoEnabled();
	}
	virtual void CalculateRenderTargetSize(const class FViewport& Viewport, uint32& InOutSizeX, uint32& InOutSizeY) override;
	virtual bool NeedReAllocateViewportRenderTarget(const class FViewport& Viewport) override;

	// IStereoLayers interface
	// Create/Set/Get/Destroy inherited from TStereoLayerManager
	virtual void UpdateSplashScreen() override;

	/** IHeadMountedDisplayVulkanExtensions */
	virtual bool GetVulkanInstanceExtensionsRequired( TArray<const ANSICHAR*>& Out ) override;
	virtual bool GetVulkanDeviceExtensionsRequired( VkPhysicalDevice_T *pPhysicalDevice, TArray<const ANSICHAR*>& Out ) override;

	// SpectatorScreen
private:
	void CreateSpectatorScreenController();
public:
	virtual FIntRect GetFullFlatEyeRect_RenderThread(FTexture2DRHIRef EyeTexture) const override;
	virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SrcTexture, FIntRect SrcRect, FTexture2DRHIParamRef DstTexture, FIntRect DstRect, bool bClearBlack) const override;

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
		virtual void FinishRendering() = 0;
		virtual void UpdateViewport(const FViewport& Viewport, FRHIViewport* InViewportRHI) = 0;
		virtual void SetNeedReinitRendererAPI() { bNeedReinitRendererAPI = true; }
        virtual bool NeedsNativePresent() override;

		virtual void Reset() = 0;
		virtual void Shutdown() = 0;
		
	protected:
		FSteamVRHMD*			Plugin;
		bool					bNeedReinitRendererAPI;
		bool					bInitialized;
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
	EXRTrackedDeviceType GetTrackedDeviceType(int32 DeviceId) const;
	STEAMVR_API ETrackingStatus GetControllerTrackingStatus(int32 DeviceId) const;

	/** Chaperone */
	/** Returns whether or not the player is currently inside the bounds */
	bool IsInsideBounds();

	/** Returns an array of the bounds as Unreal-scaled vectors, relative to the HMD calibration point (0,0,0).  The Z will always be at 0.f */
	TArray<FVector> GetBounds() const;
	
	void PoseToOrientationAndPosition(const vr::HmdMatrix34_t& InPose, const float WorldToMetersScale, FQuat& OutOrientation, FVector& OutPosition) const;
public:
	/** Constructor */
	FSteamVRHMD(ISteamVRPlugin* SteamVRPlugin);

	/** Destructor */
	virtual ~FSteamVRHMD();

	/** @return	True if the API was initialized OK */
	bool IsInitialized() const;

	vr::IVRSystem* GetVRSystem() const { return VRSystem; }
	vr::IVRRenderModels* GetRenderModelManager() const { return VRRenderModels; }

protected:

	virtual float GetWorldToMetersScale() const override;


private:

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

	// Current world to meters scale. Should only be used when refreshing poses.
	// Everywhere else, use the current tracking frame's WorldToMetersScale.
	float GameWorldToMetersScale;
 	
 	struct FTrackingFrame
 	{
 		uint32 FrameNumber;

 		bool bDeviceIsConnected[vr::k_unMaxTrackedDeviceCount];
 		bool bPoseIsValid[vr::k_unMaxTrackedDeviceCount];
 		FVector DevicePosition[vr::k_unMaxTrackedDeviceCount];
 		FQuat DeviceOrientation[vr::k_unMaxTrackedDeviceCount];
		bool bHaveVisionTracking;

		/** World units (UU) to Meters scale.  Read from the level, and used to transform positional tracking data */
		float WorldToMetersScale;

		vr::HmdMatrix34_t RawPoses[vr::k_unMaxTrackedDeviceCount];

		FTrackingFrame()
			: FrameNumber(0)
			, bHaveVisionTracking(false)
			, WorldToMetersScale(100.0f)
		{
			const uint32 MaxDevices = vr::k_unMaxTrackedDeviceCount;

			FMemory::Memzero(bDeviceIsConnected, MaxDevices * sizeof(bool));
			FMemory::Memzero(bPoseIsValid, MaxDevices * sizeof(bool));
			FMemory::Memzero(DevicePosition, MaxDevices * sizeof(FVector));


			for (uint32 i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
			{
				DeviceOrientation[i] = FQuat::Identity;
			}

			FMemory::Memzero(RawPoses, MaxDevices * sizeof(vr::HmdMatrix34_t));
		}
 	};

	void UpdatePoses();

	void ConvertRawPoses(FSteamVRHMD::FTrackingFrame& TrackingFrame) const;

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

	FIntPoint IdealRenderTargetSize;
	float PixelDensity;

	/** How far the HMD has to move before it's considered to be worn */
	float HMDWornMovementThreshold;

	/** used to check how much the HMD has moved for changing the Worn status */
	FVector					HMDStartLocation;

	// HMD base values, specify forward orientation and zero pos offset
	FQuat					BaseOrientation;	// base orientation
	FVector					BaseOffset;

	// State for tracking quit operation
	bool					bIsQuitting;
	double					QuitTimestamp;

	/**  True if the HMD sends an event that the HMD is being interacted with */
	bool					bShouldCheckHMDPosition;

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
