// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "StandaloneRendererPrivate.h"
#include "OpenGL/SlateOpenGLRenderer.h"
#include "MacWindow.h"

FSlateOpenGLViewport::FSlateOpenGLViewport()
:	Framebuffer(0)
,	Renderbuffer(0)
{
}

void FSlateOpenGLViewport::Initialize(TSharedRef<SWindow> InWindow, const FSlateOpenGLContext& SharedContext)
{
	TSharedRef<FGenericWindow> NativeWindow = InWindow->GetNativeWindow().ToSharedRef();
	RenderingContext.Viewport = this;
	RenderingContext.Initialize(NativeWindow->GetOSWindowHandle(), &SharedContext);

	const int32 Width = FMath::TruncToInt(InWindow->GetSizeInScreen().X);
	const int32 Height = FMath::TruncToInt(InWindow->GetSizeInScreen().Y);

	ViewportRect.Top = 0;
	ViewportRect.Left = 0;

	Resize(Width, Height, false);
}

void FSlateOpenGLViewport::Destroy()
{
	if (Framebuffer)
	{
		glDeleteFramebuffers(1, &Framebuffer);
		Framebuffer = 0;
	}
	if (Renderbuffer)
	{
		glDeleteRenderbuffers(1, &Renderbuffer);
		Renderbuffer = 0;
	}
	RenderingContext.Destroy();
}

void FSlateOpenGLViewport::MakeCurrent()
{
	ContextGuard.Lock();
	RenderingContext.MakeCurrent();
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
}

void FSlateOpenGLViewport::SwapBuffers()
{
	glFlushRenderAPPLE();
	[(FCocoaWindow*)[RenderingContext.View window] startRendering];
	[RenderingContext.View setNeedsDisplay:YES];
	ContextGuard.Unlock();
}

void FSlateOpenGLViewport::Resize(int32 Width, int32 Height, bool bFullscreen)
{
	FScopeLock Lock(&ContextGuard);

	ViewportRect.Right = Width;
	ViewportRect.Bottom = Height;

	// Need to create a new projection matrix each time the window is resized
	ProjectionMatrix = CreateProjectionMatrix(Width, Height);

	if (RenderingContext.Context && Width > 0 && Height > 0)
	{
		[RenderingContext.Context update];

		if (Framebuffer == 0)
		{
			glGenFramebuffers(1, &Framebuffer);
			check(Framebuffer);
		}

		if (Renderbuffer == 0)
		{
			glGenRenderbuffers(1, &Renderbuffer);
			check(Renderbuffer);
		}

		int32 CurrentDrawFramebuffer = 0;
		int32 CurrentReadFramebuffer = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &CurrentDrawFramebuffer);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &CurrentReadFramebuffer);

		glBindRenderbuffer(GL_RENDERBUFFER, Renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, Width, Height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, Renderbuffer);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, CurrentDrawFramebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, CurrentReadFramebuffer);
	}
}

void FSlateOpenGLViewport::Draw()
{
	FScopeLock Lock(&ContextGuard);
	if (RenderingContext.View && Framebuffer && [(FCocoaWindow*)[RenderingContext.View window] isRenderInitialized])
	{
		int32 CurrentReadFramebuffer = 0;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &CurrentReadFramebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, Framebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBlitFramebuffer(0, 0, ViewportRect.Right, ViewportRect.Bottom, 0, 0, RenderingContext.View.frame.size.width, RenderingContext.View.frame.size.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		CHECK_GL_ERRORS;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, CurrentReadFramebuffer);
	}
	else
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
