// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OpenGLDrvPrivate.h"
#include <SDL/SDL.h>
#if !PLATFORM_HTML5_WIN32
#include <emscripten.h>
#endif

bool FHTML5OpenGL::bCombinedDepthStencilAttachment = false;


void FHTML5OpenGL::ProcessExtensions( const FString& ExtensionsString )
{
	FOpenGLES2::ProcessQueryGLInt();
	FOpenGLBase::ProcessExtensions(ExtensionsString);

	bSupportsMapBuffer = ExtensionsString.Contains(TEXT("GL_OES_mapbuffer"));
	bSupportsDepthTexture = ExtensionsString.Contains(TEXT("GL_OES_depth_texture"));
	bSupportsOcclusionQueries = ExtensionsString.Contains(TEXT("GL_ARB_occlusion_query2")) || ExtensionsString.Contains(TEXT("GL_EXT_occlusion_query_boolean"));
	bSupportsRGBA8 = ExtensionsString.Contains(TEXT("GL_OES_rgb8_rgba8"));
	bSupportsBGRA8888 = ExtensionsString.Contains(TEXT("GL_APPLE_texture_format_BGRA8888")) || ExtensionsString.Contains(TEXT("GL_IMG_texture_format_BGRA8888")) || ExtensionsString.Contains(TEXT("GL_EXT_texture_format_BGRA8888"));
	bSupportsVertexHalfFloat = false;//ExtensionsString.Contains(TEXT("GL_OES_vertex_half_float"));
	bSupportsTextureFloat = ExtensionsString.Contains(TEXT("GL_OES_texture_float"));
	bSupportsTextureHalfFloat = ExtensionsString.Contains(TEXT("GL_OES_texture_half_float"));
	bSupportsSGRB = ExtensionsString.Contains(TEXT("GL_EXT_sRGB"));
	bSupportsColorBufferHalfFloat = ExtensionsString.Contains(TEXT("GL_EXT_color_buffer_half_float"));
	bSupportsShaderFramebufferFetch = ExtensionsString.Contains(TEXT("GL_EXT_shader_framebuffer_fetch")) || ExtensionsString.Contains(TEXT("GL_NV_shader_framebuffer_fetch"));
	// @todo ios7: SRGB support does not work with our texture format setup (ES2 docs indicate that internalFormat and format must match, but they don't at all with sRGB enabled)
	//             One possible solution us to use GLFormat.InternalFormat[bSRGB] instead of GLFormat.Format
	bSupportsSGRB = false;//ExtensionsString.Contains(TEXT("GL_EXT_sRGB"));
	bSupportsDXT = ExtensionsString.Contains(TEXT("GL_NV_texture_compression_s3tc"))  ||  
		ExtensionsString.Contains(TEXT("GL_EXT_texture_compression_s3tc")) || 
		ExtensionsString.Contains(TEXT("WEBGL_compressed_texture_s3tc"))   ||
		(ExtensionsString.Contains(TEXT("GL_EXT_texture_compression_dxt1")) &&    // ANGLE (for HTML5_Win32) exposes these 3 as discrete extensions
		ExtensionsString.Contains(TEXT("GL_ANGLE_texture_compression_dxt3")) &&
		ExtensionsString.Contains(TEXT("GL_ANGLE_texture_compression_dxt5")) );
	bSupportsPVRTC = ExtensionsString.Contains(TEXT("GL_IMG_texture_compression_pvrtc")) ;
	bSupportsATITC = ExtensionsString.Contains(TEXT("GL_ATI_texture_compression_atitc")) || ExtensionsString.Contains(TEXT("GL_AMD_compressed_ATC_texture"));
	bSupportsVertexArrayObjects = ExtensionsString.Contains(TEXT("GL_OES_vertex_array_object")) ;
	bSupportsDiscardFrameBuffer = ExtensionsString.Contains(TEXT("GL_EXT_discard_framebuffer"));
	bSupportsNVFrameBufferBlit = ExtensionsString.Contains(TEXT("GL_NV_framebuffer_blit"));
	bSupportsTextureFilterAnisotropic = ExtensionsString.Contains(TEXT("GL_EXT_texture_filter_anisotropic"));
	bSupportsShaderTextureLod = ExtensionsString.Contains(TEXT("GL_EXT_shader_texture_lod"));
    bSupportsTextureCubeLodEXT = bSupportsShaderTextureLod;

	// This never exists in WebGL (ANGLE exports this, so we want to force it to false)
	bSupportsRGBA8 = false;
	// This is not color-renderable in WebGL/ANGLE (ANGLE exposes this)
	bSupportsBGRA8888 = false;
	// ANGLE/WEBGL_depth_texture is sort of like OES_depth_texture, you just can't upload bulk data to it (via Tex*Image2D); that should be OK?
	bSupportsDepthTexture = ExtensionsString.Contains(TEXT("GL_ANGLE_depth_texture")) || ExtensionsString.Contains(TEXT("WEBGL_depth_texture"));

#if !PLATFORM_HTML5_WIN32
    // The core WebGL spec has a combined GL_DEPTH_STENCIL_ATTACHMENT, unlike the core GLES2 spec.
	bCombinedDepthStencilAttachment = true;
    // Note that WebGL always supports packed depth stencil renderbuffers (DEPTH_STENCIL renderbuffor format), but for textures
    // needs WEBGL_depth_texture (at which point it's DEPTH_STENCIL + UNSIGNED_INT_24_8)
    // @todo: if we can always create PF_DepthStencil as DEPTH_STENCIL renderbuffers, we could remove the dependency
    bSupportsPackedDepthStencil = bSupportsDepthTexture;
#else
	bCombinedDepthStencilAttachment = false;
	bSupportsPackedDepthStencil = ExtensionsString.Contains(TEXT("GL_OES_packed_depth_stencil"));
#endif

	if (!ExtensionsString.Contains(TEXT("WEBGL_depth_texture")) && 		// Catch "WEBGL_depth_texture", "MOZ_WEBGL_depth_texture" and "WEBKIT_WEBGL_depth_texture".
		!ExtensionsString.Contains(TEXT("GL_ANGLE_depth_texture")) &&	// for HTML5_WIN32 build with ANGLE
		!ExtensionsString.Contains(TEXT("GL_OES_depth_texture")))		// for a future HTML5 build without ANGLE
	{
		UE_LOG(LogRHI, Warning, TEXT("This browser does not support WEBGL_depth_texture. Rendering will not function since fallback code is not available."));
	}

	// Report shader precision
	int Range[2];
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_LOW_FLOAT, Range, &ShaderLowPrecision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, Range, &ShaderMediumPrecision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, Range, &ShaderHighPrecision);
	UE_LOG(LogRHI, Log, TEXT("Fragment shader lowp precision: %d"), ShaderLowPrecision);
	UE_LOG(LogRHI, Log, TEXT("Fragment shader mediump precision: %d"), ShaderMediumPrecision);
	UE_LOG(LogRHI, Log, TEXT("Fragment shader highp precision: %d"), ShaderHighPrecision);
}

struct FPlatformOpenGLContext
{
	SDL_Surface* Context;
	GLuint			ViewportFramebuffer;

	FPlatformOpenGLContext()
		:Context(NULL),
		ViewportFramebuffer(0)
	{
	}
};

struct FPlatformOpenGLDevice
{
	// SDL 1.2 cannot support multiple contexts. 
	FPlatformOpenGLContext* SharedContext;
	FPlatformOpenGLDevice()
	{
		SharedContext = new FPlatformOpenGLContext; 
		PlatformCreateOpenGLContext(this,NULL);
	}
};

FPlatformOpenGLDevice* PlatformCreateOpenGLDevice()
{
	return new FPlatformOpenGLDevice;
}

void PlatformReleaseOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
	check (Device); 

	glDeleteFramebuffers(1, &Device->SharedContext->ViewportFramebuffer);
	Device->SharedContext->ViewportFramebuffer = 0;

	SDL_FreeSurface(Device->SharedContext->Context);
	Device->SharedContext->Context = 0;
}

void PlatformDestroyOpenGLDevice(FPlatformOpenGLDevice* Device)
{
	PlatformReleaseOpenGLContext( Device, NULL);
}

FPlatformOpenGLContext* PlatformCreateOpenGLContext(FPlatformOpenGLDevice* Device, void* InWindowHandle)
{
	Device->SharedContext->Context = SDL_SetVideoMode(800,600, 16, SDL_OPENGL);
	return Device->SharedContext; 
}

void PlatformDestroyOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
	PlatformReleaseOpenGLContext( Device, Context);
}

void PlatformBlitToViewport( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 BackbufferSizeX, uint32 BackbufferSizeY, bool bPresent,bool bLockToVsync, int32 SyncInterval )
{
	SDL_GL_SwapBuffers();
}

void PlatformRenderingContextSetup(FPlatformOpenGLDevice* Device)
{	
}

void PlatformFlushIfNeeded()
{
}

void PlatformRebindResources(FPlatformOpenGLDevice* Device)
{
}

void PlatformSharedContextSetup(FPlatformOpenGLDevice* Device)
{
}

void PlatformNULLContextSetup()
{
	// ? 
}

EOpenGLCurrentContext PlatformOpenGLCurrentContext(FPlatformOpenGLDevice* Device)
{
	return CONTEXT_Shared; 
}

void PlatformResizeGLContext( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 SizeX, uint32 SizeY, bool bFullscreen, bool bWasFullscreen, GLenum BackBufferTarget, GLuint BackBufferResource)
{
	check(Context);
	VERIFY_GL_SCOPE();

	// we can't resize with the win32 emu right now -- we have no way of resizing the window.
#if !PLATFORM_HTML5_WIN32
	emscripten_set_canvas_size(SizeX, SizeY);
#endif

	glViewport(0, 0, SizeX, SizeY);
	//@todo-mobile Do we need to clear here?
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

}

void PlatformGetSupportedResolution(uint32 &Width, uint32 &Height)
{
	// how? 
}

bool PlatformGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	return true;
}


bool PlatformInitOpenGL()
{
	return true;
}

bool PlatformOpenGLContextValid()
{
	// Get Current Context. 
	// @todo-HTML5
	// to do get current context. 
	SDL_Surface* Current = SDL_GetVideoSurface(); 
	if ( Current == NULL )
		return false; 
	return true; 
} 

int32 PlatformGlGetError()
{
	return glGetError();
}

void PlatformGetBackbufferDimensions( uint32& OutWidth, uint32& OutHeight )
{
	// @todo-mobile
	// akhare- return current surface dimensions ?
	SDL_Surface* Current = SDL_GetVideoSurface(); 
	check( Current != NULL)
	OutWidth = Current->w; 
	OutHeight = Current->h;
}

// =============================================================

bool PlatformContextIsCurrent( uint64 QueryContext )
{
	return true;
}

FRHITexture* PlatformCreateBuiltinBackBuffer(FOpenGLDynamicRHI* OpenGLRHI, uint32 SizeX, uint32 SizeY)
{
	uint32 Flags = TexCreate_RenderTargetable;
	FOpenGLTexture2D* Texture2D = new FOpenGLTexture2D(OpenGLRHI, 0, GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, SizeX, SizeY, 0, 1, 1, 1, PF_B8G8R8A8, false, false, Flags);
	OpenGLTextureAllocated(Texture2D, Flags);

	return Texture2D;
}

void PlatformGetNewRenderQuery( GLuint* OutQuery, uint64* OutQueryContext )
{
}

void PlatformReleaseRenderQuery( GLuint Query, uint64 QueryContext )
{
}

void PlatformRestoreDesktopDisplayMode()
{
}
