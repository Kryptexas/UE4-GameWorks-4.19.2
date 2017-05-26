// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "IStereoLayers.h"
#include "HeadMountedDisplayBase.h"
#include "SceneViewExtension.h"
#include "ScenePrivate.h"
#include "Runtime/UtilityShaders/Public/OculusShaders.h"

class FHeadMountedDisplay;

#if UE_BUILD_SHIPPING
	#define OCULUS_STRESS_TESTS_ENABLED	0
#else
	#if PLATFORM_ANDROID
		#define OCULUS_STRESS_TESTS_ENABLED	0
	#else
		// Disabled since OculusStressTests.cpp is included by both OculusRift and GearVR plugins
		// This causes self-registering console commands to get registers twice, which causes an error.
		#define OCULUS_STRESS_TESTS_ENABLED	0
#endif
#endif	// #if UE_BUILD_SHIPPING


/** Converts vector from meters to UU (Unreal Units) */
FORCEINLINE FVector MetersToUU(const FVector& InVec, float WorldToMetersScale)
{
	return InVec * WorldToMetersScale;
}
/** Converts vector from UU (Unreal Units) to meters */
FORCEINLINE FVector UUToMeters(const FVector& InVec, float WorldToMetersScale)
{
	return InVec * (1.f / WorldToMetersScale);
}

class FHMDSettings : public TSharedFromThis<FHMDSettings, ESPMode::ThreadSafe>
{
public:
	/** Whether or not the Oculus was successfully initialized */
	enum EInitStatus
	{
		eNotInitialized = 0x00,
		eStartupExecuted = 0x01,
		eInitialized = 0x02,
	};

	union
	{
		struct
		{
			uint64 InitStatus : 2; // see bitmask EInitStatus

			/** Whether stereo is currently on or off. */
			uint64 bStereoEnabled : 1;

			/** Whether or not switching to stereo is allowed */
			uint64 bHMDEnabled : 1;

			/** Debugging:  Whether or not the IPD setting have been manually overridden by an exec command. */
			uint64 bOverrideIPD : 1;

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

			/** True, if PlayerController follows HMD orient/pos; false, if this behavior should be disabled. */
			uint64				bPlayerControllerFollowsHmd : 1;

			/** True, if PlayerCameraManager should follow HMD orientation */
			uint64				bPlayerCameraManagerFollowsHmdOrientation : 1;

			/** True, if PlayerCameraManager should follow HMD position */
			uint64				bPlayerCameraManagerFollowsHmdPosition : 1;

			/** Rendering should be (could be) paused */
			uint64				bPauseRendering : 1;

			/** HQ Distortion */
			uint64				bHQDistortion : 1;
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

	/** Interpupillary distance, in meters (user configurable) */
	float InterpupillaryDistance;

	/** User-tunable modification to the interpupillary distance */
	float UserDistanceToScreenModifier;

	/** Optional far clipping plane for projection matrix */
	float NearClippingPlane;

	/** Optional far clipping plane for projection matrix */
	float FarClippingPlane;

	/** HMD base values, specify forward orientation and zero pos offset */
	FVector2D				NeckToEyeInMeters;  // neck-to-eye vector, in meters (X - horizontal, Y - vertical)
	FVector					BaseOffset;			// base position, in meters, relatively to the sensor //@todo hmd: clients need to stop using oculus space
	FQuat					BaseOrientation;	// base orientation

	/** Viewports for each eye, in render target texture coordinates */
	FIntRect				EyeRenderViewport[3];

	/** Maximum adaptive resolution viewports for each eye, in render target texture coordinates */
	FIntRect				EyeMaxRenderViewport[3];

	/** Deprecated position offset */
	FVector					PositionOffset;


	FHMDSettings();
	virtual ~FHMDSettings() {}

	bool IsStereoEnabled() const { return Flags.bStereoEnabled && Flags.bHMDEnabled; }

	virtual void SetEyeRenderViewport(int OneEyeVPw, int OneEyeVPh);
	virtual FIntPoint GetTextureSize() const;
	
	virtual TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> Clone() const;
};

class FHMDGameFrame : public TSharedFromThis<FHMDGameFrame, ESPMode::ThreadSafe>
{
public:
	uint64					FrameNumber; // current frame number.
	TSharedPtr<FHMDSettings, ESPMode::ThreadSafe>	Settings;

	float					MonoCullingDistance;

	FVector					CameraScale3D;

	FRotator				CachedViewRotation[3]; // cached view rotations

	FQuat					LastHmdOrientation; // contains last APPLIED ON GT HMD orientation
	FVector					LastHmdPosition;    // contains last APPLIED ON GT HMD position

	FIntPoint				ViewportSize;		// full final viewport size (window size, backbuffer size)
	FVector2D				WindowSize;			// actual window size

	FQuat					PlayerOrientation;  // orientation of the player (player's torso).
	FVector					PlayerLocation;		// location of the player in the world.

	union
	{
		struct
		{
			/** True, if game thread is finished */
			uint64			bOutOfFrame : 1;

			/** Whether ScreenPercentage usage is enabled */
			uint64			bScreenPercentageEnabled : 1;

			/** True, if vision is acquired at the moment */
			uint64			bHaveVisionTracking : 1;

			/** True, if HMD orientation was applied during the game thread */
			uint64			bOrientationChanged : 1;
			/** True, if HMD position was applied during the game thread */
			uint64			bPositionChanged : 1;
			/** True, if ApplyHmdRotation was used */
			uint64			bPlayerControllerFollowsHmd : 1;
		};
		uint64 Raw;
	} Flags;

	FHMDGameFrame();
	virtual ~FHMDGameFrame() {}

	FHMDSettings* GetSettings()
	{
		return Settings.Get();
	}

	virtual TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> Clone() const;

	float GetWorldToMetersScale() const;
	void SetWorldToMetersScale( const float NewWorldToMetersScale );
	
	/** World units (UU) to Meters scale.  Read from the level, and used to transform positional tracking data */
	float WorldToMetersScaleWhileInFrame;

private:

};

typedef TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> FHMDGameFramePtr;

class FHMDViewExtension : public ISceneViewExtension, public TSharedFromThis<FHMDViewExtension, ESPMode::ThreadSafe>
{
public:
	FHMDViewExtension(FHeadMountedDisplay* InDelegate);
	virtual ~FHMDViewExtension();

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PostRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;


public: // data
	FHeadMountedDisplay* Delegate;
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> RenderFrame;
};

/**
 * A proxy class to work with texture sets in abstract way. This class should be 
 * extended and implemented on per-platform basis.
 */
class FTextureSetProxy : public TSharedFromThis<FTextureSetProxy, ESPMode::ThreadSafe>
{
public:
	FTextureSetProxy():SourceSizeX(0),SourceSizeY(0),SourceNumMips(1),SourceFormat(PF_Unknown) {}
	FTextureSetProxy(uint32 InSrcSizeX, uint32 InSrcSizeY, EPixelFormat InSrcFormat, uint32 InSrcNumMips)
		: SourceSizeX(InSrcSizeX), SourceSizeY(InSrcSizeY), SourceNumMips(InSrcNumMips), SourceFormat(InSrcFormat) {}
	virtual ~FTextureSetProxy() {}

	virtual FTextureRHIRef GetRHITexture() const = 0;
	virtual FTexture2DRHIRef GetRHITexture2D() const = 0;

	uint32 GetSizeX() const
	{ 
		check(IsInRenderingThread());  
		FTexture2DRHIRef Tex = GetRHITexture2D();
		return (Tex.IsValid() ? Tex->GetSizeX() : 0);
	}
	uint32 GetSizeY() const
	{
		check(IsInRenderingThread());
		FTexture2DRHIRef Tex = GetRHITexture2D();
		return (Tex.IsValid() ? Tex->GetSizeY() : 0);
	}

	virtual void ReleaseResources() = 0;
	virtual void SwitchToNextElement() = 0;

	virtual bool Commit(FRHICommandListImmediate& RHICmdList) = 0;
	virtual bool IsStaticImage() const { return false; }

	uint32 GetSourceSizeX() const { return SourceSizeX; }
	uint32 GetSourceSizeY() const { return SourceSizeY; }
	EPixelFormat GetSourceFormat() const { return SourceFormat; }
	uint32 GetSourceNumMips() const { return SourceNumMips; }
protected:
	uint32			SourceSizeX, SourceSizeY, SourceNumMips;
	EPixelFormat	SourceFormat;
};
typedef TSharedPtr<FTextureSetProxy, ESPMode::ThreadSafe>	FTextureSetProxyPtr;

/**
 * Base implementation for a layer descriptor.
 */
class FHMDLayerDesc : public TSharedFromThis<FHMDLayerDesc>
{
	friend class FHMDLayerManager;
public:
	enum ELayerTypeMask : uint32
	{
		Eye			= 0x00000000,
		Quad		= 0x40000000,
		Cylinder	= 0x80000000,
		Cubemap		= 0x90000000,
		Debug		= 0xC0000000,

		TypeMask = (Eye | Quad | Cylinder | Cubemap | Debug),
		IdMask =  ~TypeMask,

		MaxPriority = IdMask
	};

	FHMDLayerDesc(class FHMDLayerManager&, ELayerTypeMask InType, uint32 InPriority, uint32 InID);
	~FHMDLayerDesc() {}

	void SetFlags(uint32 InFlags) { Flags = InFlags; }
	const uint32 GetFlags() const { return Flags; }

	void SetTransform(const FTransform& InTransform);
	const FTransform GetTransform() const { return Transform; }

	void SetQuadSize(const FVector2D& InSize);
	FVector2D GetQuadSize() const { return QuadSize; }

	void SetCylinderSize(const FVector2D& InSize); // X is Radius, Y is length
	FVector2D GetCylinderSize() const { return CylinderSize; }

	void SetCylinderHeight(const float InHeight);
	float GetCylinderHeight() const { return CylinderHeight; }

	void SetTexture(FTextureRHIRef InTexture);
	void SetLeftTexture(FTextureRHIRef InTexture);
	FTextureRHIRef GetTexture() const { return (HasTexture()) ? Texture : nullptr; }
	FTextureRHIRef GetLeftTexture() const { return (HasLeftTexture()) ? LeftTexture : nullptr; }
	bool HasTexture() const { return Texture.IsValid(); }
	bool HasLeftTexture() const { return LeftTexture.IsValid(); }

	void SetTextureSet(const FTextureSetProxyPtr& InTextureSet);
	const FTextureSetProxyPtr& GetTextureSet() const { return TextureSet; }
	bool HasTextureSet() const { return TextureSet.IsValid(); }

	void SetTextureViewport(const FBox2D&);
	FBox2D GetTextureViewport() const { return TextureUV; }

	void SetPriority(uint32);
	uint32 GetPriority() const { return Priority; }

	ELayerTypeMask GetType() const { return ELayerTypeMask(Id & TypeMask); }
	uint32 GetId() const { return Id; }

	void SetHighQuality(bool bHQ = true) { bHighQuality = bHQ; }
	void SetLockedToHead(bool bToHead = true) { bHeadLocked = bToHead; }
	void SetLockedToTorso(bool bToTorso = true) { bTorsoLocked = bToTorso; }
	void SetLockedToWorld() { bHeadLocked = bTorsoLocked = false; }

	bool IsHighQuality() const { return bHighQuality; }
	bool IsHeadLocked() const { return bHeadLocked; }
	bool IsTorsoLocked() const { return bTorsoLocked; }
	bool IsWorldLocked() const { return !bHeadLocked && !bTorsoLocked; }
	bool IsNewLayer() const { return bNewLayer; }

	FHMDLayerDesc& operator=(const FHMDLayerDesc&);

	bool IsTextureChanged() const { return bTextureHasChanged; }
	void MarkTextureChanged() const { bTextureHasChanged = true; }
	bool IsTransformChanged() const { return bTransformHasChanged; }
	bool IsPokeAHole() const { return (Flags & IStereoLayers::LAYER_FLAG_SUPPORT_DEPTH) != 0 && !IsHeadLocked(); }
	void ResetChangedFlags();

protected:
	class FHMDLayerManager& LayerManager;
	uint32			Id;		// ELayerTypeMask | Id
	mutable FTextureRHIRef Texture;// Source texture (for quads) (mutable for GC)
	mutable FTextureRHIRef LeftTexture;
	FTextureSetProxyPtr TextureSet;	// TextureSet (for eye buffers)
	uint32			Flags;
	FBox2D			TextureUV;
	FTransform		Transform; // layer world or HMD transform (Rotation, Translation, Scale), see bHeadLocked.
	FVector2D		QuadSize;  // size of the quad in UU
	FVector2D	    CylinderSize;
	float			CylinderHeight;
	uint32			Priority;  // priority of the layer (Z-axis); the higher value, the later layer is drawn
	bool			bHighQuality : 1; // high quality flag
	bool			bHeadLocked  : 1; // the layer is head-locked; Transform becomes relative to HMD
	bool			bTorsoLocked : 1; // locked to torso (to player's orientation / location)
	mutable bool	bTextureHasChanged : 1;
	bool			bTransformHasChanged : 1;
	bool			bNewLayer : 1;
	bool			bAlreadyAdded : 1; // internal flag indicating the layer is already added into render layers.
};

/**
 * A base class that stores layer info for rendering. Could be extended
 */
class FHMDRenderLayer : public TSharedFromThis<FHMDRenderLayer>
{
public:
	explicit FHMDRenderLayer(FHMDLayerDesc&);
	virtual ~FHMDRenderLayer() {}

	virtual TSharedPtr<FHMDRenderLayer> Clone() const;

	virtual void ReleaseResources();

	// This method checks if the layer is completely setup, and if it is not it will be excluded from rendering. May be called on RenderThread.
	virtual bool IsFullySetup() const { return LayerInfo.GetType() != FHMDLayerDesc::Eye || TextureSet.IsValid(); }

	const FHMDLayerDesc& GetLayerDesc() const { return LayerInfo; }
	void SetLayerDesc(const FHMDLayerDesc& InDesc) { LayerInfo = InDesc; }

	void TransferTextureSet(FHMDLayerDesc& InDesc)
	{
		TextureSet = InDesc.GetTextureSet();
		InDesc.SetTextureSet(nullptr);
		bOwnsTextureSet = true;
	}

	bool CommitTextureSet(FRHICommandListImmediate& RHICmdList)
	{
		if (TextureSet.IsValid())
		{
			return TextureSet->Commit(RHICmdList);
		}
		return false;
	}
	void ResetChangedFlags() { LayerInfo.ResetChangedFlags(); }

protected:
	FHMDLayerDesc		LayerInfo;
	FTextureSetProxyPtr	TextureSet;
	bool				bOwnsTextureSet; // indicate that the TextureSet is owned by this instance; otherwise, should be false
};

/**
 * Base implementation for a layer manager.
 */
class FHMDLayerManager : public TSharedFromThis<FHMDLayerManager>
{
	friend class FHeadMountedDisplay;
public:
	FHMDLayerManager();
	virtual ~FHMDLayerManager();

	virtual void Startup();
	virtual void Shutdown();

	enum LayerOriginType
	{
		Layer_UnknownOrigin = 0,
		Layer_WorldLocked = 1,
		Layer_HeadLocked = 2,
		Layer_TorsoLocked = 3,
	};

	// Adds layer, returns the layer and its' ID.
	virtual TSharedPtr<FHMDLayerDesc> AddLayer(FHMDLayerDesc::ELayerTypeMask InType, uint32 InPriority, LayerOriginType InLayerOriginType, uint32& OutLayerId);

	virtual void RemoveLayer(uint32 LayerId);

	virtual const FHMDLayerDesc* GetLayerDesc(uint32 LayerId) const;
	virtual void UpdateLayer(const FHMDLayerDesc&);

	// Marks RenderLayers 'dirty': those to be updated.
	virtual void SetDirty() { bLayersChanged = true; }

	// Releases all textureSets resources used by all layers. Shouldn't touch anything else.
	virtual void ReleaseTextureSets_RenderThread_NoLock();

	void PokeAHole(FRHICommandListImmediate& RHICmdList, const FHMDGameFrame* CurrentFrame, IRendererModule* RendererModule, FSceneViewFamily& InViewFamily);

	virtual void GetPokeAHoleMatrices(const FViewInfo *LeftView, const FViewInfo *RightView, const FHMDLayerDesc& LayerDesc, const FHMDGameFrame* CurrentFrame, FMatrix &LeftMatrix, FMatrix &RightMatrix, bool &InvertCoordinates) { }

	void RemoveAllLayers();

	void ReleaseRenderLayers_RenderThread()
	{
		FScopeLock ScopeLock(&LayersLock);
		ReleaseTextureSets_RenderThread_NoLock();
		LayersToRender.Reset(0);
		bLayersChanged = true;
	}

	FHMDRenderLayer* GetRenderLayer_RenderThread_NoLock(uint32 LayerId);
	const FHMDRenderLayer* GetRenderLayer_RenderThread_NoLock(uint32 LayerId) const
	{
		return const_cast<FHMDLayerManager*>(this)->GetRenderLayer_RenderThread_NoLock(LayerId);
	}

	uint32 GetTotalNumberOfLayers() const;

protected:
	// Creates a layer. Could be overridden by inherited class to create custom layers. Called on a RenderThread
	virtual TSharedPtr<FHMDRenderLayer> CreateRenderLayer_RenderThread(FHMDLayerDesc&);

	virtual TSharedPtr<FHMDLayerDesc> FindLayer_NoLock(uint32 LayerId) const;

	virtual bool ShouldSupportLayerType(FHMDLayerDesc::ELayerTypeMask InType) { return true; }

	// Should be called before SubmitFrame is called.
	// Updates sizes, distances, orientations, textures of each layer, as needed.
	virtual void PreSubmitUpdate_RenderThread(FRHICommandListImmediate& RHICmdList, const FHMDGameFrame* CurrentFrame, bool ShowFlagsRendering);

	virtual uint32 GetTotalNumberOfLayersSupported() const = 0;

	static uint32 FindLayerIndex(const TArray<TSharedPtr<FHMDLayerDesc> >& Layers, uint32 LayerId);
	const TArray<TSharedPtr<FHMDLayerDesc> >& GetLayersArrayById(uint32 LayerId) const;
	TArray<TSharedPtr<FHMDLayerDesc> >& GetLayersArrayById(uint32 LayerId);

	void ReleaseTextureSetsInArray_RenderThread_NoLock(TArray<TSharedPtr<FHMDLayerDesc> >& Layers);
protected:
	int32 CurrentId;

	// a map of layers. int32 is ID of the layer. Used on game thread.
	TArray<TSharedPtr<FHMDLayerDesc> > EyeLayers;
	TArray<TSharedPtr<FHMDLayerDesc> > QuadLayers;
	TArray<TSharedPtr<FHMDLayerDesc> > DebugLayers;

	mutable FCriticalSection LayersLock;
	// Ordered list of layers 
	TArray<TSharedPtr<FHMDRenderLayer> > LayersToRender;
	volatile bool bLayersChanged; // set when LayersToRender should be re-done (under LayersLock)
};

//-------------------------------------------------------------------------------------------------
// FHMDCommonConsoleCommands - Wrapper around various console command handlers used by FHeadMountedDisplay
//-------------------------------------------------------------------------------------------------
class FHMDCommonConsoleCommands : private FSelfRegisteringExec
{
public:
	FHMDCommonConsoleCommands(class FHeadMountedDisplay* InHMDPtr);
private:
	FAutoConsoleCommand UpdateOnRenderThreadCommand;
#if !UE_BUILD_SHIPPING
	// Debug console commands
	FAutoConsoleCommand UpdateOnGameThreadCommand;
	FAutoConsoleCommand PositionOffsetCommand;
	FAutoConsoleCommand EnforceHeadTrackingCommand;
#endif // !UE_BUILD_SHIPPING
	FAutoConsoleCommand ShowSettingsCommand;
	FAutoConsoleCommand ResetSettingsCommand;
	FAutoConsoleCommand IPDCommand;
	FAutoConsoleCommand FCPCommand;
	FAutoConsoleCommand NCPCommand;

	// Exec handler that aliases old deprecated Oculus console commands to the new ones.
	bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
};

/**
 * HMD device interface
 */
class FHeadMountedDisplay : public FHeadMountedDisplayBase, public IStereoLayers
{
	friend class FHMDCommonConsoleCommands;
public:
	FHeadMountedDisplay();
	~FHeadMountedDisplay();

	/** @return	True if the HMD was initialized OK */
	virtual bool IsInitialized() const;

	/**
	 * Get the ISceneViewExtension for this HMD, or none.
	 */
	virtual TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe> GetViewExtension() override;

	/**
	 * Get the FStereoRendering for this HMD, or none.
	 */
	virtual class FStereoRendering* GetStereoRendering()
	{
		return nullptr;
	}

	/**
	 * This method is called when new game frame begins (called on a game thread).
	 */
	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;

	/**
	 * This method is called when game frame ends (called on a game thread).
	 */
	virtual bool OnEndGameFrame(FWorldContext& WorldContext) override;

	virtual void SetupViewFamily(class FSceneViewFamily& InViewFamily) {}
	virtual void SetupView(class FSceneViewFamily& InViewFamily, FSceneView& InView) {}

	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> PassRenderFrameOwnership();

	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool allow = true) override;

	virtual void GetCurrentOrientationAndPosition(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	virtual bool DoesSupportPositionalTracking() const override;
	virtual bool HasValidTrackingPosition() override;
	virtual void RebaseObjectOrientationAndPosition(FVector& OutPosition, FQuat& OutOrientation) const override;

	virtual bool IsHeadTrackingAllowed() const override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;

	virtual bool IsChromaAbCorrectionEnabled() const override;

	virtual bool IsPositionalTrackingEnabled() const override;

	virtual bool EnableStereo(bool stereo = true) override;
	virtual bool IsStereoEnabled() const override;
	virtual bool IsStereoEnabledOnNextFrame() const override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;

	virtual void SetClippingPlanes(float NCP, float FCP) override;

	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;

	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	// Returns true, if HMD is currently active
	virtual bool IsHMDActive() { return IsHMDConnected(); }

	/**
	* A helper function that calculates the estimated neck position using the specified orientation and position
	* (for example, reported by GetCurrentOrientationAndPosition()).
	*
	* @param CurrentOrientation	(in) The device's current rotation
	* @param CurrentPosition		(in) The device's current position, in its own tracking space
	* @param PositionScale			(in) The 3D scale that will be applied to position.
	* @return			The estimated neck position, calculated using NeckToEye vector from User Profile. Same coordinate space as CurrentPosition.
	*/
	virtual FVector GetNeckPosition(const FQuat& CurrentOrientation, const FVector& CurrentPosition, const FVector& PositionScale);

	/**
	* Sets base position offset (in meters). The base position offset is the distance from the physical (0, 0, 0) position
	* to current HMD position (bringing the (0, 0, 0) point to the current HMD position)
	* Note, this vector is set by ResetPosition call; use this method with care.
	* The axis of the vector are the same as in Unreal: X - forward, Y - right, Z - up.
	*
	* @param BaseOffset			(in) the vector to be set as base offset, in meters.
	*/
	virtual void SetBaseOffsetInMeters(const FVector& BaseOffset);

	/**
	* Returns the currently used base position offset, previously set by the
	* ResetPosition or SetBasePositionOffset calls. It represents a vector that translates the HMD's position
	* into (0,0,0) point, in meters.
	* The axis of the vector are the same as in Unreal: X - forward, Y - right, Z - up.
	*
	* @return Base position offset, in meters.
	*/
	virtual FVector GetBaseOffsetInMeters() const;

	/* Raw sensor data structure. */
	struct SensorData
	{
		FVector AngularAcceleration; // Angular acceleration in radians per second per second.
		FVector LinearAcceleration;  // Acceleration in meters per second per second.
		FVector AngularVelocity;     // Angular velocity in radians per second.
		FVector LinearVelocity;      // Velocity in meters per second.
		double TimeInSeconds;		 // Time when the reported IMU reading took place, in seconds.
	};

	/**
	* Reports raw sensor data. If HMD doesn't support any of the parameters then it should be set to zero.
	*
	* @param OutData	(out) SensorData structure to be filled in.
	*/
	virtual void GetRawSensorData(SensorData& OutData) 
	{
		FMemory::Memzero(OutData);
	}

	/**
	* User profile structure.
	*/
	struct UserProfile
	{
		FString Name;
		FString Gender;
		float PlayerHeight;				// Height of the player, in meters
		float EyeHeight;				// Height of the player's eyes, in meters
		float IPD;						// Interpupillary distance, in meters
		FVector2D NeckToEyeDistance;	// Neck-to-eye distance, X - horizontal, Y - vertical, in meters
		TMap<FString, FString> ExtraFields; // extra fields in name / value pairs.
	};
	virtual bool GetUserProfile(UserProfile& OutProfile) { return false;  }

	virtual void ApplyHmdRotation(APlayerController* PC, FRotator& ViewRotation) override;
	virtual bool UpdatePlayerCamera(FQuat& CurrentOrientation, FVector& CurrentPosition) override;

	// An improved version of GetCurrentOrientationAndPostion, used from blueprints by OculusLibrary.
	virtual void GetCurrentHMDPose(FQuat& CurrentOrientation, FVector& CurrentPosition,
		bool bUseOrienationForPlayerCamera, bool bUsePositionForPlayerCamera, const FVector& PositionScale = FVector::ZeroVector);

	virtual FHMDLayerManager* GetLayerManager() { return nullptr; }

	virtual IStereoLayers* GetStereoLayers() override { return this; }

	//** IStereoLayers implementation
	virtual uint32 CreateLayer(const IStereoLayers::FLayerDesc& InLayerDesc) override;
	virtual void DestroyLayer(uint32 LayerId) override;
	virtual void SetLayerDesc(uint32 LayerId, const IStereoLayers::FLayerDesc& InLayerDesc) override;
	virtual bool GetLayerDesc(uint32 LayerId, IStereoLayers::FLayerDesc& OutLayerDesc) override;
	virtual void MarkTextureForUpdate(uint32 LayerId) override;
	virtual void UpdateSplashScreen() override;

	virtual class FAsyncLoadingSplash* GetAsyncLoadingSplash() const { return nullptr; }

	virtual void SetPixelDensity(float NewPD) {}

	uint32 GetCurrentFrameNumber() const { return CurrentFrameNumber.GetValue(); }

	virtual IRendererModule* GetRendererModule() { return NULL; }

protected:
	virtual TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> CreateNewGameFrame() const = 0;
	virtual TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> CreateNewSettings() const = 0;
	void CreateAndInitNewGameFrame(const class AWorldSettings* WorldSettings);

	virtual bool DoEnableStereo(bool bStereo) = 0;
	virtual void GetCurrentPose(FQuat& CurrentHmdOrientation, FVector& CurrentHmdPosition, bool bUseOrienationForPlayerCamera = false, bool bUsePositionForPlayerCamera = false) = 0;

	virtual void ResetStereoRenderingParams();

	virtual FHMDGameFrame* GetCurrentFrame() const;

	/**
	 * Reads the device configuration, and sets up the stereoscopic rendering parameters
	 */
	virtual void UpdateStereoRenderingParams() {}
	virtual void UpdateHmdRenderInfo() {}
	virtual void UpdateDistortionCaps() {}
	virtual void UpdateHmdCaps() {}
	/** Applies overrides on system when switched to stereo (such as VSync and ScreenPercentage) */
	virtual void ApplySystemOverridesOnStereo(bool force = false) {}

	virtual void ResetControlRotation() const;

protected:
	FHMDCommonConsoleCommands CommonConsoleCommands;
	virtual void UpdateOnRenderThreadCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
#if !UE_BUILD_SHIPPING
	virtual void UpdateOnGameThreadCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	virtual void PositionOffsetCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	virtual void EnforceHeadTrackingCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
#endif
	virtual void ShowSettingsCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	// The reset command calls ResetStereoRenderingParams directly
	virtual void IPDCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	virtual void FCPCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);
	virtual void NCPCommandHandler(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar);

protected:
	TSharedPtr<FHMDSettings, ESPMode::ThreadSafe> Settings;
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> Frame;
	TSharedPtr<FHMDGameFrame, ESPMode::ThreadSafe> RenderFrame;

	FRotator			DeltaControlRotation;  // used from ApplyHmdRotation
	
	FThreadSafeCounter	CurrentFrameNumber;

	union
	{
		struct
		{
			/** True, if game frame has started */
			uint64	bFrameStarted : 1;

			/** Indicates if it is necessary to update stereo rendering params */
			uint64	bNeedUpdateStereoRenderingParams : 1;
			uint64	bApplySystemOverridesOnStereo : 1;

			uint64	bNeedEnableStereo : 1;
			uint64	bNeedDisableStereo : 1;

			uint64  bNeedUpdateDistortionCaps : 1;

			/** True, if vision was acquired at previous frame */
			uint64	bHadVisionTracking : 1;
		};
		uint64 Raw;
	} Flags;


	FHMDGameFrame* GetGameFrame()
	{
		return Frame.Get();
	}

	FHMDGameFrame* GetRenderFrame()
	{
		return RenderFrame.Get();
	}

	const FHMDGameFrame* GetGameFrame() const
	{
		return Frame.Get();
	}

	const FHMDGameFrame* GetRenderFrame() const
	{
		return RenderFrame.Get();
	}


#if !UE_BUILD_SHIPPING
	TWeakObjectPtr<class AStaticMeshActor> SeaOfCubesActorPtr;
	FVector							 CachedViewLocation;
	FString							 CubeMeshName, CubeMaterialName;
	float							 SideOfSingleCubeInMeters;
	float							 SeaOfCubesVolumeSizeInMeters;
	int								 NumberOfCubesInOneSide;
	FVector							 CenterOffsetInMeters; // offset from the center of 'sea of cubes'
#endif

public:
	FHMDSettings* GetSettings()
	{
		return Settings.Get();
	}

	const FHMDSettings* GetSettings() const
	{
		return Settings.Get();
	}

	static void QuantizeBufferSize(int32& InOutBufferSizeX, int32& InOutBufferSizeY, uint32 DividableBy = 32);

};
