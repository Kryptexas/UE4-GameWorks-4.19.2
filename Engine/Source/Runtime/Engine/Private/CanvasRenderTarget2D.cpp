// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/CanvasRenderTarget2D.h"

UCanvasRenderTarget2D::UCanvasRenderTarget2D( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	bNeedsTwoCopies = false;
}


void UCanvasRenderTarget2D::UpdateResource()
{
	// Call parent implementation
	Super::UpdateResource();
	
	// Create or find the canvas object to use to render onto the texture.  Multiple canvas render target textures can share the same canvas.
	static const FName CanvasName( TEXT( "CanvasRenderTarget2DCanvas" ) );
	UCanvas* Canvas = (UCanvas*)StaticFindObjectFast(UCanvas::StaticClass(), GetTransientPackage(), CanvasName );
	if (Canvas == nullptr)
	{
		Canvas = ConstructObject<UCanvas>(UCanvas::StaticClass(), GetTransientPackage(), CanvasName );
		Canvas->AddToRoot();
	}

	Canvas->Init(GetSurfaceWidth(), GetSurfaceHeight(), nullptr);
	Canvas->Update();

	// Update the resource immediately to remove it from the deferred resource update list. This prevents the texture
	// from being cleared each frame.
	UpdateResourceImmediate();

	// Enqueue the rendering command to set up the rendering canvas.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		CanvasRenderTargetMakeCurrentCommand,
		FTextureRenderTarget2DResource*,
		TextureRenderTarget,
		(FTextureRenderTarget2DResource*)GameThread_GetRenderTargetResource(),
		{
			RHISetRenderTarget(TextureRenderTarget->GetRenderTargetTexture(), FTexture2DRHIRef());
			RHISetViewport(0, 0, 0.0f, TextureRenderTarget->GetSizeXY().X, TextureRenderTarget->GetSizeXY().Y, 1.0f);
		}
	);

	// Create the FCanvas which does the actual rendering.
	FCanvas RenderCanvas(GameThread_GetRenderTargetResource(), nullptr, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime);
	Canvas->Canvas = &RenderCanvas;

	// Check if the render target update delegate is bound and if it is, execute the delegate with the current Canvas being used.
	if (OnCanvasRenderTargetUpdate.IsBound())
	{
		OnCanvasRenderTargetUpdate.Execute(Canvas);
	}

	// Clean up and flush the rendering canvas.
	Canvas->Canvas = nullptr;
	RenderCanvas.Flush();

	// Enqueue the rendering command to copy the freshly rendering texture resource back to the render target RHI 
	// so that the texture is updated and available for rendering.
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER
	(
		CanvasRenderTargetResolveCommand,
		FTextureRenderTargetResource*,
		RenderTargetResource,
		GameThread_GetRenderTargetResource(),
		{
			RHICopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, true, FResolveParams());
		}
	);
}
