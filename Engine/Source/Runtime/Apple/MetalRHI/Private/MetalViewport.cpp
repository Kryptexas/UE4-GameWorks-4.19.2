// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalViewport.cpp: Metal viewport RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"
#if PLATFORM_MAC
#include "CocoaWindow.h"
#include "CocoaThread.h"
#else
#include "IOSAppDelegate.h"
#endif
#include "RenderCommandFence.h"

extern int32 GMetalSupportsIntermediateBackBuffer;

#if PLATFORM_MAC

@implementation FMetalView

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self)
	{
	}
	return self;
}

- (BOOL)isOpaque
{
	return YES;
}

- (BOOL)mouseDownCanMoveWindow
{
	return YES;
}

@end

#endif

FMetalViewport::FMetalViewport(void* WindowHandle, uint32 InSizeX,uint32 InSizeY,bool bInIsFullscreen,EPixelFormat Format)
: Drawable(nil)
{
#if PLATFORM_MAC
	MainThreadCall(^{
		FCocoaWindow* Window = (FCocoaWindow*)WindowHandle;
		const NSRect ContentRect = NSMakeRect(0, 0, InSizeX, InSizeY);
		View = [[FMetalView alloc] initWithFrame:ContentRect];
		[View setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
		[View setWantsLayer:YES];

		CAMetalLayer* Layer = [CAMetalLayer new];

		CGFloat bgColor[] = { 0.0, 0.0, 0.0, 0.0 };
		Layer.edgeAntialiasingMask = 0;
		Layer.masksToBounds = YES;
		Layer.backgroundColor = CGColorCreate(CGColorSpaceCreateDeviceRGB(), bgColor);
		Layer.presentsWithTransaction = NO;
		Layer.anchorPoint = CGPointMake(0.5, 0.5);
		Layer.frame = ContentRect;
		Layer.magnificationFilter = kCAFilterNearest;
		Layer.minificationFilter = kCAFilterNearest;

		[Layer setDevice:GetMetalDeviceContext().GetDevice()];
		
		[Layer setFramebufferOnly:NO];
		[Layer removeAllAnimations];

		[View setLayer:Layer];

		[Window setContentView:View];
		[[Window standardWindowButton:NSWindowCloseButton] setAction:@selector(performClose:)];
	}, NSDefaultRunLoopMode, true);
#endif
	Resize(InSizeX, InSizeY, bInIsFullscreen, Format);
}

FMetalViewport::~FMetalViewport()
{
	BackBuffer[0].SafeRelease();	// when the rest of the engine releases it, its framebuffers will be released too (those the engine knows about)
	BackBuffer[1].SafeRelease();
	check(!IsValidRef(BackBuffer[0]));
	check(!IsValidRef(BackBuffer[1]));
}

void FMetalViewport::BeginDrawingViewport()
{
	FScopeLock Lock(&Mutex);
	if (BackBuffersQueue.Num() > 0)
	{
		uint32 Index = GetViewportIndex(EMetalViewportAccessRHI);
		BackBuffer[Index] = BackBuffersQueue.Pop(false);
	}
}

uint32 FMetalViewport::GetViewportIndex(EMetalViewportAccessFlag Accessor) const
{
	switch(Accessor)
	{
		case EMetalViewportAccessRHI:
			check(IsInRHIThread() || IsInRenderingThread());
            return GRHISupportsRHIThread ? Accessor : EMetalViewportAccessRenderer;
		case EMetalViewportAccessRenderer:
			check(IsInRenderingThread());
			return Accessor;
		case EMetalViewportAccessGame:
			check(IsInGameThread());
			return EMetalViewportAccessRenderer;
		default:
			check(false);
			return EMetalViewportAccessRenderer;
	}
}

void FMetalViewport::Resize(uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen,EPixelFormat Format)
{
	bool bCanUseHDR = GRHISupportsHDROutput;
	
#if PLATFORM_MAC
	static bool sbHDROSVersionSafe = FPlatformMisc::MacOSXVersionCompare(10,13,0) >= 0;
	bCanUseHDR = bCanUseHDR && (sbHDROSVersionSafe || bInIsFullscreen || IsRunningGame());
#endif

	uint32 Index = GetViewportIndex(EMetalViewportAccessGame);
	
	FRHIResourceCreateInfo CreateInfo;

	Format = (Format == PF_FloatRGBA && bCanUseHDR) ? PF_FloatRGBA : PF_B8G8R8A8;
	MTLPixelFormat MetalFormat = (Format == PF_FloatRGBA && bCanUseHDR) ? MTLPixelFormatRGBA16Float : MTLPixelFormatBGRA8Unorm;
	if (IsValidRef(BackBuffer[Index]) && Format != BackBuffer[Index]->GetFormat())
	{
		// Really need to flush the RHI thread & GPU here...
		ENQUEUE_UNIQUE_RENDER_COMMAND(
									  FlushPendingRHICommands,
									  {
										  GRHICommandList.GetImmediateCommandList().BlockUntilGPUIdle();
									  }
									  );
		
		// Issue a fence command to the rendering thread and wait for it to complete.
		FRenderCommandFence Fence;
		Fence.BeginFence();	
		Fence.Wait();
	}

	if (GMetalSupportsIntermediateBackBuffer)
	{
		BackBuffer[Index] = (FMetalTexture2D*)(FTexture2DRHIParamRef)GDynamicRHI->RHICreateTexture2D(InSizeX, InSizeY, Format, 1, 1, TexCreate_RenderTargetable, CreateInfo);
	}
	else
	{
		BackBuffer[Index] = (FMetalTexture2D*)(FTexture2DRHIParamRef)GDynamicRHI->RHICreateTexture2D(InSizeX, InSizeY, Format, 1, 1, TexCreate_RenderTargetable | TexCreate_Presentable, CreateInfo);
	}
#if PLATFORM_MAC
	MainThreadCall(^
	{
		CAMetalLayer* MetalLayer = (CAMetalLayer*)View.layer;
		
		MetalLayer.drawableSize = CGSizeMake(InSizeX, InSizeY);
		
		if (MetalFormat != MetalLayer.pixelFormat)
		{
			MetalLayer.pixelFormat = MetalFormat;
		}
		
		BOOL bUsingHDR = MetalFormat == MTLPixelFormatRGBA16Float;
		if (bUsingHDR != MetalLayer.wantsExtendedDynamicRangeContent)
		{
			MetalLayer.wantsExtendedDynamicRangeContent = bUsingHDR;
		}
		
	}, NSDefaultRunLoopMode, true);
#else
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	FIOSView* GLView = AppDelegate.IOSView;
	[GLView UpdateRenderWidth:InSizeX andHeight:InSizeY];
	
	// check the size of the window
	float ScalingFactor = [[IOSAppDelegate GetDelegate].IOSView contentScaleFactor];
	CGRect ViewFrame = [[IOSAppDelegate GetDelegate].IOSView frame];
	check(FMath::TruncToInt(ScalingFactor * ViewFrame.size.width) == InSizeX &&
		  FMath::TruncToInt(ScalingFactor * ViewFrame.size.height) == InSizeY);
#endif
	BackBuffer[Index]->Surface.Viewport = this;
    if (GRHISupportsRHIThread)
	{
		FScopeLock Lock(&Mutex);
		BackBuffersQueue.Insert(BackBuffer[Index], 0);
	}
}

FMetalTexture2D* FMetalViewport::GetBackBuffer(EMetalViewportAccessFlag Accessor) const
{
	uint32 Index = GetViewportIndex(Accessor);
	check(IsValidRef(BackBuffer[Index]));
	return BackBuffer[Index];
}

id<MTLDrawable> FMetalViewport::GetDrawable(EMetalViewportAccessFlag Accessor)
{
	if (!Drawable)
	{
		@autoreleasepool
		{
			uint32 IdleStart = FPlatformTime::Cycles();
			
	#if PLATFORM_MAC
			CAMetalLayer* CurrentLayer = (CAMetalLayer*)[View layer];
			Drawable = CurrentLayer ? [[CurrentLayer nextDrawable] retain] : nil;
	#else
			Drawable = [[[IOSAppDelegate GetDelegate].IOSView MakeDrawable] retain];
	#endif
#if METAL_DEBUG_OPTIONS
			if (((id<CAMetalDrawable>)Drawable).layer.drawableSize.width != BackBuffer[GetViewportIndex(Accessor)]->GetSizeX() || ((id<CAMetalDrawable>)Drawable).layer.drawableSize.height != BackBuffer[GetViewportIndex(Accessor)]->GetSizeY())
			{
				UE_LOG(LogMetal, Display, TEXT("Viewport Size Mismatch: Drawable W:%f H:%f, Viewport W:%u H:%u"), ((id<CAMetalDrawable>)Drawable).layer.drawableSize.width, ((id<CAMetalDrawable>)Drawable).layer.drawableSize.height, BackBuffer[GetViewportIndex(Accessor)]->GetSizeX(), BackBuffer[GetViewportIndex(Accessor)]->GetSizeY());
			}
#endif
			GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUPresent] += FPlatformTime::Cycles() - IdleStart;
			GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUPresent]++;
		}
	}
	return Drawable;
}

id<MTLTexture> FMetalViewport::GetDrawableTexture(EMetalViewportAccessFlag Accessor)
{
	id<CAMetalDrawable> CurrentDrawable = (id<CAMetalDrawable>)GetDrawable(Accessor);
#if METAL_DEBUG_OPTIONS
	@autoreleasepool
	{
#if PLATFORM_MAC
		CAMetalLayer* CurrentLayer = (CAMetalLayer*)[View layer];
#else
		CAMetalLayer* CurrentLayer = (CAMetalLayer*)[[IOSAppDelegate GetDelegate].IOSView layer];
#endif
		
		uint32 Index = GetViewportIndex(Accessor);
		CGSize Size = CurrentLayer.drawableSize;
		if (CurrentDrawable.texture.width != BackBuffer[Index]->GetSizeX() || CurrentDrawable.texture.height != BackBuffer[Index]->GetSizeY())
		{
			UE_LOG(LogMetal, Display, TEXT("Viewport Size Mismatch: Drawable W:%f H:%f, Texture W:%llu H:%llu, Viewport W:%u H:%u"), Size.width, Size.height, CurrentDrawable.texture.width, CurrentDrawable.texture.height, BackBuffer[Index]->GetSizeX(), BackBuffer[Index]->GetSizeY());
		}
	}
#endif
	return CurrentDrawable.texture;
}

void FMetalViewport::ReleaseDrawable()
{
	if (Drawable)
	{
		[Drawable release];
		Drawable = nil;
	}
	if (!GMetalSupportsIntermediateBackBuffer && IsValidRef(BackBuffer[GetViewportIndex(EMetalViewportAccessRHI)]))
	{
		BackBuffer[GetViewportIndex(EMetalViewportAccessRHI)]->Surface.Texture = nil;
	}
}

#if PLATFORM_MAC
NSWindow* FMetalViewport::GetWindow() const
{
	return [View window];
}
#endif

/*=============================================================================
 *	The following RHI functions must be called from the main thread.
 *=============================================================================*/
FViewportRHIRef FMetalDynamicRHI::RHICreateViewport(void* WindowHandle,uint32 SizeX,uint32 SizeY,bool bIsFullscreen,EPixelFormat PreferredPixelFormat)
{
	check( IsInGameThread() );
	@autoreleasepool {
	return new FMetalViewport(WindowHandle, SizeX, SizeY, bIsFullscreen, PreferredPixelFormat);
	}
}

void FMetalDynamicRHI::RHIResizeViewport(FViewportRHIParamRef Viewport, uint32 SizeX, uint32 SizeY, bool bIsFullscreen)
{
	RHIResizeViewport(Viewport, SizeX, SizeY, bIsFullscreen, PF_Unknown);
}

void FMetalDynamicRHI::RHIResizeViewport(FViewportRHIParamRef ViewportRHI,uint32 SizeX,uint32 SizeY,bool bIsFullscreen,EPixelFormat Format)
{
	@autoreleasepool {
	check( IsInGameThread() );

	FMetalViewport* Viewport = ResourceCast(ViewportRHI);
	Viewport->Resize(SizeX, SizeY, bIsFullscreen, Format);
	}
}

void FMetalDynamicRHI::RHITick( float DeltaTime )
{
	check( IsInGameThread() );
}

/*=============================================================================
 *	Viewport functions.
 *=============================================================================*/

void FMetalRHICommandContext::RHIBeginDrawingViewport(FViewportRHIParamRef ViewportRHI, FTextureRHIParamRef RenderTargetRHI)
{
	check(false);
}

void FMetalRHIImmediateCommandContext::RHIBeginDrawingViewport(FViewportRHIParamRef ViewportRHI, FTextureRHIParamRef RenderTargetRHI)
{
	@autoreleasepool {
	FMetalViewport* Viewport = ResourceCast(ViewportRHI);
	check(Viewport);

	Viewport->BeginDrawingViewport();
	
	((FMetalDeviceContext*)Context)->BeginDrawingViewport(Viewport);

	// Set the render target and viewport.
	if (RenderTargetRHI)
	{
		FRHIRenderTargetView RTV(RenderTargetRHI, ERenderTargetLoadAction::ELoad);
		RHISetRenderTargets(1, &RTV, nullptr, 0, NULL);
	}
	else
	{
		FRHIRenderTargetView RTV(Viewport->GetBackBuffer(EMetalViewportAccessRHI), ERenderTargetLoadAction::ELoad);
		RHISetRenderTargets(1, &RTV, nullptr, 0, NULL);
	}
	}
}

void FMetalRHICommandContext::RHIEndDrawingViewport(FViewportRHIParamRef ViewportRHI,bool bPresent,bool bLockToVsync)
{
	check(false);
}

void FMetalRHIImmediateCommandContext::RHIEndDrawingViewport(FViewportRHIParamRef ViewportRHI,bool bPresent,bool bLockToVsync)
{
	@autoreleasepool {
	FMetalViewport* Viewport = ResourceCast(ViewportRHI);
	((FMetalDeviceContext*)Context)->EndDrawingViewport(Viewport, bPresent);
#if PLATFORM_MAC
	FCocoaWindow* Window = (FCocoaWindow*)Viewport->GetWindow();
	MainThreadCall(^{
		[Window startRendering];
	}, NSDefaultRunLoopMode, false);
#endif
	}
}

FTexture2DRHIRef FMetalDynamicRHI::RHIGetViewportBackBuffer(FViewportRHIParamRef ViewportRHI)
{
	@autoreleasepool {
	FMetalViewport* Viewport = ResourceCast(ViewportRHI);
	return Viewport->GetBackBuffer(EMetalViewportAccessRenderer);
	}
}

void FMetalDynamicRHI::RHIAdvanceFrameForGetViewportBackBuffer()
{
}
