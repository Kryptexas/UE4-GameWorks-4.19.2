// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/CanvasRenderTarget2D.h"

TMap<FName, TAutoWeakObjectPtr<UCanvasRenderTarget2D>> UCanvasRenderTarget2D::GCanvasRenderTarget2Ds;

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

	if (!IsPendingKill() && OnCanvasRenderTargetUpdate.IsBound())
	{
		OnCanvasRenderTargetUpdate.Broadcast(Canvas, GetSurfaceWidth(), GetSurfaceHeight());
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

UCanvasRenderTarget2D* UCanvasRenderTarget2D::FindCanvasRenderTarget2D(FName InRenderTargetName, int32 Width, int32 Height, bool bCreateIfNotFound)
{
	if (Width > 0 && Height > 0 && InRenderTargetName != NAME_None)
	{
		// Check if the render target with the name has already been created.
		if (GCanvasRenderTarget2Ds.Num() > 0)
		{
			for (TMap<FName, TAutoWeakObjectPtr<UCanvasRenderTarget2D>>::TConstIterator It(GCanvasRenderTarget2Ds); It; ++It)
			{
				if (It->Key == InRenderTargetName && (It->Value).IsValid())
				{
					return (It->Value).Get();
				}
			}
		}

		// Create a new one since it doesn't exist.
		if (bCreateIfNotFound)
		{
			UCanvasRenderTarget2D* NewCanvasRenderTarget = ConstructObject<UCanvasRenderTarget2D>(UCanvasRenderTarget2D::StaticClass(), GetTransientPackage());
			if (NewCanvasRenderTarget)
			{
				NewCanvasRenderTarget->InitAutoFormat(Width, Height);

				// Add it to the global static map.
				GCanvasRenderTarget2Ds.Add(InRenderTargetName, NewCanvasRenderTarget);

				return NewCanvasRenderTarget;
			}
		}
	}

	return nullptr;
}

void UCanvasRenderTarget2D::UpdateCanvasRenderTarget(FName InRenderTargetName)
{
	if (GCanvasRenderTarget2Ds.Num() > 0)
	{
		for (TMap<FName, TAutoWeakObjectPtr<UCanvasRenderTarget2D>>::TConstIterator It(GCanvasRenderTarget2Ds); It; ++It)
		{
			if (It->Key == InRenderTargetName && (It->Value).IsValid())
			{
				(It->Value)->UpdateResource();
				
				break;
			}
		}
	}
}

void UCanvasRenderTarget2D::GetCanvasRenderTargetSize(FName InRenderTargetName, int32& Width, int32& Height)
{
	if (GCanvasRenderTarget2Ds.Num() > 0)
	{
		for (TMap<FName, TAutoWeakObjectPtr<UCanvasRenderTarget2D>>::TConstIterator It(GCanvasRenderTarget2Ds); It; ++It)
		{
			if (It->Key == InRenderTargetName && (It->Value).IsValid())
			{
				Width = (It->Value)->GetSurfaceWidth();
				Height = (It->Value)->GetSurfaceHeight();

				break;
			}
		}
	}
}