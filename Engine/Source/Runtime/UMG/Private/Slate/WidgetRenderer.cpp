// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "WidgetRenderer.h"

#if !UE_SERVER
#include "ISlateRHIRendererModule.h"
#include "ISlate3DRenderer.h"
#endif // !UE_SERVER

extern SLATECORE_API int32 bFoldTick;

void SVirtualWindow::Construct(const FArguments& InArgs)
{
	bIsPopupWindow = true;
	SetCachedSize(InArgs._Size);
	SetNativeWindow(MakeShareable(new FGenericWindow()));
	SetContent(SNullWidget::NullWidget);
}

FWidgetRenderer::FWidgetRenderer()
{
	Renderer = FModuleManager::Get().LoadModuleChecked<ISlateRHIRendererModule>("SlateRHIRenderer").CreateSlate3DRenderer();
}

FWidgetRenderer::~FWidgetRenderer()
{
}

ISlate3DRenderer* FWidgetRenderer::GetSlateRenderer()
{
	return Renderer.Get();
}

UTextureRenderTarget2D* FWidgetRenderer::DrawWidget(TSharedRef<SWidget>& Widget, FVector2D DrawSize)
{
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->InitCustomFormat(DrawSize.X, DrawSize.Y, PF_B8G8R8A8, false);

	DrawWidget(RenderTarget, Widget, DrawSize, 0);

	return RenderTarget;
}

void FWidgetRenderer::DrawWidget(UTextureRenderTarget2D* RenderTarget, TSharedRef<SWidget>& Widget, FVector2D DrawSize, float DeltaTime)
{
	TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow).Size(DrawSize);
	TSharedRef<FHittestGrid> HitTestGrid = MakeShareable(new FHittestGrid);

	Window->SetContent(Widget);
	Window->Resize(DrawSize);

	DrawWindow(RenderTarget, HitTestGrid, Window, DrawSize, DeltaTime);
}

void FWidgetRenderer::DrawWindow(UTextureRenderTarget2D* RenderTarget, TSharedRef<FHittestGrid> HitTestGrid, TSharedRef<SWindow> Window, FVector2D DrawSize, float DeltaTime)
{
#if !UE_SERVER
	FGeometry WindowGeometry = FGeometry::MakeRoot(DrawSize, FSlateLayoutTransform());

	if ( !bFoldTick )
	{
		Window->TickWidgetsRecursively(WindowGeometry, FApp::GetCurrentTime(), DeltaTime);
	}

	// Ticking can cause geometry changes.  Recompute
	Window->SlatePrepass();

	// Prepare the test grid 
	HitTestGrid->ClearGridForNewFrame(WindowGeometry.GetClippingRect());

	// Get the free buffer & add our virtual window
	FSlateDrawBuffer& DrawBuffer = Renderer->GetDrawBuffer();
	FSlateWindowElementList& WindowElementList = DrawBuffer.AddWindowElementList(Window);

	int32 MaxLayerId = 0;
	{
		// Paint the window
		MaxLayerId = Window->Paint(
			FPaintArgs(Window.Get(), HitTestGrid.Get(), FVector2D::ZeroVector, FApp::GetCurrentTime(), DeltaTime),
			WindowGeometry, WindowGeometry.GetClippingRect(),
			WindowElementList,
			0,
			FWidgetStyle(),
			Window->IsEnabled());
	}

	Renderer->DrawWindow_GameThread(DrawBuffer);

	// Enqueue a command to unlock the draw buffer after all windows have been drawn
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(UWidgetComponentRenderToTexture,
		FSlateDrawBuffer&, InDrawBuffer, DrawBuffer,
		UTextureRenderTarget2D*, InRenderTarget, RenderTarget,
		ISlate3DRenderer*, InRenderer, Renderer.Get(),
		{
			InRenderer->DrawWindowToTarget_RenderThread(RHICmdList, InRenderTarget, InDrawBuffer);
		});
#endif // !UE_SERVER
}
