// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Slate/DebugCanvas.h"
#include "RenderingThread.h"
#include "UnrealClient.h"
#include "CanvasTypes.h"
#include "Framework/Application/SlateApplication.h"




/**
 * Simple representation of the backbuffer that the debug canvas renders to
 * This class may only be accessed from the render thread
 */
class FSlateCanvasRenderTarget : public FRenderTarget
{
public:
	/** FRenderTarget interface */
	virtual FIntPoint GetSizeXY() const
	{
		return ViewRect.Size();
	}

	/** Sets the texture that this target renders to */
	void SetRenderTargetTexture( FTexture2DRHIRef& InRHIRef )
	{
		RenderTargetTextureRHI = InRHIRef;
	}

	/** Clears the render target texture */
	void ClearRenderTargetTexture()
	{
		RenderTargetTextureRHI.SafeRelease();
	}

	/** Sets the viewport rect for the render target */
	void SetViewRect( const FIntRect& InViewRect ) 
	{ 
		ViewRect = InViewRect;
	}

	/** Gets the viewport rect for the render target */
	const FIntRect& GetViewRect() const 
	{
		return ViewRect; 
	}
private:
	FIntRect ViewRect;
};

FDebugCanvasDrawer::FDebugCanvasDrawer()
	: GameThreadCanvas( NULL )
	, RenderThreadCanvas( NULL )
	, RenderTarget( new FSlateCanvasRenderTarget )
{}

FDebugCanvasDrawer::~FDebugCanvasDrawer()
{
	delete RenderTarget;

	// We assume that the render thread is no longer utilizing any canvases
	if( GameThreadCanvas.IsValid() && RenderThreadCanvas != GameThreadCanvas )
	{
		GameThreadCanvas.Reset();
	}

	RenderThreadCanvas.Reset();
}

FCanvas* FDebugCanvasDrawer::GetGameThreadDebugCanvas()
{
	return GameThreadCanvas.Get();
}


void FDebugCanvasDrawer::BeginRenderingCanvas( const FIntRect& CanvasRect )
{
	if( CanvasRect.Size().X > 0 && CanvasRect.Size().Y > 0 )
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER
		(
			BeginRenderingDebugCanvas,
			FDebugCanvasDrawer*, CanvasDrawer, this, 
			FCanvasPtr, CanvasToRender, GameThreadCanvas,
			FIntRect, CanvasRect, CanvasRect,
			{
				// Delete the old rendering thread canvas
				if( CanvasDrawer->GetRenderThreadCanvas().IsValid() && CanvasToRender.IsValid() )
				{
					CanvasDrawer->DeleteRenderThreadCanvas();
				}

				if(!CanvasToRender.IsValid())
				{
					CanvasToRender = CanvasDrawer->GetRenderThreadCanvas();
				}

				CanvasDrawer->SetRenderThreadCanvas( CanvasRect, CanvasToRender );
			}
		);
		
		// Gave the canvas to the render thread
		GameThreadCanvas = nullptr;
	}
}


void FDebugCanvasDrawer::InitDebugCanvas(UWorld* InWorld)
{
	// If the canvas is not null there is more than one viewport draw call before slate draws.  This can happen on resizes. 
	// We need to delete the old canvas
		// This can also happen if we are debugging a HUD blueprint and in that case we need to continue using
		// the same canvas
	if (FSlateApplication::Get().IsNormalExecution())
	{
		FCanvas* Canvas = new FCanvas(RenderTarget, nullptr, InWorld, InWorld ? InWorld->FeatureLevel : GMaxRHIFeatureLevel);

		GameThreadCanvas = MakeShared<FCanvas, ESPMode::ThreadSafe>(*Canvas);
	}
}

void FDebugCanvasDrawer::DrawRenderThread(FRHICommandListImmediate& RHICmdList, const void* InWindowBackBuffer)
{
	check( IsInRenderingThread() );

	if( RenderThreadCanvas.IsValid() )
	{
		FTexture2DRHIRef& RT = *(FTexture2DRHIRef*)InWindowBackBuffer;
		RenderTarget->SetRenderTargetTexture( RT );
		bool bNeedToFlipVertical = RenderThreadCanvas->GetAllowSwitchVerticalAxis();
		// Do not flip when rendering to the back buffer
		RenderThreadCanvas->SetAllowSwitchVerticalAxis(false);
		if (RenderThreadCanvas->IsScaledToRenderTarget() && IsValidRef(RT)) 
		{
			RenderThreadCanvas->SetRenderTargetRect( FIntRect(0, 0, RT->GetSizeX(), RT->GetSizeY()) );
		}
		else
		{
			RenderThreadCanvas->SetRenderTargetRect( RenderTarget->GetViewRect() );
		}
		RenderThreadCanvas->Flush_RenderThread(RHICmdList, true);
		RenderThreadCanvas->SetAllowSwitchVerticalAxis(bNeedToFlipVertical);
		RenderTarget->ClearRenderTargetTexture();
	}
}

FCanvasPtr FDebugCanvasDrawer::GetRenderThreadCanvas()
{
	check( IsInRenderingThread() );
	return RenderThreadCanvas;
}

void FDebugCanvasDrawer::DeleteRenderThreadCanvas()
{
	check( IsInRenderingThread() );
	RenderThreadCanvas.Reset();
}

void FDebugCanvasDrawer::SetRenderThreadCanvas( const FIntRect& InCanvasRect, FCanvasPtr& Canvas )
{
	check( IsInRenderingThread() );
	RenderTarget->SetViewRect( InCanvasRect );
	RenderThreadCanvas = Canvas;
}
