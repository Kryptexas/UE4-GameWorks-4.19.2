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
	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, const FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation) const = 0;

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
};
