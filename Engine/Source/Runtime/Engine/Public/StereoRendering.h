// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StereoRendering.h: Abstract stereoscopic rendering interface
=============================================================================*/

#pragma once

class IStereoRendering
{
public:
	/** 
	 * Whether or not stereo rendering is on.
	 */
	virtual bool IsStereoEnabled() const = 0;

	/** 
	 * Switches stereo rendering on / off. Returns current state of stereo.
	 */
	virtual bool EnableStereo(bool stereo = true) = 0;

    /**
     * Adjusts the viewport rectangle for stereo, based on which eye pass is being rendered.
     */
    virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const = 0;

    /**
	 * Calculates the offset for the camera position, given the specified position, rotation, and world scale
	 */
	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) = 0;

	/**
	 * Gets a projection matrix for the device, given the specified eye setup
	 */
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType, const float FOV) const = 0;

	/**
	 * Sets view-specific params (such as view projection matrix) for the canvas.
	 */
	virtual void InitCanvasFromView(FSceneView* InView, UCanvas* Canvas) = 0;

	/**
	 * Pushes transformations based on specified viewport into canvas. Necessary to call PopTransform on
	 * FCanvas object afterwards.
	 */
	virtual void PushViewportCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FViewport *InViewport) const = 0;

	/**
	 * Pushes transformations based on specified view into canvas. Necessary to call PopTransform on FCanvas 
	 * object afterwards.
	 */
	virtual void PushViewCanvas(EStereoscopicPass StereoPass, FCanvas *InCanvas, UCanvas *InCanvasObject, FSceneView *InView) const = 0;

	/**
	 * Returns eye render params, used from PostProcessHMD, RenderThread.
	 */
	virtual void GetEyeRenderParams_RenderThread(EStereoscopicPass StereoPass, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const {}

	/**
	 * Returns timewarp matrices, used from PostProcessHMD, RenderThread.
	 */
	virtual void GetTimewarpMatrices_RenderThread(EStereoscopicPass StereoPass, FMatrix& EyeRotationStart, FMatrix& EyeRotationEnd) const {}

	// Optional methods to support rendering into a texture.
	/**
	 * Updates viewport for direct rendering of distortion. Should be called on a game thread.
	 */
	virtual void UpdateViewport(bool bUseSeparateRenderTarget, const FViewport& Viewport) {}

	/**
	 * Calculates dimensions of the render target texture for direct rendering of distortion.
	 */
	virtual void CalculateRenderTargetSize(uint32& InOutSizeX, uint32& InOutSizeY) const {}

	/**
	 * Returns true, if render target texture must be re-calculated. 
	 */
	virtual bool NeedReAllocateViewportRenderTarget(const FViewport& Viewport) const { return false; }

	// Whether separate render target should be used or not.
	virtual bool ShouldUseSeparateRenderTarget() const { return false; }

	// Renders texture into a backbuffer. Could be empty if no rendertarget texture is used, or if direct-rendering 
	// through RHI bridge is implemented. 
	virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTexture2DRHIParamRef BackBuffer, FTexture2DRHIParamRef SrcTexture) const {}

	/**
	 * Called after Present is called.
	 */
	virtual void FinishRenderingFrame_RenderThread() {}
};
