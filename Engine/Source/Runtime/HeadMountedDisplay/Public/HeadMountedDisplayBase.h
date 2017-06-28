// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IHeadMountedDisplay.h"
#include "DefaultSpectatorScreenController.h"

/**
 * Default implementation for various IHeadMountedDisplay methods.
 * You can extend this class instead of IHeadMountedDisplay directly when implementing support for new HMD devices.
 */

class HEADMOUNTEDDISPLAY_API FHeadMountedDisplayBase : public IHeadMountedDisplay
{

public:
	virtual ~FHeadMountedDisplayBase() {}

	/**
	 * Record analytics - To add custom information logged with the analytics, override PopulateAnalyticsAttributes
	 */
	virtual void RecordAnalytics() override;


	/** 
	 * Default stereo layer implementation
	 */
	virtual IStereoLayers* GetStereoLayers() override;
	virtual void GatherViewExtensions(TArray<TSharedPtr<class ISceneViewExtension, ESPMode::ThreadSafe>>& OutViewExtensions) override;

	/** Overridden so the late update HMD transform can be passed on to the default stereo layers implementation */
	virtual void ApplyLateUpdate(FSceneInterface* Scene, const FTransform& OldRelativeTransform, const FTransform& NewRelativeTransform) override;

	virtual bool IsSpectatorScreenActive() const override;

	virtual class ISpectatorScreenController* GetSpectatorScreenController() override;
	virtual class ISpectatorScreenController const* GetSpectatorScreenController() const override;

	// Spectator Screen Hooks into specific implementations
	virtual FIntRect GetFullFlatEyeRect(FTexture2DRHIRef EyeTexture) const { return FIntRect(0, 0, 1, 1); }
	virtual void CopyTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef SrcTexture, FIntRect SrcRect, FTexture2DRHIParamRef DstTexture, FIntRect DstRect, bool bClearBlack) const {}

protected:
	/**
	 * Called by RecordAnalytics when creating the analytics event sent during HMD initialization.
	 *
	 * Return false to disable recording the analytics event	
	 */
	virtual bool PopulateAnalyticsAttributes(TArray<struct FAnalyticsEventAttribute>& EventAttributes);

	/** 
	 * Implement this method to provide an alternate render target for head locked stereo layer rendering, when using the default Stereo Layers implementation.
	 * 
	 * Return a FTexture2DRHIRef pointing to a texture that can be composed on top of each eye without applying reprojection to it.
	 * Return nullptr to render head locked stereo layers into the same render target as other layer types, in which case InOutViewport must not be modified.
	 */
	virtual FTexture2DRHIRef GetOverlayLayerTarget_RenderThread(EStereoscopicPass StereoPass, FIntRect& InOutViewport) { return nullptr; }

	/**
	 * Implement this method to override the render target for scene based stereo layers.
	 * Return nullptr to render stereo layers into the normal render target passed to the stereo layers scene view extension, in which case OutViewport must not be modified.
	 */
	virtual FTexture2DRHIRef GetSceneLayerTarget_RenderThread(EStereoscopicPass StereoPass, FIntRect& InOutViewport) { return nullptr; }

	mutable TSharedPtr<class FDefaultStereoLayers, ESPMode::ThreadSafe> DefaultStereoLayers;
	
	friend class FDefaultStereoLayers;

	TUniquePtr<FDefaultSpectatorScreenController> SpectatorScreenController;
};
