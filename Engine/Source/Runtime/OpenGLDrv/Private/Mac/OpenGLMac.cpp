// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OpenGLDrvPrivate.h"

#include "MacWindow.h"

/*------------------------------------------------------------------------------
	OpenGL context management.
------------------------------------------------------------------------------*/

char const* const CompositedBlitVertexShader = "#version 150\n"
"const int VertexCount = 6;\n"
"uniform int TextureDirection;\n"
"const vec2 Position[VertexCount] = vec2[](\n"
"	vec2(-1.0,-1.0),\n"
"	vec2( 1.0,-1.0),\n"
"	vec2( 1.0, 1.0),\n"
"	vec2(-1.0,-1.0),\n"
"	vec2( 1.0, 1.0),\n"
"	vec2(-1.0, 1.0));\n"
"const vec2 TexCoords[VertexCount] = vec2[](\n"
"	vec2(0.0, 1.0),\n"
"	vec2(1.0, 1.0),\n"
"	vec2(1.0, 0.0),\n"
"	vec2(0.0, 1.0),\n"
"	vec2(1.0, 0.0),\n"
"	vec2(0.0, 0.0));\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"	TexCoord = TexCoords[gl_VertexID];\n"
"	if(TextureDirection == 1){ TexCoord.x = (TextureDirection - TexCoords[gl_VertexID].x); }\n"
"	gl_Position = vec4(Position[gl_VertexID], 0.0, 1.0);\n"
"}\n";

char const* const CompositedBlitFragmentShader = "#version 150\n"
"uniform sampler2D WindowTexture;\n"
"in vec2 TexCoord;\n"
"out vec4 Color;\n"
"void main()\n"
"{\n"
"	vec4 WindowColor = texture(WindowTexture, TexCoord);\n"
"	Color = vec4(WindowColor.x, WindowColor.y, WindowColor.z, WindowColor.x);\n"
"}\n";

const NSOpenGLPixelFormatAttribute ContextCreationAttributes[] =
{
	kCGLPFAOpenGLProfile,
	kCGLOGLPVersion_3_2_Core,
	kCGLPFAAccelerated,
	kCGLPFANoRecovery,
	kCGLPFADoubleBuffer,
	kCGLPFAColorSize,
	32,
	0
};

static NSOpenGLPixelFormat* PixelFormat = nil;

static NSOpenGLContext* CreateContext( NSOpenGLContext* SharedContext )
{
	SCOPED_AUTORELEASE_POOL;

	if (!PixelFormat)
	{
		PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes: ContextCreationAttributes];
		check(PixelFormat);
	}

	NSOpenGLContext* Context = [[NSOpenGLContext alloc] initWithFormat: PixelFormat shareContext: SharedContext];
	check(Context);
	int32 SyncInterval = 0;
	[Context setValues: &SyncInterval forParameter: NSOpenGLCPSwapInterval];
	// Setup Opacity - can't change it dynamically later!
	int32 SurfaceOpacity = 0;
	[Context setValues: &SurfaceOpacity forParameter: NSOpenGLCPSurfaceOpacity];

	if (FParse::Param(FCommandLine::Get(),TEXT("openglUseMacMTEngine")))
	{
		CGLEnable((CGLContextObj)[Context CGLContextObj], kCGLCEMPEngine);
	}

	return Context;
}

class FScopeContext
{
public:
	FScopeContext( NSOpenGLContext* Context )
	{
		SCOPED_AUTORELEASE_POOL;

		check(Context);
		PreviousContext = [NSOpenGLContext currentContext];
		bSameContext = (PreviousContext == Context);
		if (!bSameContext)
		{
			if (PreviousContext)
			{
				glFlush();
			}
			[Context makeCurrentContext];
		}
	}

	~FScopeContext( void )
	{
		if (!bSameContext)
		{
			SCOPED_AUTORELEASE_POOL;

			glFlush();
			if (PreviousContext)
			{
				[PreviousContext makeCurrentContext];
			}
			else
			{
				[NSOpenGLContext clearCurrentContext];
			}
		}
	}

private:
	NSOpenGLContext*	PreviousContext;
	bool				bSameContext;
};

void DeleteQueriesForCurrentContext( NSOpenGLContext* Context );

@interface NSView(NSThemeFramePrivate)
- (float)roundedCornerRadius;
@end

/**
 * Custom view class
 */
@interface FCocoaOpenGLView : NSView
{
}

@end

@implementation FCocoaOpenGLView

- (void)drawRect:(NSRect)DirtyRect
{
	SCOPED_AUTORELEASE_POOL;
	
	[super drawRect:DirtyRect];
	
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow redrawContents];
		
		if([SlateCocoaWindow inLiveResize])
		{
			FlushRenderingCommands();
		}
	}
}

- (BOOL)acceptsFirstMouse:(NSEvent *)Event
{
	return YES;
}

/** Forward mouse events up to the window rather than through the responder chain - thus avoiding 
 *	the hidden titlebar controls. Normal windows just use the responder chain as usual.
 */
- (void)mouseDown:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow mouseDown:Event];
	}
	else
	{
		[super mouseDown:Event];
	}
}

- (void)rightMouseDown:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow rightMouseDown:Event];
	}
	else
	{
		[super rightMouseDown:Event];
	}
}

- (void)otherMouseDown:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow otherMouseDown:Event];
	}
	else
	{
		[super otherMouseDown:Event];
	}
}

- (void)mouseUp:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow mouseUp:Event];
	}
	else
	{
		[super mouseUp:Event];
	}
}

- (void)rightMouseUp:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow rightMouseUp:Event];
	}
	else
	{
		[super rightMouseUp:Event];
	}
}

- (void)otherMouseUp:(NSEvent*)Event
{
	FSlateCocoaWindow* SlateCocoaWindow = [[self window] isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)[self window] : nil;
	if (SlateCocoaWindow)
	{
		[SlateCocoaWindow otherMouseUp:Event];
	}
	else
	{
		[super otherMouseUp:Event];
	}
}

@end

/** Platform specific OpenGL context. */
struct FPlatformOpenGLContext
{
	NSWindow*			WindowHandle;
	FCocoaOpenGLView*	OpenGLView;
	NSOpenGLContext*	OpenGLContext;
	GLuint				ViewportFramebuffer;
	int32				SyncInterval;
	GLuint				VertexArrayObject;	// one has to be generated and set for each context (OpenGL 3.2 Core requirements)
};

struct OpenGLContextInfo
{
	NSOpenGLContext*	Context;
	GLuint				VertexArrayObject;
};

extern void OnQueryInvalidation( void );

struct FPlatformOpenGLDevice
{
	NSOpenGLContext*	SharedContext;
	GLuint				SharedContextVAO;
	GLuint				SharedContextCompositeVertexShader;
	GLuint				SharedContextCompositeFragmentShader;
	GLuint				SharedContextCompositeProgram;
	GLint				SharedContextWindowTextureUniform;
	GLint				SharedContextTextureDirectionUniform;
	GLuint				SharedContextCompositeTexture;
	uint32              SharedContextCompositeTextureSizeX;
    uint32              SharedContextCompositeTextureSizeY;

	NSOpenGLContext*	RenderingContext;
	GLuint				RenderingContextVAO;

	TArray<OpenGLContextInfo>	FreeOpenGLContexts;
	int32						NumUsedContexts;

	/** Guards against operating on viewport contexts from more than one thread at the same time. */
	FCriticalSection*			ContextUsageGuard;

	FPlatformOpenGLDevice()
		: NumUsedContexts(0)
	{
		SCOPED_AUTORELEASE_POOL;

		extern void InitDebugContext();
		ContextUsageGuard = new FCriticalSection;
		SharedContext = CreateContext(NULL);
		RenderingContext = CreateContext(SharedContext);

		[RenderingContext makeCurrentContext];
		InitDebugContext();
		glGenVertexArrays(1,&RenderingContextVAO);
		glBindVertexArray(RenderingContextVAO);
		InitDefaultGLContextState();
		glFlush();

		[SharedContext makeCurrentContext];
		InitDebugContext();
		glGenVertexArrays(1,&SharedContextVAO);
		glBindVertexArray(SharedContextVAO);
		
		SharedContextCompositeVertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(SharedContextCompositeVertexShader, 1, &CompositedBlitVertexShader, NULL);
		glCompileShader(SharedContextCompositeVertexShader);
		
		SharedContextCompositeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(SharedContextCompositeFragmentShader, 1, &CompositedBlitFragmentShader, NULL);
		glCompileShader(SharedContextCompositeFragmentShader);
		
		SharedContextCompositeProgram = glCreateProgram();
		glAttachShader(SharedContextCompositeProgram, SharedContextCompositeVertexShader);
		glAttachShader(SharedContextCompositeProgram, SharedContextCompositeFragmentShader);
		glBindFragDataLocation(SharedContextCompositeProgram, 0, "Color");
		glLinkProgram(SharedContextCompositeProgram);
		glValidateProgram(SharedContextCompositeProgram);
		SharedContextWindowTextureUniform = glGetUniformLocation(SharedContextCompositeProgram, "WindowTexture");
		SharedContextTextureDirectionUniform = glGetUniformLocation(SharedContextCompositeProgram, "TextureDirection");
		
		glGenTextures(1, &SharedContextCompositeTexture);
		SharedContextCompositeTextureSizeX = 0;
		SharedContextCompositeTextureSizeY = 0;
		
		InitDefaultGLContextState();
	}

	~FPlatformOpenGLDevice()
	{
		SCOPED_AUTORELEASE_POOL;

		check(NumUsedContexts==0);

		// Release all used OpenGL contexts
		{
			FScopeLock ScopeLock(ContextUsageGuard);
			for (int32 ContextIndex = 0; ContextIndex < FreeOpenGLContexts.Num(); ++ContextIndex)
			{
				{
					FScopeContext ScopeContext(FreeOpenGLContexts[ContextIndex].Context);
					DeleteQueriesForCurrentContext(FreeOpenGLContexts[ContextIndex].Context);
					glBindVertexArray(0);
					glDeleteVertexArrays(1, &FreeOpenGLContexts[ContextIndex].VertexArrayObject);
				}
				[FreeOpenGLContexts[ContextIndex].Context release];
			}
			FreeOpenGLContexts.Empty();
		}

		OnQueryInvalidation();

		{
			FScopeContext Context(RenderingContext);
			DeleteQueriesForCurrentContext(RenderingContext);
			glBindVertexArray(0);
			glDeleteVertexArrays(1,&RenderingContextVAO);
		}

		{
			FScopeContext Context(SharedContext);
			DeleteQueriesForCurrentContext(SharedContext);
			glBindVertexArray(0);
			glDeleteVertexArrays(1,&SharedContextVAO);
			
			glDeleteProgram(SharedContextCompositeProgram);
			glDeleteShader(SharedContextCompositeVertexShader);
			glDeleteShader(SharedContextCompositeFragmentShader);
			
			glDeleteTextures(1, &SharedContextCompositeTexture);
		}

		[RenderingContext release];
		[SharedContext release];

		delete ContextUsageGuard;
	}
};

FPlatformOpenGLDevice* PlatformCreateOpenGLDevice()
{
	return new FPlatformOpenGLDevice;
}

void PlatformDestroyOpenGLDevice(FPlatformOpenGLDevice* Device)
{
	delete Device;
}

FPlatformOpenGLContext* PlatformCreateOpenGLContext(FPlatformOpenGLDevice* Device, void* InWindowHandle)
{
	SCOPED_AUTORELEASE_POOL;

	check(InWindowHandle);

	FPlatformOpenGLContext* Context = new FPlatformOpenGLContext;

	Context->ViewportFramebuffer = 0;	// will be created on demand
	Context->SyncInterval = 0;
	Context->WindowHandle = (NSWindow*)InWindowHandle;

	NSRect ContentRect = [[Context->WindowHandle contentView] frame];
	Context->OpenGLView = [[FCocoaOpenGLView alloc] initWithFrame: ContentRect];

	{
		FScopeLock ScopeLock(Device->ContextUsageGuard);
		if( Device->FreeOpenGLContexts.Num() )
		{
			OpenGLContextInfo Info = Device->FreeOpenGLContexts.Pop();
			Context->OpenGLContext = Info.Context;
			Context->VertexArrayObject = Info.VertexArrayObject;
		}
		else
		{
			Context->OpenGLContext = CreateContext(Device->SharedContext);

			{
				FScopeContext ScopeContext(Context->OpenGLContext);

				glGenVertexArrays(1, &Context->VertexArrayObject);
				glBindVertexArray(Context->VertexArrayObject);
				
				InitDefaultGLContextState();
			}
		}
	}
	++Device->NumUsedContexts;

	// Attach the view to the window
	// Slate windows may require a view that fills the entire window & border frame, not just content
	if([Context->WindowHandle styleMask] & (NSTexturedBackgroundWindowMask))
	{
		// For windows where we want to hide the titlebar add the view as the uppermost child of the
		// window's superview.
		NSView* SuperView = [[Context->WindowHandle contentView] superview];
		
		[Context->OpenGLView setFrameSize:[Context->WindowHandle frame].size];
		[Context->OpenGLView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
		
		[SuperView addSubview:Context->OpenGLView];
	}
	else
	{
		// Otherwise, set as the content view to see the title bar.
		[Context->WindowHandle setContentView: Context->OpenGLView];
	}
	
	// Attach the context to the view if needed
	[Context->OpenGLContext setView: Context->OpenGLView];
	
	return Context;
}

void PlatformReleaseOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
	SCOPED_AUTORELEASE_POOL;

	check(Context && Context->OpenGLView && Context->OpenGLContext);

	{
		FScopeLock ScopeLock(Device->ContextUsageGuard);

		[Context->OpenGLContext clearDrawable];

		{
			OpenGLContextInfo Info;
			Info.Context = Context->OpenGLContext;
			Info.VertexArrayObject = Context->VertexArrayObject;
			Device->FreeOpenGLContexts.Add(Info);
		}

		Context->OpenGLContext = NULL;

		if (Context->ViewportFramebuffer)
		{
			check([NSOpenGLContext currentContext]);
			glDeleteFramebuffers(1,&Context->ViewportFramebuffer);	// this can be done from any context, as long as it's not nil.
			Context->ViewportFramebuffer = 0;
		}

		--Device->NumUsedContexts;
	}
	[Context->OpenGLView release];
	Context->OpenGLView = NULL;
}

void PlatformDestroyOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
	PlatformReleaseOpenGLContext(Device, Context);
	delete Context;
}

void PlatformBlitToViewport( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 BackbufferSizeX, uint32 BackbufferSizeY, bool bPresent,bool bLockToVsync, int32 SyncInterval )
{
	check(Context && Context->OpenGLView);

	{
		FScopeLock ScopeLock(Device->ContextUsageGuard);
		{
			FScopeContext ScopeContext(Context->OpenGLContext);
			
			// OpenGL state necessary for blit is set up in PlatformResizeGLContext(), and should be correct here,
			// as viewport contexts aren't bound at any other occasion.
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, Context->ViewportFramebuffer);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			glBlitFramebuffer(
				0, 0, BackbufferSizeX, BackbufferSizeY,
				0, BackbufferSizeY, BackbufferSizeX, 0,
				GL_COLOR_BUFFER_BIT,
				GL_NEAREST
				);

			if (bPresent)
			{
				SCOPED_AUTORELEASE_POOL;
				
				// Clear the Alpha channel
				glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_TRUE);
				glClearColor(0.f,0.f,0.f,1.f);
				glClear(GL_COLOR_BUFFER_BIT);
				glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
				glClearColor(0.f,0.f,0.f,0.f);
				
				bool bRoundedBlit = false;
				
				NSWindow* Window  = [Context->OpenGLView window];
				NSView* SuperView = [[Window contentView] superview];
				FSlateCocoaWindow* SlateCocoaWindow = [Window isKindOfClass:[FSlateCocoaWindow class]] ? (FSlateCocoaWindow*)Window : nil;
				if([SuperView respondsToSelector:@selector(roundedCornerRadius)] && SlateCocoaWindow)
				{
					bool bRoundedCorners = [SlateCocoaWindow roundedCorners];
					bool bFullWindowRendering = ([SlateCocoaWindow styleMask] & (NSTexturedBackgroundWindowMask));
					EWindowMode::Type WindowMode = [SlateCocoaWindow windowMode];
					bRoundedBlit = (WindowMode == EWindowMode::Windowed && bRoundedCorners && bFullWindowRendering);
				}
				
				if(bRoundedBlit)
				{
					bool bGenerateTexture = !Device->SharedContextCompositeTextureSizeX && !Device->SharedContextCompositeTextureSizeY;
					glBindSampler(0, 0);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture (GL_TEXTURE_2D, Device->SharedContextCompositeTexture);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					
					if(bGenerateTexture)
					{
						uint32 Size = 32;
						Device->SharedContextCompositeTextureSizeX = Size;
						Device->SharedContextCompositeTextureSizeY = Size;
						NSImage* MaskImage = [[[NSImage alloc] initWithSize:NSMakeSize(Size*2, Size*2)] autorelease];
						{
							[MaskImage lockFocus];
							{
								NSGraphicsContext* CurrentContext = [NSGraphicsContext currentContext];
								[CurrentContext saveGraphicsState];
								[CurrentContext setShouldAntialias: NO];
								
								[[NSColor clearColor] set];
								[[NSBezierPath bezierPathWithRect:NSMakeRect(0.f, 0.f, Size*2, Size*2)] fill];
								
								[[NSColor colorWithDeviceRed:0.0f green:0.0f blue:0.0f alpha:1.0f] set];
								float Radius = [SuperView roundedCornerRadius] * 1.6f;
								[[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(0.f, 0.f, Size*2, Size*2) xRadius: Radius yRadius: Radius] fill];
								
								[CurrentContext restoreGraphicsState];
							}
							[MaskImage unlockFocus];
						}
						
						NSRect SrcRect = NSMakeRect(0, Size, Size, Size);
						NSRect DestRect = NSMakeRect(0, 0, Size, Size);
						NSImage* CornerImage = [[[NSImage alloc] initWithSize:DestRect.size] autorelease];
						{
							[CornerImage lockFocus];
							{
								NSGraphicsContext* CurrentContext = [NSGraphicsContext currentContext];
								[CurrentContext saveGraphicsState];
								
								[[NSColor clearColor] set];
								[[NSBezierPath bezierPathWithRect:DestRect] fill];
								
								[MaskImage drawInRect:DestRect fromRect:SrcRect operation:NSCompositeSourceOver fraction:1 respectFlipped:YES hints:nil];
								
								[CurrentContext restoreGraphicsState];
							}
							[CornerImage unlockFocus];
						}
						
						CGImageRef CGImage = [CornerImage CGImageForProposedRect:nil context:nil hints:nil];
						check(CGImage);
						NSBitmapImageRep* ImageRep = [[[NSBitmapImageRep alloc] initWithCGImage:CGImage] autorelease];
						check(ImageRep);
						
						GLenum format = [ImageRep hasAlpha] ? GL_RGBA : GL_RGB;
						glTexImage2D (GL_TEXTURE_2D, 0, format, [ImageRep size].width, [ImageRep size].height, 0, format, GL_UNSIGNED_BYTE, [ImageRep bitmapData]);
					}
					
					glUseProgram(Device->SharedContextCompositeProgram);
					
					glUniform1i(Device->SharedContextWindowTextureUniform, 0);
					
					glUniform1i(Device->SharedContextTextureDirectionUniform, 0);
					
					glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_TRUE);
					
					glViewport(0, BackbufferSizeY-Device->SharedContextCompositeTextureSizeY, Device->SharedContextCompositeTextureSizeX, Device->SharedContextCompositeTextureSizeY);
					
					glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
					
					glUniform1i(Device->SharedContextTextureDirectionUniform, 1);
					
					glViewport(BackbufferSizeX-Device->SharedContextCompositeTextureSizeX, BackbufferSizeY-Device->SharedContextCompositeTextureSizeY, Device->SharedContextCompositeTextureSizeX, Device->SharedContextCompositeTextureSizeY);
					
					glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
					
					glViewport(0, 0, BackbufferSizeX, BackbufferSizeY);
					
					glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
				}

				uint32 IdleStart = FPlatformTime::Cycles();

				int32 RealSyncInterval = bLockToVsync ? SyncInterval : 0;

				if (Context->SyncInterval != RealSyncInterval)
				{
					[Context->OpenGLContext setValues: &RealSyncInterval forParameter: NSOpenGLCPSwapInterval];
					Context->SyncInterval = RealSyncInterval;
				}

				[Context->OpenGLContext flushBuffer];
				REPORT_GL_END_BUFFER_EVENT_FOR_FRAME_DUMP();
//				INITIATE_GL_FRAME_DUMP_EVERY_X_CALLS( 1000 );
				
				if(SlateCocoaWindow)
				{
					// Using dispatch was safer - but during loading we aren't responsive on the main thread
					// this makes it impossible to open the window & so it never appears to play the loading screen.
					// Just hope that this will work OK...
					[SlateCocoaWindow performDeferredOrderFront];
				}
				
				GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUPresent] += FPlatformTime::Cycles() - IdleStart;
				GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUPresent]++;
			}
		}
	}
}

void PlatformRenderingContextSetup(FPlatformOpenGLDevice* Device)
{
	check(Device);

	SCOPED_AUTORELEASE_POOL;

	if ([NSOpenGLContext currentContext])
	{
		glFlush();
	}
	[Device->RenderingContext makeCurrentContext];
}

void PlatformSharedContextSetup(FPlatformOpenGLDevice* Device)
{
	check(Device);

	SCOPED_AUTORELEASE_POOL;

	if ([NSOpenGLContext currentContext])
	{
		glFlush();
	}
	[Device->SharedContext makeCurrentContext];
}

void PlatformNULLContextSetup()
{
	SCOPED_AUTORELEASE_POOL;
	if ([NSOpenGLContext currentContext])
	{
		glFlush();
	}
	[NSOpenGLContext clearCurrentContext];
}

EOpenGLCurrentContext PlatformOpenGLCurrentContext(FPlatformOpenGLDevice* Device)
{
	SCOPED_AUTORELEASE_POOL;

	NSOpenGLContext* Context = [NSOpenGLContext currentContext];

	if (Context == Device->RenderingContext)	// most common case
	{
		return CONTEXT_Rendering;
	}
	else if (Context == Device->SharedContext)
	{
		return CONTEXT_Shared;
	}
	else if (Context)
	{
		return CONTEXT_Other;
	}
	else
	{
		return CONTEXT_Invalid;
	}
}

void PlatformFlushIfNeeded()
{
}

void PlatformRebindResources(FPlatformOpenGLDevice* Device)
{
	// @todo: Figure out if we need to rebind frame & renderbuffers after switching contexts
}

void PlatformResizeGLContext( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 SizeX, uint32 SizeY, bool bFullscreen, bool bWasFullscreen, GLenum BackBufferTarget, GLuint BackBufferResource)
{
	FScopeLock ScopeLock(Device->ContextUsageGuard);
	{
		FScopeContext ScopeContext(Context->OpenGLContext);

		SCOPED_AUTORELEASE_POOL;
		
		// Cache & clear the drawable view before resizing the context
		// otherwise backstore size changes won't be respected.
		NSView* View = [Context->OpenGLContext view];
		[Context->OpenGLContext clearDrawable];
		
		// Resize the context to the desired resolution directly rather than
		// trusting the OS. We must do this because of our subversion of AppKit
		// message handling. If we don't do this ourselves then the OS will not
		// resize the backing store when the window size changes to adjust the resolution
		// when exiting fullscreen. This fails despite the transition from fullscreen
		// having completed according to the notifications.
		GLint dim[2];
		dim[0] = SizeX;
		dim[1] = SizeY;
		CGLError Err = CGLSetParameter((CGLContextObj)[Context->OpenGLContext CGLContextObj], kCGLCPSurfaceBackingSize, &dim[0]);
		Err = CGLEnable((CGLContextObj)[Context->OpenGLContext CGLContextObj], kCGLCESurfaceBackingSize);
		
		// Restore the drawable view to make it display again.
		[Context->OpenGLContext setView: View];
		
		[Context->OpenGLContext update];

		if (Context->ViewportFramebuffer == 0)
		{
			glGenFramebuffers(1, &Context->ViewportFramebuffer);
			check(Context->ViewportFramebuffer);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, Context->ViewportFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, BackBufferTarget, BackBufferResource, 0);
#if UE_BUILD_DEBUG
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		GLenum CompleteResult = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (CompleteResult != GL_FRAMEBUFFER_COMPLETE)
		{
			UE_LOG(LogRHI, Fatal,TEXT("Framebuffer not complete. Status = 0x%x"), CompleteResult);
		}
#endif

		// Clear new buffer to black
		glViewport(0, 0, SizeX, SizeY);
		static GLfloat ZeroColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glClearBufferfv(GL_COLOR, 0, ZeroColor );

		// Set up the state for framebuffer blit
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, Context->ViewportFramebuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	}
}

void PlatformGetSupportedResolution(uint32 &Width, uint32 &Height)
{
	uint32 InitializedMode = false;
	uint32 BestWidth = 0;
	uint32 BestHeight = 0;

	CFArrayRef AllModes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, NULL);
	if (AllModes)
	{
		int32 NumModes = CFArrayGetCount(AllModes);
		for (int32 Index = 0; Index < NumModes; Index++)
		{
			CGDisplayModeRef Mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(AllModes, Index);
			int32 ModeWidth = (int32)CGDisplayModeGetWidth(Mode);
			int32 ModeHeight = (int32)CGDisplayModeGetHeight(Mode);

			bool IsEqualOrBetterWidth = FMath::Abs((int32)ModeWidth - (int32)Width) <= FMath::Abs((int32)BestWidth - (int32)Width);
			bool IsEqualOrBetterHeight = FMath::Abs((int32)ModeHeight - (int32)Height) <= FMath::Abs((int32)BestHeight - (int32)Height);
			if(!InitializedMode || (IsEqualOrBetterWidth && IsEqualOrBetterHeight))
			{
				BestWidth = ModeWidth;
				BestHeight = ModeHeight;
				InitializedMode = true;
			}
		}
		CFRelease(AllModes);
	}
	check(InitializedMode);
	Width = BestWidth;
	Height = BestHeight;
}

bool PlatformGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	int32 MinAllowableResolutionX = 0;
	int32 MinAllowableResolutionY = 0;
	int32 MaxAllowableResolutionX = 10480;
	int32 MaxAllowableResolutionY = 10480;
	int32 MinAllowableRefreshRate = 0;
	int32 MaxAllowableRefreshRate = 10480;

	if (MaxAllowableResolutionX == 0)
	{
		MaxAllowableResolutionX = 10480;
	}
	if (MaxAllowableResolutionY == 0)
	{
		MaxAllowableResolutionY = 10480;
	}
	if (MaxAllowableRefreshRate == 0)
	{
		MaxAllowableRefreshRate = 10480;
	}

	CFArrayRef AllModes = CGDisplayCopyAllDisplayModes(kCGDirectMainDisplay, NULL);
	if (AllModes)
	{
		int32 NumModes = CFArrayGetCount(AllModes);
		for (int32 Index = 0; Index < NumModes; Index++)
		{
			CGDisplayModeRef Mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(AllModes, Index);
			SIZE_T Width = CGDisplayModeGetWidth(Mode);
			SIZE_T Height = CGDisplayModeGetHeight(Mode);
			int32 RefreshRate = (int32)CGDisplayModeGetRefreshRate(Mode);

			if (((int32)Width >= MinAllowableResolutionX) &&
				((int32)Width <= MaxAllowableResolutionX) &&
				((int32)Height >= MinAllowableResolutionY) &&
				((int32)Height <= MaxAllowableResolutionY)
				)
			{
				bool bAddIt = true;
				if (bIgnoreRefreshRate == false)
				{
					if (RefreshRate < MinAllowableRefreshRate || RefreshRate > MaxAllowableRefreshRate)
					{
						continue;
					}
				}
				else
				{
					// See if it is in the list already
					for (int32 CheckIndex = 0; CheckIndex < Resolutions.Num(); CheckIndex++)
					{
						FScreenResolutionRHI& CheckResolution = Resolutions[CheckIndex];
						if ((CheckResolution.Width == Width) &&
							(CheckResolution.Height == Height))
						{
							// Already in the list...
							bAddIt = false;
							break;
						}
					}
				}

				if (bAddIt)
				{
					// Add the mode to the list
					int32 Temp2Index = Resolutions.AddZeroed();
					FScreenResolutionRHI& ScreenResolution = Resolutions[Temp2Index];

					ScreenResolution.Width = Width;
					ScreenResolution.Height = Height;
					ScreenResolution.RefreshRate = RefreshRate;
				}
			}
		}

		CFRelease(AllModes);
	}

	return true;
}

static CGDisplayModeRef GDesktopDisplayMode = NULL;

void PlatformRestoreDesktopDisplayMode()
{
	if (GDesktopDisplayMode)
	{
		CGDisplaySetDisplayMode(kCGDirectMainDisplay, GDesktopDisplayMode, NULL);
		CGDisplayModeRelease(GDesktopDisplayMode);
		GDesktopDisplayMode = NULL;
	}
}

bool PlatformInitOpenGL()
{
	if (!GDesktopDisplayMode)
	{
		GDesktopDisplayMode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
	}
	return true;
}

bool PlatformOpenGLContextValid()
{
	return( [NSOpenGLContext currentContext] != NULL );
}

int32 PlatformGlGetError()
{
	return glGetError();
}

void PlatformGetBackbufferDimensions( uint32& OutWidth, uint32& OutHeight )
{
	SCOPED_AUTORELEASE_POOL;

	OutWidth = OutHeight = 0;
	NSOpenGLContext* Context = [NSOpenGLContext currentContext];
	if( Context )
	{
		NSView* View = [Context view];
		if( View )
		{
			// I assume here that kCGLCESurfaceBackingSize is not used
			NSRect Frame = [View frame];
			OutWidth = (uint32)Frame.size.width;
			OutHeight = (uint32)Frame.size.height;
		}
	}
}

// =============================================================

struct FOpenGLReleasedQuery
{
	NSOpenGLContext*	Context;
	GLuint				Query;
};

static TArray<FOpenGLReleasedQuery>	ReleasedQueries;
static FCriticalSection*			ReleasedQueriesGuard;

void PlatformGetNewRenderQuery( GLuint* OutQuery, uint64* OutQueryContext )
{
	if( !ReleasedQueriesGuard )
	{
		ReleasedQueriesGuard = new FCriticalSection;
	}

	{
		FScopeLock Lock(ReleasedQueriesGuard);

#ifdef UE_BUILD_DEBUG
		check( OutQuery && OutQueryContext );
#endif

		SCOPED_AUTORELEASE_POOL;

		NSOpenGLContext* Context = [NSOpenGLContext currentContext];
		check( Context );

		GLuint NewQuery = 0;

		// Check for possible query reuse
		const int32 ArraySize = ReleasedQueries.Num();
		for( int32 Index = 0; Index < ArraySize; ++Index )
		{
			if( ReleasedQueries[Index].Context == Context )
			{
				NewQuery = ReleasedQueries[Index].Query;
				ReleasedQueries.RemoveAtSwap(Index);
				break;
			}
		}

		if( !NewQuery )
		{
			glGenQueries( 1, &NewQuery );
		}

		*OutQuery = NewQuery;
		*OutQueryContext = (uint64)Context;
	}
}

void PlatformReleaseRenderQuery( GLuint Query, uint64 QueryContext )
{
	SCOPED_AUTORELEASE_POOL;

	NSOpenGLContext* Context = [NSOpenGLContext currentContext];
	if( (uint64)Context == QueryContext )
	{
		glDeleteQueries(1, &Query );
	}
	else
	{
		FScopeLock Lock(ReleasedQueriesGuard);
#ifdef UE_BUILD_DEBUG
		check( Query && QueryContext && ReleasedQueriesGuard );
#endif
		FOpenGLReleasedQuery ReleasedQuery;
		ReleasedQuery.Context = (NSOpenGLContext*)QueryContext;
		ReleasedQuery.Query = Query;
		ReleasedQueries.Add(ReleasedQuery);
	}
}

void DeleteQueriesForCurrentContext( NSOpenGLContext* Context )
{
	if( !ReleasedQueriesGuard )
	{
		ReleasedQueriesGuard = new FCriticalSection;
	}

	{
		FScopeLock Lock(ReleasedQueriesGuard);
		for( int32 Index = 0; Index < ReleasedQueries.Num(); ++Index )
		{
			if( ReleasedQueries[Index].Context == Context )
			{
				glDeleteQueries(1,&ReleasedQueries[Index].Query);
				ReleasedQueries.RemoveAtSwap(Index);
				--Index;
			}
		}
	}
}

bool PlatformContextIsCurrent( uint64 QueryContext )
{
	SCOPED_AUTORELEASE_POOL;
	return (uint64)[NSOpenGLContext currentContext] == QueryContext;
}

FRHITexture* PlatformCreateBuiltinBackBuffer(FOpenGLDynamicRHI* OpenGLRHI, uint32 SizeX, uint32 SizeY)
{
	return NULL;
}

bool FMacOpenGL::bSupportsTextureCubeMapArray = false;
PFNGLTEXSTORAGE2DPROC FMacOpenGL::glTexStorage2D = NULL;
PFNGLTEXSTORAGE3DPROC FMacOpenGL::glTexStorage3D = NULL;
bool FMacOpenGL::bSupportsDrawBuffersBlend = false;
PFNGLBLENDFUNCSEPARATEIARBPROC FMacOpenGL::glBlendFuncSeparatei = NULL;
PFNGLBLENDEQUATIONSEPARATEIARBPROC FMacOpenGL::glBlendEquationSeparatei = NULL;
PFNGLBLENDFUNCIARBPROC FMacOpenGL::glBlendFunci = NULL;
PFNGLBLENDEQUATIONIARBPROC FMacOpenGL::glBlendEquationi = NULL;
PFNGLLABELOBJECTEXTPROC FMacOpenGL::glLabelObjectEXT = NULL;
PFNGLPUSHGROUPMARKEREXTPROC FMacOpenGL::glPushGroupMarkerEXT = NULL;
PFNGLPOPGROUPMARKEREXTPROC FMacOpenGL::glPopGroupMarkerEXT = NULL;
bool FMacOpenGL::bSupportsTessellationShader = false;
PFNGLPATCHPARAMETERIPROC FMacOpenGL::glPatchParameteri = NULL;

void FMacOpenGL::ProcessQueryGLInt()
{
#ifndef __clang__
#define LOG_AND_GET_GL_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetIntegerv(IntEnum, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT("  ") ## TEXT(#IntEnum) ## TEXT(": %d"), Dest);*/ } while(0)
#else
#define LOG_AND_GET_GL_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetIntegerv(IntEnum, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT("  " #IntEnum ": %d"), Dest);*/ } while(0)
#endif
	
	LOG_AND_GET_GL_INT(GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, 0, MaxHullUniformComponents);
	LOG_AND_GET_GL_INT(GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, 0, MaxDomainUniformComponents);
}

void FMacOpenGL::ProcessExtensions(const FString& ExtensionsString)
{
	ProcessQueryGLInt();
	FOpenGL3::ProcessExtensions(ExtensionsString);
	
	// Not all GPUs support the new 4.1 Core Profile required for GL_ARB_texture_storage
	// Those that are stuck with 3.2 don't have this extension.
	if(ExtensionsString.Contains(TEXT("GL_ARB_texture_storage")))
	{
		glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)dlsym(RTLD_SELF, "glTexStorage2D");
		glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)dlsym(RTLD_SELF, "glTexStorage3D");
	}
	
	bSupportsTextureCubeMapArray = ExtensionsString.Contains(TEXT("GL_ARB_texture_cube_map_array"));
	
	// Not all GPUs support the new 4.1 Core Profile required for GL_ARB_draw_buffers_blend
	// Those that are stuck with 3.2 don't have this extension.
	if(ExtensionsString.Contains(TEXT("GL_ARB_draw_buffers_blend")))
	{
		bSupportsDrawBuffersBlend = true;
		glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIARBPROC)dlsym(RTLD_SELF, "glBlendFuncSeparatei");
		glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIARBPROC)dlsym(RTLD_SELF, "glBlendEquationSeparatei");
		glBlendFunci = (PFNGLBLENDFUNCIARBPROC)dlsym(RTLD_SELF, "glBlendFunci");
		glBlendEquationi = (PFNGLBLENDEQUATIONIARBPROC)dlsym(RTLD_SELF, "glBlendEquationi");
	}
	
	if(ExtensionsString.Contains(TEXT("GL_EXT_debug_label")))
	{
		glLabelObjectEXT = (PFNGLLABELOBJECTEXTPROC)dlsym(RTLD_SELF, "glLabelObjectEXT");
	}
	
	if(ExtensionsString.Contains(TEXT("GL_EXT_debug_marker")))
	{
		glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXTPROC)dlsym(RTLD_SELF, "glPushGroupMarkerEXT");
		glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXTPROC)dlsym(RTLD_SELF, "glPopGroupMarkerEXT");
	}
	
	if(ExtensionsString.Contains(TEXT("GL_ARB_tessellation_shader")))
	{
		bSupportsTessellationShader = true;
		glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)dlsym(RTLD_SELF, "glPatchParameteri");
	}
}
