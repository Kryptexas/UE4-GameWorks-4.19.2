// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Canvas.h"
#include "CanvasRenderTarget2D.generated.h"

/** This delegate allows developers to bind a function to be called every time the texture is updated, so that they can use the supplied canvas to draw to the texture. */
DECLARE_DELEGATE_OneParam( FOnCanvasRenderTargetUpdate, class UCanvas* );

/**
 * CanvasRenderTarget2D is 2D render target which exposes a Canvas interface to allow you to draw elements onto 
 * it directly.
 */
UCLASS(MinimalAPI)
class UCanvasRenderTarget2D : public UTextureRenderTarget2D
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Updates the the canvas render target texture's resource. This is where the render target will create or 
	 * find a canvas object to use.  It also calls UpdateResourceImmediate() to clear the render target texture 
	 * from the deferred rendering list, to stop the texture from being cleared the next frame. From there it
	 * will ask the rendering thread to set up the RHI viewport. The canvas is then set up for rendering and 
	 * then the user's update delegate is called.  The canvas is then flushed and the RHI resolves the 
	 * texture to make it available for rendering.
	 */
	virtual void UpdateResource();

	/**
	 * Assigns a user delegate to be called every time the texture needs to be refreshed for rendering.  This delegate
	 * passes along a Canvas object that you can use to draw to the texture directly!
	 *
	 * @param	InDelegate	The canvas update delegate to use.  You can change this at any time.
	 */
	void SetOnCanvasRenderTargetUpdate( const FOnCanvasRenderTargetUpdate& InDelegate )
	{
		OnCanvasRenderTargetUpdate = InDelegate;
	}

private:

	/** Texture update delegate */
	FOnCanvasRenderTargetUpdate OnCanvasRenderTargetUpdate;
};
