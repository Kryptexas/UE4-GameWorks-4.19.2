// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FunctionalUIScreenshotTest.h"

#include "Engine/GameViewportClient.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "Misc/AutomationTest.h"
#include "Widgets/SViewport.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/SceneViewport.h"
#include "Slate/WidgetRenderer.h"

AFunctionalUIScreenshotTest::AFunctionalUIScreenshotTest( const FObjectInitializer& ObjectInitializer )
	: AScreenshotFunctionalTestBase(ObjectInitializer)
{
	WidgetLocation = EWidgetTestAppearLocation::Viewport;
}

/**
 * Get pixel format and color space of a backbuffer. Do nothing if the viewport doesn't
 * render into backbuffer directly
 * @InViewport - the viewport to get backbuffer from
 * @OutPixelFormat - pixel format of the backbuffer
 * @OutIsSRGB - whether the backbuffer stores pixels in sRGB space
 */ 
void GetBackbufferInfo(const FViewport* InViewport, EPixelFormat* OutPixelFormat, bool* OutIsSRGB)
{
	if (!InViewport->GetViewportRHI())
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(GetBackbufferFormatCmd)(
		[InViewport, OutPixelFormat, OutIsSRGB](FRHICommandListImmediate& RHICmdList)
	{
		FViewportRHIRef ViewportRHI = InViewport->GetViewportRHI();
		check(ViewportRHI.IsValid());
		FTexture2DRHIRef BackbufferTexture = RHICmdList.GetViewportBackBuffer(ViewportRHI);
		check(BackbufferTexture.IsValid());
		*OutPixelFormat = BackbufferTexture->GetFormat();
		*OutIsSRGB = (BackbufferTexture->GetFlags() & TexCreate_SRGB) == TexCreate_SRGB;
	});
	FlushRenderingCommands();
}

void AFunctionalUIScreenshotTest::PrepareTest()
{
	// Resize viewport to screenshot size
	Super::PrepareTest();

	// Resize screenshot render target to have the same size as the game viewport. Also
	// make sure they have the same data format (pixel format, color space, etc.) if possible
	const FSceneViewport* GameViewport = GEngine->GameViewport->GetGameViewport();
	FIntPoint ScreenshotSize = GameViewport->GetSizeXY();
	EPixelFormat PixelFormat = PF_A2B10G10R10;
	bool bIsSRGB = false;
	GetBackbufferInfo(GameViewport, &PixelFormat, &bIsSRGB);

	if (!ScreenshotRT)
	{
		ScreenshotRT = NewObject<UTextureRenderTarget2D>(this);
	}
	ScreenshotRT->ClearColor = FLinearColor::Transparent;
	ScreenshotRT->InitCustomFormat(ScreenshotSize.X, ScreenshotSize.Y, PixelFormat, !bIsSRGB);

	// Spawn the widget
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	SpawnedWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	
	if (SpawnedWidget)
	{
		if (WidgetLocation == EWidgetTestAppearLocation::Viewport)
		{
			SpawnedWidget->AddToViewport();
		}
		else
		{
			SpawnedWidget->AddToPlayerScreen();
		}

		SpawnedWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UAutomationBlueprintFunctionLibrary::FinishLoadingBeforeScreenshot();
}

void AFunctionalUIScreenshotTest::OnScreenshotTakenAndCompared()
{
	if (SpawnedWidget)
	{
		SpawnedWidget->RemoveFromParent();
	}

	// Restore viewport size and finish the test
	Super::OnScreenshotTakenAndCompared();
}

/**
 * Source and destination textures need to have the same data format (pixel format, color space, etc.)
 * @InViewport - backbuffer comes from this viewport
 * @InOutScreenshotRT - copy destination
 */
void CopyBackbufferToScreenshotRT(const FViewport* InViewport, UTextureRenderTarget2D* InOutScreenshotRT)
{
	// Make sure rendering to the viewport backbuffer is finished
	FlushRenderingCommands();

	ENQUEUE_RENDER_COMMAND(CopyBackbufferCmd)(
		[InViewport, InOutScreenshotRT](FRHICommandListImmediate& RHICmdList)
	{
		FViewportRHIRef ViewportRHI = InViewport->GetViewportRHI();
		FTexture2DRHIRef BackbufferTexture = RHICmdList.GetViewportBackBuffer(ViewportRHI);
		FTexture2DRHIRef ScreenshotRTTexture = InOutScreenshotRT->GetRenderTargetResource()->GetRenderTargetTexture();
		check(BackbufferTexture->GetSizeXY() == ScreenshotRTTexture->GetSizeXY());
		RHICmdList.CopyToResolveTarget(BackbufferTexture, ScreenshotRTTexture, true, FResolveParams());
	});
	FlushRenderingCommands();
}

void ReadPixelsFromRT(UTextureRenderTarget2D* InRT, TArray<FColor>* OutPixels)
{
	ENQUEUE_RENDER_COMMAND(ReadScreenshotRTCmd)(
		[InRT, OutPixels](FRHICommandListImmediate& RHICmdList)
	{
		FTextureRenderTarget2DResource* RTResource =
			static_cast<FTextureRenderTarget2DResource*>(InRT->GetRenderTargetResource());
		RHICmdList.ReadSurfaceData(
			RTResource->GetTextureRHI(),
			FIntRect(0, 0, InRT->SizeX, InRT->SizeY),
			*OutPixels,
			FReadSurfaceDataFlags());
	});
	FlushRenderingCommands();
}

void AFunctionalUIScreenshotTest::RequestScreenshot()
{
	// Register a handler to UGameViewportClient::OnScreenshotCaptured
	Super::RequestScreenshot();

	UGameViewportClient* GameViewportClient = GEngine->GameViewport;
	TSharedPtr<SViewport> ViewportWidget = GameViewportClient->GetGameViewportWidget();
	FIntPoint ScreenshotSize = GameViewportClient->GetGameViewport()->GetSizeXY();
	TArray<FColor> OutColorData;

	if (ViewportWidget.IsValid())
	{
		// If SViewport renders directly to backbuffer (no separate RT), the scene will
		// be black (only widgets will be rendered) if we just render SViewport. So we
		// need to copy the scene to ScreenshotRT before rendering the test widget
		if (ViewportWidget->ShouldRenderDirectly())
		{
			const FSceneViewport* GameViewport = GameViewportClient->GetGameViewport();
			check(GameViewport);
			CopyBackbufferToScreenshotRT(GameViewport, ScreenshotRT);
		}

		// Draw the game viewport (overlaid with the widget to screenshot) to our ScreenshotRT
		TSharedPtr<FWidgetRenderer> WidgetRenderer = MakeShareable(new FWidgetRenderer(true, false));
		check(WidgetRenderer.IsValid());
		WidgetRenderer->DrawWidget(ScreenshotRT, ViewportWidget.ToSharedRef(), ScreenshotSize, 0.f);
		FlushRenderingCommands();

		// Possibly resolves the RT to a texture so we can read it back
		ScreenshotRT->UpdateResourceImmediate(false);

		ReadPixelsFromRT(ScreenshotRT, &OutColorData);
		check(OutColorData.Num() == ScreenshotSize.X * ScreenshotSize.Y);

		// For UI, we only care about what the final image looks like. So don't compare alpha channel.
		// In editor, scene is rendered into a PF_B8G8R8A8 RT and then copied over to the R10B10G10A2 swapchain back buffer and
		// this copy ignores alpha. In game, however, scene is directly rendered into the back buffer and the alpha values are
		// already meaningless at that stage.
		for (int32 Idx = 0; Idx < OutColorData.Num(); ++Idx)
		{
			OutColorData[Idx].A = 0xff;
		}
	}

	GameViewportClient->OnScreenshotCaptured().Broadcast(ScreenshotSize.X, ScreenshotSize.Y, OutColorData);
}
