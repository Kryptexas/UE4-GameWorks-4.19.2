// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OpenGLDrvPrivate.h"
#include "IOSAppDelegate.h"
#include "EAGLView.h"
#include "Slate.h"

// This header file gives access to the thread_policy_set function.
#include <mach/thread_act.h>

struct FPlatformOpenGLContext
{
	EAGLContext*	Context;
	GLuint			ViewportFramebuffer;

	FPlatformOpenGLContext()
	{
		Context = NULL;
		ViewportFramebuffer = 0;
	}
};

struct FPlatformOpenGLDevice
{
	FPlatformOpenGLContext RenderingContext;
	FPlatformOpenGLContext SharedContext;
	bool bSingleContext;

	FPlatformOpenGLDevice()
	{
		//SetRealTimeMode(pthread_self(), 20, 60);

		bSingleContext = !GUseThreadedRendering;

		IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
		EAGLView* GLView = AppDelegate.GLView;

		// EAGL Context (as the rendering one) on the EAGLView
		check(GLView->Context);
		RenderingContext.Context = GLView->Context;

		// @todo-mobile
		FOpenGL::LabelObject(GL_RENDERBUFFER, GLView.OnScreenColorRenderBuffer, "OnScreenColorRB");
		FOpenGL::LabelObject(GL_FRAMEBUFFER, GLView->ResolveFrameBuffer, "ResolveFB");
		RenderingContext.ViewportFramebuffer = GLView->ResolveFrameBuffer;
		SharedContext.ViewportFramebuffer = GLView->ResolveFrameBuffer;

		if (bSingleContext)
		{
			[EAGLContext setCurrentContext:RenderingContext.Context];
		}
		else
		{
			// Create a shared context from the rendering one
			SharedContext.Context = [[EAGLContext alloc] initWithAPI:[RenderingContext.Context API] sharegroup: [RenderingContext.Context sharegroup]];
			check(SharedContext.Context);

			PlatformRenderingContextSetup(this);
			InitDefaultGLContextState();
		}

		PlatformSharedContextSetup(this);
		InitDefaultGLContextState();
	}

	bool SetRealTimeMode(pthread_t ThreadHandle, uint32 NormalProcessingTimeMs, uint32 ConstraintProcessingTimeMs)
	{
		static bool bSingleton = true;
		check(bSingleton);
		bSingleton = !bSingleton;

		mach_timebase_info_data_t TimeBaseInfo;
		mach_timebase_info( &TimeBaseInfo );
		double MsToAbs = ((double)TimeBaseInfo.denom / (double)TimeBaseInfo.numer) * 1000000.0;

		thread_time_constraint_policy_data_t Policy;
		Policy.period      = 0;
		Policy.computation = (uint32_t)(NormalProcessingTimeMs * MsToAbs);
		Policy.constraint  = (uint32_t)(ConstraintProcessingTimeMs * MsToAbs);
		Policy.preemptible = true;

		kern_return_t Result = thread_policy_set(pthread_mach_thread_np(ThreadHandle), THREAD_TIME_CONSTRAINT_POLICY, (thread_policy_t)&Policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);

		return (Result == KERN_SUCCESS);
	}
};

FPlatformOpenGLDevice* PlatformCreateOpenGLDevice()
{
	return new FPlatformOpenGLDevice;
}

void PlatformDestroyOpenGLDevice(FPlatformOpenGLDevice* Device)
{
}

FPlatformOpenGLContext* PlatformCreateOpenGLContext(FPlatformOpenGLDevice* Device, void* InWindowHandle)
{
	// @todo-mobile
	return Device->bSingleContext ? &Device->SharedContext : &Device->RenderingContext;
}

void PlatformReleaseOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
}

void PlatformDestroyOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
}

void PlatformBlitToViewport( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 BackbufferSizeX, uint32 BackbufferSizeY, bool bPresent,bool bLockToVsync, int32 SyncInterval )
{
	// @todo-mobile
	check(Device->bSingleContext || Context == &Device->RenderingContext);
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	EAGLView* GLView = AppDelegate.GLView;

	[GLView SwapBuffers];
}

void PlatformFlushIfNeeded()
{
	glFlush();
}

void PlatformRebindResources(FPlatformOpenGLDevice* Device)
{
	if (!Device->bSingleContext)
	{
		IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
		EAGLView* GLView = AppDelegate.GLView;

		glBindRenderbuffer(GL_RENDERBUFFER, GLView.OnScreenColorRenderBuffer);
	}
}


void PlatformRenderingContextSetup(FPlatformOpenGLDevice* Device)
{
	if (!Device->bSingleContext)
	{
		glFlush();
		[EAGLContext setCurrentContext:Device->RenderingContext.Context];
	}
}

void PlatformSharedContextSetup(FPlatformOpenGLDevice* Device)
{
	if (!Device->bSingleContext)
	{
		glFlush();
		[EAGLContext setCurrentContext:Device->SharedContext.Context];
	}
}

void PlatformNULLContextSetup()
{
	glFlush();
	[EAGLContext setCurrentContext:nil];
}

EOpenGLCurrentContext PlatformOpenGLCurrentContext(FPlatformOpenGLDevice* Device)
{
	if (Device->bSingleContext)
	{
		return CONTEXT_Shared;
	}

	EAGLContext* Current = [EAGLContext currentContext];
	if (Current == Device->RenderingContext.Context)
	{
		return CONTEXT_Rendering;
	}
	else if (Current == Device->SharedContext.Context)
	{
		return CONTEXT_Shared;
	}
	else if (Current)
	{
		return CONTEXT_Other;
	}

	return CONTEXT_Invalid;
}

FRHITexture* PlatformCreateBuiltinBackBuffer(FOpenGLDynamicRHI* OpenGLRHI, uint32 SizeX, uint32 SizeY)
{
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	EAGLView* GLView = AppDelegate.GLView;

	uint32 Flags = TexCreate_RenderTargetable;
	FOpenGLTexture2D* Texture2D = new FOpenGLTexture2D(OpenGLRHI, GLView.OnScreenColorRenderBuffer, GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, SizeX, SizeY, 0, 1, 1, 1, PF_B8G8R8A8, false, false, Flags);
	OpenGLTextureAllocated(Texture2D, Flags);

	return Texture2D;
}

void PlatformResizeGLContext( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 SizeX, uint32 SizeY, bool bFullscreen, bool bWasFullscreen, GLenum BackBufferTarget, GLuint BackBufferResource)
{
	check(Context);
	VERIFY_GL_SCOPE();
	glBindFramebuffer(GL_FRAMEBUFFER, Context->ViewportFramebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, BackBufferResource);

	glViewport(0, 0, SizeX, SizeY);

	// @todo-mobile Do we need to clear here?
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void PlatformGetSupportedResolution(uint32 &Width, uint32 &Height)
{
}

bool PlatformGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	return true;
}

void PlatformRestoreDesktopDisplayMode()
{
}

bool PlatformInitOpenGL()
{
	return true;
}

bool PlatformOpenGLContextValid()
{
	EAGLContext* Current = [EAGLContext currentContext];
	return Current != NULL;
}

int32 PlatformGlGetError()
{
	return glGetError();
}

void PlatformGetBackbufferDimensions( uint32& OutWidth, uint32& OutHeight )
{
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	OutWidth = DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left;
	OutHeight = DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top;
}

// =============================================================

void PlatformGetNewRenderQuery( GLuint* OutQuery, uint64* OutQueryContext )
{
}

void PlatformReleaseRenderQuery( GLuint Query, uint64 QueryContext )
{
}

bool PlatformContextIsCurrent( uint64 QueryContext )
{
	return true;
}

/** Toggles the filter used to upscale the GL view between nearest and linear. */
void ToggleUpscaleFilter()
{
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	EAGLView* GLView = AppDelegate.GLView;
	CAEAGLLayer* Layer = (CAEAGLLayer*)GLView.layer;
	if (Layer.magnificationFilter == kCAFilterNearest)
	{
		Layer.magnificationFilter = kCAFilterLinear;
	}
	else
	{
		Layer.magnificationFilter = kCAFilterNearest;
	}
	UE_LOG(LogRHI,Log,TEXT("iOS magnification filter: %s"), Layer.magnificationFilter == kCAFilterNearest ? TEXT("NEAREST") : TEXT("LINEAR"));
}
static FAutoConsoleCommand GToggleUpscaleFilterCmd(
	TEXT("ios.ToggleUpscaleFilter"),
	TEXT("Toggles the filter used to upscale the GL view between nearest and linear."),
	FConsoleCommandDelegate::CreateStatic(ToggleUpscaleFilter)
	);
