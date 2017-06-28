// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OculusHMDPrivate.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusHMD_GameFrame.h"
#include "OculusHMD_Layer.h"
#include "TickableObjectRenderThread.h"

namespace OculusHMD
{

class FOculusHMD;


//-------------------------------------------------------------------------------------------------
// FSplashDesc
//-------------------------------------------------------------------------------------------------

struct FSplashDesc
{
	UTexture2D*			LoadingTexture;					// a UTexture pointer, either loaded manually or passed externally.
	FString				TexturePath;					// a path to a texture for auto loading, can be empty if LoadingTexture is specified explicitly
	FTransform			TransformInMeters;				// transform of center of quad (meters)
	FVector2D			QuadSizeInMeters;				// dimensions in meters
	FQuat				DeltaRotation;					// a delta rotation that will be added each rendering frame (half rate of full vsync)
	FTextureRHIRef		LoadedTexture;					// texture reference for when a TexturePath or UTexture is not available
	FVector2D			TextureOffset;					// texture offset amount from the top left corner
	FVector2D			TextureScale;					// texture scale
	bool				bNoAlphaChannel;				// whether the splash layer uses it's alpha channel

	FSplashDesc() : LoadingTexture(nullptr)
		, TransformInMeters(FVector(4.0f, 0.f, 0.f))
		, QuadSizeInMeters(3.f, 3.f)
		, DeltaRotation(FQuat::Identity)
		, LoadedTexture(nullptr)
		, TextureOffset(0.0f, 0.0f)
		, TextureScale(1.0f, 1.0f)
		, bNoAlphaChannel(false)
	{
	}

	bool operator==(const FSplashDesc& d) const
	{
		return LoadingTexture == d.LoadingTexture && TexturePath == d.TexturePath && LoadedTexture == d.LoadedTexture &&
			TransformInMeters.Equals(d.TransformInMeters) &&
			QuadSizeInMeters == d.QuadSizeInMeters && DeltaRotation.Equals(d.DeltaRotation) &&
			TextureOffset == d.TextureOffset && TextureScale == d.TextureScale;
	}
};


//-------------------------------------------------------------------------------------------------
// FSplashLayer
//-------------------------------------------------------------------------------------------------

struct FSplashLayer
{
	FSplashDesc Desc;
	FLayerPtr Layer;

public:
	FSplashLayer(const FSplashDesc& InDesc) : Desc(InDesc) {}
	FSplashLayer(const FSplashLayer& InSplashLayer) : Desc(InSplashLayer.Desc), Layer(InSplashLayer.Layer) {}
};


//-------------------------------------------------------------------------------------------------
// FSplash
//-------------------------------------------------------------------------------------------------

class FSplash : public TSharedFromThis<FSplash>, public FGCObject
{
protected:
	class FTicker : public FTickableObjectRenderThread, public TSharedFromThis<FTicker>
	{
	public:
		FTicker(FSplash* InSplash) : FTickableObjectRenderThread(false, true), pSplash(InSplash) {}

		virtual void Tick(float DeltaTime) override { pSplash->Tick_RenderThread(DeltaTime); }
		virtual TStatId GetStatId() const override  { RETURN_QUICK_DECLARE_CYCLE_STAT(FSplash, STATGROUP_Tickables); }
		virtual bool IsTickable() const override	{ return pSplash->IsTickable(); }
	protected:
		FSplash* pSplash;
	};

public:
	FSplash(FOculusHMD* InPlugin);
	virtual ~FSplash();

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FGCObject interface

	void Tick_RenderThread(float DeltaTime);
	bool IsTickable() const { return bTickable; }
	bool IsShown() const;

	void Startup();
	void PreShutdown();
	void Shutdown();

	bool IsLoadingStarted() const { return bLoadingStarted; }
	bool IsLoadingCompleted() const { return bLoadingCompleted; }

	void OnLoadingBegins();
	void OnLoadingEnds();

	bool AddSplash(const FSplashDesc&);
	void ClearSplashes();
	bool GetSplash(unsigned index, FSplashDesc& OutDesc);

	void SetAutoShow(bool bInAuto) { bAutoShow = bInAuto; }
	bool IsAutoShow() const { return bAutoShow; }

	void SetLoadingIconMode(bool bInLoadingIconMode) { bLoadingIconMode = bInLoadingIconMode; }
	bool IsLoadingIconMode() const { return bLoadingIconMode; }

	enum EShowFlags
	{
		ShowManually = (1 << 0),
		ShowAutomatically = (1 << 1),
	};

	void Show(uint32 InShowFlags = ShowManually);
	void Hide(uint32 InShowFlags = ShowManually);

	// delegate method, called when loading begins
	void OnPreLoadMap(const FString&) { OnLoadingBegins(); }

	// delegate method, called when loading ends
	void OnPostLoadMap(UWorld*) { OnLoadingEnds(); }


protected:
	void OnShow();
	void OnHide();
	bool PushFrame();
	void UnloadTextures();
	void LoadTexture(FSplashLayer& InSplashLayer);
	void UnloadTexture(FSplashLayer& InSplashLayer);

	void RenderFrame_RenderThread(FRHICommandListImmediate& RHICmdList, double TimeInSeconds);

protected:
	FOculusHMD* OculusHMD;
	FCustomPresent* CustomPresent;
	TSharedPtr<FTicker> Ticker;
	FCriticalSection RenderThreadLock;
	FSettingsPtr Settings;
	FGameFramePtr Frame;
	TArray<FSplashLayer> SplashLayers;
	uint32 NextLayerId;
	FLayerPtr BlackLayer;
	TArray<FLayerPtr> Layers_RenderThread;
	TArray<FLayerPtr> Layers_RHIThread;

	// All these flags are only modified from the Game thread
	bool bInitialized;
	bool bTickable;
	bool bLoadingStarted;
	bool bLoadingCompleted;
	bool bLoadingIconMode; // this splash screen is a simple loading icon (if supported)
	bool bAutoShow; // whether or not show splash screen automatically (when LoadMap is called)
	bool bIsBlack;

	float SystemDisplayInterval;
	uint32 ShowFlags;
	double LastTimeInSeconds;
};

typedef TSharedPtr<FSplash> FSplashPtr;


} // namespace OculusHMD

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS