// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "Slate3DRenderer.h"
#include "ElementBatcher.h"

FSlate3DRenderer::FSlate3DRenderer( TSharedPtr<FSlateRHIResourceManager> InResourceManager, TSharedPtr<FSlateFontCache> InFontCache )
	: ResourceManager( InResourceManager.ToSharedRef() )
	, FontCache( InFontCache.ToSharedRef() )
{

	RenderTargetPolicy = MakeShareable( new FSlateRHIRenderingPolicy( FontCache, ResourceManager ) );
	RenderTargetPolicy->SetUseGammaCorrection( false );

	ElementBatcher = MakeShareable(new FSlateElementBatcher(RenderTargetPolicy.ToSharedRef()));
}

FSlate3DRenderer::~FSlate3DRenderer()
{
	if( RenderTargetPolicy.IsValid() )
	{
		RenderTargetPolicy->ReleaseResources();
	}

	FlushRenderingCommands();
}

FSlateDrawBuffer& FSlate3DRenderer::GetDrawBuffer()
{
	FreeBufferIndex = (FreeBufferIndex + 1) % 2;

	FSlateDrawBuffer* Buffer = &DrawBuffers[FreeBufferIndex];

	while (!Buffer->Lock())
	{
		FlushRenderingCommands();

		UE_LOG(LogSlate, Log, TEXT("Slate: Had to block on waiting for a draw buffer"));
		FreeBufferIndex = (FreeBufferIndex + 1) % NumDrawBuffers;

		Buffer = &DrawBuffers[FreeBufferIndex];
	}


	Buffer->ClearBuffer();

	return *Buffer;
}

void FSlate3DRenderer::DrawWindow_GameThread(FSlateDrawBuffer& DrawBuffer)
{
	check( IsInGameThread() );

	TArray<TSharedPtr<FSlateWindowElementList>>& WindowElementLists = DrawBuffer.GetWindowElementLists();

	// Only one window element list for now.
	check( WindowElementLists.Num() == 1);

	FSlateWindowElementList& ElementList = *WindowElementLists[0];

	TSharedPtr<SWindow> Window = ElementList.GetWindow();

	if (Window.IsValid())
	{
		const FVector2D WindowSize = Window->GetSizeInScreen();
		if (WindowSize.X > 0 && WindowSize.Y > 0)
		{
			// Add all elements for this window to the element batcher
			ElementBatcher->AddElements(ElementList);

			// Update the font cache with new text after elements are batched
			FontCache->UpdateCache();

			bool temp = false;


			// All elements for this window have been batched and rendering data updated
			ElementBatcher->ResetBatches();
		}
	}

	FontCache->ConditionalFlushCache();
}

void FSlate3DRenderer::DrawWindowToTarget_RenderThread( FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* RenderTarget, FSlateDrawBuffer& InDrawBuffer )
{
	FSlateBatchData& BatchData = InDrawBuffer.GetWindowElementLists()[0]->GetBatchData();

	BatchData.CreateRenderBatches();
	RenderTargetPolicy->BeginDrawingWindows();

	RenderTargetPolicy->UpdateVertexAndIndexBuffers( RHICmdList, BatchData );
	FTextureRenderTarget2DResource* RenderTargetResource = static_cast<FTextureRenderTarget2DResource*>( RenderTarget->GetRenderTargetResource() );
	
	// Set render target and clear.
	FRHIRenderTargetView ColorRTV(RenderTargetResource->GetTextureRHI());
	FRHISetRenderTargetsInfo Info(1, &ColorRTV, FTextureRHIParamRef());
	Info.bClearColor = true;
	ensure(ColorRTV.Texture->GetClearColor() == RenderTarget->ClearColor);

	RHICmdList.SetRenderTargetsAndClear(Info);

	FMatrix ProjectionMatrix = FSlateRHIRenderer::CreateProjectionMatrix( RenderTarget->SizeX, RenderTarget->SizeY );
	if (BatchData.GetRenderBatches().Num() > 0)
	{
		FSlateBackBuffer BackBufferTarget(RenderTargetResource->GetTextureRHI(), FIntPoint(RenderTarget->SizeX, RenderTarget->SizeY ) );

		// The scene renderer will handle it in this case
		const bool bAllowSwitchVerticalAxis = false;

		RenderTargetPolicy->DrawElements
		(
			RHICmdList,
			BackBufferTarget,
			ProjectionMatrix,
			BatchData.GetRenderBatches(),
			bAllowSwitchVerticalAxis
		);
	}

	InDrawBuffer.Unlock();

	RenderTargetPolicy->EndDrawingWindows();
}