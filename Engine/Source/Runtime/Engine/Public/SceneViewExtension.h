// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneViewExtension.h: Allow changing the view parameters on the render thread
=============================================================================*/

#pragma once

class ISceneViewExtension
{
public:
    /**
     * Called on game thread when creating the view family.
     */
    virtual void ModifyShowFlags(FEngineShowFlags& ShowFlags) = 0;

    /**
     * Called on game thread when creating the view.
     */
    virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) = 0;

    /**
     * Called on render thread at the start of rendering.
	 * FrameNumber is a copy of SceneRenderer.FrameNumber (that is equal to already incremented GFrameNumber).
     */
    virtual void PreRenderViewFamily_RenderThread(FSceneViewFamily& InViewFamily, uint32 InFrameNumber) = 0;

	/**
     * Called on render thread at the start of rendering, for each view, after PreRenderViewFamily_RenderThread call.
     */
    virtual void PreRenderView_RenderThread(FSceneView& InView) = 0;
};
