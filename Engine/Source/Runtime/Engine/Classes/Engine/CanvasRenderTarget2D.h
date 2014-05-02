// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Canvas.h"
#include "CanvasRenderTarget2D.generated.h"

/** This delegate is assignable through Blueprint and has similar functionality to the above. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCanvasRenderTargetUpdate, UCanvas*, Canvas, int32, Width, int32, Height);

/**
 * CanvasRenderTarget2D is 2D render target which exposes a Canvas interface to allow you to draw elements onto 
 * it directly.  Use FindCanvasRenderTarget2D() to find or create a render target texture by unique name, then
 * bind a function to the OnCanvasRenderTargetUpdate delegate which will be called when the render target is
 * updated.  If you need to repaint your canvas every single frame, simply call UpdateResource() on it from a Tick
 * function.  Also, remember to hold onto your new canvas render target with a reference so that it doesn't get
 * garbage collected.
 */
UCLASS()
class ENGINE_API UCanvasRenderTarget2D : public UTextureRenderTarget2D
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
	 * Statically finds a canvas render target from the global map of canvas render targets. It can optionally create a new render target if not found as well.
	 *
	 * @param	InRenderTargetName	Name of the render target. This should be unique.
	 * @param	Width				Width of the render target.
	 * @param	Height				Height of the render target.
	 * @param	bCreateIfNotFound	True to create the render target if we couldn't find one with this name
	 * @return						Returns the instanced render target.
	 */
	UFUNCTION(BlueprintCallable, Category="Canvas Render Target 2D")
	static UCanvasRenderTarget2D* FindCanvasRenderTarget2D(FName InRenderTargetName, int32 Width, int32 Height, bool bCreateIfNotFound=false);

	/**
	 * Updates a specific render target from the global map of canvas render targets.  Does not trigger an error if the specified render target can't be found.
	 *
	 * @param	InRenderTargetName		Name of the render target to update.
	 */
	UFUNCTION(BlueprintCallable, Category="Canvas Render Target 2D")
	static void UpdateCanvasRenderTarget(FName InRenderTargetName);

	/**
	 * Gets a specific render target's size from the global map of canvas render targets.  Does not trigger an error if the specified render target can't be found.
	 *
	 * @param	Width	Out: The width of this render target
	 * @param	Height	Out: The height of this render target
	 */
	UFUNCTION(BlueprintCallable, Category="Canvas Render Target 2D")
	static void GetCanvasRenderTargetSize(FName InRenderTargetName, int32& Width, int32& Height);


	/** Called when this Canvas Render Target is asked to update its texture resource. */
	UPROPERTY(BlueprintAssignable, Category="Canvas Render Target 2D")
	FOnCanvasRenderTargetUpdate OnCanvasRenderTargetUpdate;


protected:

	/** Global mapped array of all canvas render targets */
	static TMap<FName, TAutoWeakObjectPtr<UCanvasRenderTarget2D>> GCanvasRenderTarget2Ds;
};