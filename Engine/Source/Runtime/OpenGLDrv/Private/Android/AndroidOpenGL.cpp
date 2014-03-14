// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OpenGLDrvPrivate.h"
#include "OpenGLES2.h"
#include "AndroidWindow.h"


PFNEGLGETSYSTEMTIMENVPROC eglGetSystemTimeNV;
PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR = NULL;
PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR = NULL;
PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR = NULL;

// Occlusion Queries
PFNGLGENQUERIESEXTPROC 					glGenQueriesEXT = NULL;
PFNGLDELETEQUERIESEXTPROC 				glDeleteQueriesEXT = NULL;
PFNGLISQUERYEXTPROC 					glIsQueryEXT = NULL;
PFNGLBEGINQUERYEXTPROC 					glBeginQueryEXT = NULL;
PFNGLENDQUERYEXTPROC 					glEndQueryEXT = NULL;
PFNGLGETQUERYIVEXTPROC 					glGetQueryivEXT = NULL;  
PFNGLGETQUERYOBJECTIVEXTPROC 			glGetQueryObjectivEXT = NULL;
PFNGLGETQUERYOBJECTUIVEXTPROC 			glGetQueryObjectuivEXT = NULL;

PFNGLQUERYCOUNTEREXTPROC				glQueryCounterEXT = NULL;
PFNGLGETQUERYOBJECTUI64VEXTPROC			glGetQueryObjectui64vEXT = NULL;

// Offscreen MSAA rendering
PFNBLITFRAMEBUFFERNVPROC				glBlitFramebufferNV = NULL;
PFNGLDISCARDFRAMEBUFFEREXTPROC			glDiscardFramebufferEXT = NULL;

PFNGLPUSHGROUPMARKEREXTPROC				glPushGroupMarkerEXT = NULL;
PFNGLPOPGROUPMARKEREXTPROC				glPopGroupMarkerEXT = NULL;
PFNGLLABELOBJECTEXTPROC					glLabelObjectEXT = NULL;
PFNGLGETOBJECTLABELEXTPROC				glGetObjectLabelEXT = NULL;

PFNGLMAPBUFFEROESPROC					glMapBufferOES = NULL;
PFNGLUNMAPBUFFEROESPROC					glUnmapBufferOES = NULL;

PFNGLTEXSTORAGE2DPROC					glTexStorage2D = NULL;

struct FPlatformOpenGLDevice
{

	void SetCurrentSharedContext();
	void SetCurrentRenderingContext();
	void SetCurrentNULLContext();

	FPlatformOpenGLDevice();
	~FPlatformOpenGLDevice();
	void Init();
	void LoadEXT();
	void Terminate();
	void ReInit();
};


FPlatformOpenGLDevice::~FPlatformOpenGLDevice()
{
	AndroidEGL::GetInstance()->DestroyBackBuffer();
	AndroidEGL::GetInstance()->Terminate();
}

FPlatformOpenGLDevice::FPlatformOpenGLDevice()
{
}

void FPlatformOpenGLDevice::Init()
{
	AndroidEGL::GetInstance()->InitSurface(false);
	PlatformRenderingContextSetup(this);
	InitDefaultGLContextState();

	PlatformSharedContextSetup(this);
	InitDefaultGLContextState();

	AndroidEGL::GetInstance()->InitBackBuffer(); //can be done only after context is made current.
	LoadEXT();
}

FPlatformOpenGLDevice* PlatformCreateOpenGLDevice()
{
	FPlatformOpenGLDevice* Device = new FPlatformOpenGLDevice();
	Device->Init();
	return Device;
}

void PlatformReleaseOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
}

void PlatformBlitToViewport( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 BackbufferSizeX, uint32 BackbufferSizeY, bool bPresent,bool bLockToVsync, int32 SyncInterval )
{
	AndroidEGL::GetInstance()->SwapBuffers();
}

void PlatformRenderingContextSetup(FPlatformOpenGLDevice* Device)
{
	Device->SetCurrentRenderingContext();
}

void PlatformFlushIfNeeded()
{
}

void PlatformFlush()
{
	glFlush();
}

void PlatformRebindResources(FPlatformOpenGLDevice* Device)
{
}

void PlatformSharedContextSetup(FPlatformOpenGLDevice* Device)
{
	Device->SetCurrentSharedContext();
}

void PlatformNULLContextSetup()
{
	AndroidEGL::GetInstance()->SetCurrentContext(EGL_NO_CONTEXT, EGL_NO_SURFACE);
}

EOpenGLCurrentContext PlatformOpenGLCurrentContext(FPlatformOpenGLDevice* Device)
{
	return (EOpenGLCurrentContext)AndroidEGL::GetInstance()->GetCurrentContextType();
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
	return AndroidEGL::GetInstance()->IsCurrentContextValid();
}

void PlatformGetBackbufferDimensions( uint32& OutWidth, uint32& OutHeight )
{
	AndroidEGL::GetInstance()->GetDimensions(OutWidth, OutHeight);
}

// =============================================================

void PlatformGetNewOcclusionQuery( GLuint* OutQuery, uint64* OutQueryContext )
{
}

bool PlatformContextIsCurrent( uint64 QueryContext )
{
	return true;
}

void FPlatformOpenGLDevice::LoadEXT()
{
	eglGetSystemTimeNV = (PFNEGLGETSYSTEMTIMENVPROC)((void*)eglGetProcAddress("eglGetSystemTimeNV"));
	eglCreateSyncKHR = (PFNEGLCREATESYNCKHRPROC)((void*)eglGetProcAddress("eglCreateSyncKHR"));
	eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHRPROC)((void*)eglGetProcAddress("eglDestroySyncKHR"));
	eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHRPROC)((void*)eglGetProcAddress("eglClientWaitSyncKHR"));
}

FPlatformOpenGLContext* PlatformCreateOpenGLContext(FPlatformOpenGLDevice* Device, void* InWindowHandle)
{
	//Assumes Device is already initialized and context already created.
	return AndroidEGL::GetInstance()->GetRenderingContext();
}

void PlatformDestroyOpenGLContext(FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context)
{
	delete Device; //created here, destroyed here, but held by RHI.
}

FRHITexture* PlatformCreateBuiltinBackBuffer(FOpenGLDynamicRHI* OpenGLRHI, uint32 SizeX, uint32 SizeY)
{
	uint32 Flags = TexCreate_RenderTargetable;
	FOpenGLTexture2D* Texture2D = new FOpenGLTexture2D(OpenGLRHI, AndroidEGL::GetInstance()->GetOnScreenColorRenderBuffer(), GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, SizeX, SizeY, 0, 1, 1, 1, PF_B8G8R8A8, false, false, Flags);
	OpenGLTextureAllocated(Texture2D, Flags);

	return Texture2D;
}

void PlatformResizeGLContext( FPlatformOpenGLDevice* Device, FPlatformOpenGLContext* Context, uint32 SizeX, uint32 SizeY, bool bFullscreen, bool bWasFullscreen, GLenum BackBufferTarget, GLuint BackBufferResource)
{
	check(Context);

	glViewport(0, 0, SizeX, SizeY);
	VERIFY_GL(glViewport);
}

void PlatformGetSupportedResolution(uint32 &Width, uint32 &Height)
{
}

bool PlatformGetAvailableResolutions(FScreenResolutionArray& Resolutions, bool bIgnoreRefreshRate)
{
	return true;
}

int32 PlatformGlGetError()
{
	return glGetError();
}

// =============================================================

void PlatformReleaseOcclusionQuery( GLuint Query, uint64 QueryContext )
{
}

void FPlatformOpenGLDevice::SetCurrentSharedContext()
{
	AndroidEGL::GetInstance()->SetCurrentSharedContext();
}

void PlatformDestroyOpenGLDevice(FPlatformOpenGLDevice* Device)
{
	delete Device;
}

void FPlatformOpenGLDevice::SetCurrentRenderingContext()
{
	AndroidEGL::GetInstance()->SetCurrentRenderingContext();
}

void PlatformLabelObjects()
{
	// @todo: Check that there is a valid id (non-zero) as LabelObject will fail otherwise
	GLuint RenderBuffer = AndroidEGL::GetInstance()->GetOnScreenColorRenderBuffer();
	if (RenderBuffer != 0)
	{
		FOpenGL::LabelObject(GL_RENDERBUFFER, RenderBuffer, "OnScreenColorRB");
	}

	GLuint FrameBuffer = AndroidEGL::GetInstance()->GetResolveFrameBuffer();
	if (FrameBuffer != 0)
	{
		FOpenGL::LabelObject(GL_FRAMEBUFFER, FrameBuffer, "ResolveFB");
	}
}

//--------------------------------

void PlatformGetNewRenderQuery( GLuint* OutQuery, uint64* OutQueryContext )
{
	GLuint NewQuery = 0;
	FOpenGL::GenQueries( 1, &NewQuery );
	*OutQuery = NewQuery;
	*OutQueryContext = 0;
}

void PlatformReleaseRenderQuery( GLuint Query, uint64 QueryContext )
{
	FOpenGL::DeleteQueries(1, &Query );
}


bool FAndroidOpenGL::bUseAdrenoHalfFloatTexStorage = false;
bool FAndroidOpenGL::bUseES30ShadingLanguage = false;

void FAndroidOpenGL::ProcessExtensions(const FString& ExtensionsString)
{
	FOpenGLES2::ProcessExtensions(ExtensionsString);

	const bool bES30Support = FString(ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_VERSION))).Contains(TEXT("OpenGL ES 3.0"));

	// Get procedures
	if (bSupportsOcclusionQueries || bSupportsDisjointTimeQueries)
	{
		glGenQueriesEXT        = (PFNGLGENQUERIESEXTPROC)       ((void*)eglGetProcAddress("glGenQueriesEXT"));
		glDeleteQueriesEXT     = (PFNGLDELETEQUERIESEXTPROC)    ((void*)eglGetProcAddress("glDeleteQueriesEXT"));
		glIsQueryEXT           = (PFNGLISQUERYEXTPROC)          ((void*)eglGetProcAddress("glIsQueryEXT"));
		glBeginQueryEXT        = (PFNGLBEGINQUERYEXTPROC)       ((void*)eglGetProcAddress("glBeginQueryEXT"));
		glEndQueryEXT          = (PFNGLENDQUERYEXTPROC)         ((void*)eglGetProcAddress("glEndQueryEXT"));
		glGetQueryivEXT        = (PFNGLGETQUERYIVEXTPROC)       ((void*)eglGetProcAddress("glGetQueryivEXT"));
		glGetQueryObjectivEXT  = (PFNGLGETQUERYOBJECTIVEXTPROC) ((void*)eglGetProcAddress("glGetQueryObjectivEXT"));
		glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXTPROC)((void*)eglGetProcAddress("glGetQueryObjectuivEXT"));
	}

	if (bSupportsDisjointTimeQueries)
	{
		glQueryCounterEXT			= (PFNGLQUERYCOUNTEREXTPROC)		((void*)eglGetProcAddress("glQueryCounterEXT"));
		glGetQueryObjectui64vEXT	= (PFNGLGETQUERYOBJECTUI64VEXTPROC)	((void*)eglGetProcAddress("glGetQueryObjectui64vEXT"));
	}

	glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXTPROC)((void*)eglGetProcAddress("glDiscardFramebufferEXT"));
	glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXTPROC)((void*)eglGetProcAddress("glPushGroupMarkerEXT"));
	glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXTPROC)((void*)eglGetProcAddress("glPopGroupMarkerEXT"));
	glLabelObjectEXT = (PFNGLLABELOBJECTEXTPROC)((void*)eglGetProcAddress("glLabelObjectEXT"));
	glGetObjectLabelEXT = (PFNGLGETOBJECTLABELEXTPROC)((void*)eglGetProcAddress("glGetObjectLabelEXT"));

	bSupportsETC2 = bES30Support;

	// Attempt to find ES 3.0 glTexStorage2D if we're on an Adreno device that supports it.
	if( FString(ANSI_TO_TCHAR((const ANSICHAR*)glGetString(GL_RENDERER))).Contains(TEXT("Adreno")) )
	{
		glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)((void*)eglGetProcAddress("glTexStorage2D"));
		if( glTexStorage2D != NULL )
		{
			bUseAdrenoHalfFloatTexStorage = true;
			if( bES30Support )
			{
				bUseES30ShadingLanguage = true;
			}
		}
		else
		{
			// need to disable GL_EXT_color_buffer_half_float support because we have no way to allocate the storage and the driver doesn't work without it.
			UE_LOG(LogRHI,Warning,TEXT("Disabling support for GL_EXT_color_buffer_half_float to avoid an Adreno driver bug"));
			bSupportsColorBufferHalfFloat = false;
		}
	}

	//@todo android: need GMSAAAllowed	 ?
	if (bSupportsNVFrameBufferBlit)
	{
		glBlitFramebufferNV = (PFNBLITFRAMEBUFFERNVPROC)((void*)eglGetProcAddress("glBlitFramebufferNV"));
	}

	glMapBufferOES = (PFNGLMAPBUFFEROESPROC)((void*)eglGetProcAddress("glMapBufferOES"));
	glUnmapBufferOES = (PFNGLUNMAPBUFFEROESPROC)((void*)eglGetProcAddress("glUnmapBufferOES"));

	//On Android, there are problems compiling shaders with textureCubeLodEXT calls in the glsl code,
	// so we set this to false to modify the glsl manually at compile-time.
	bSupportsTextureCubeLodEXT = false;
}

class FAndroidGPUInfo
{
public:
	static FAndroidGPUInfo& Get()
	{
		static FAndroidGPUInfo This;
		return This;
	}

	FString GPUFamily;
	bool bSupportsFloatingPointRenderTargets;
	TArray<FString> TargetPlatformNames;

private:
	FAndroidGPUInfo()
	{
		// this is only valid in the game thread, make sure we are initialized there before being called on other threads!
		check(IsInGameThread())

		// make sure GL is started so we can get the supported formats
		AndroidEGL* EGL = AndroidEGL::GetInstance();
		EGL->Init();
		EGL->InitSurface(true);
		EGL->SetCurrentSharedContext();

		// get extensions
		const ANSICHAR* GlGetStringOutput = (const ANSICHAR*) glGetString(GL_EXTENSIONS);

		// process them (will happen again, but that's okay)
		FAndroidOpenGL::ProcessExtensions(FString(GlGetStringOutput));

		GPUFamily = (const ANSICHAR*)glGetString(GL_RENDERER);
		check(!GPUFamily.IsEmpty());

		// highest priority is the per-texture version
		if (FAndroidOpenGL::SupportsASTC())
		{
			TargetPlatformNames.Add(TEXT("Android_ASTC"));
		}
		if (FAndroidOpenGL::SupportsDXT())
		{
			TargetPlatformNames.Add(TEXT("Android_DXT"));
		}
		if (FAndroidOpenGL::SupportsATITC())
		{
			TargetPlatformNames.Add(TEXT("Android_ATC"));
		}
		if (FAndroidOpenGL::SupportsPVRTC())
		{
			TargetPlatformNames.Add(TEXT("Android_PVRTC"));
		}
		if (FAndroidOpenGL::SupportsETC2())
		{
			TargetPlatformNames.Add(TEXT("Android_ETC2"));
		}

		// all devices support ETC
		TargetPlatformNames.Add(TEXT("Android_ETC1"));

		// finally, generic Android
		TargetPlatformNames.Add(TEXT("Android"));

		bSupportsFloatingPointRenderTargets = FAndroidOpenGL::SupportsColorBufferHalfFloat();
	}
};

FString FAndroidMisc::GetGPUFamily()
{
	return FAndroidGPUInfo::Get().GPUFamily;
}

bool FAndroidMisc::SupportsFloatingPointRenderTargets()
{
	return FAndroidGPUInfo::Get().bSupportsFloatingPointRenderTargets;
}

void FAndroidMisc::GetValidTargetPlatforms(TArray<FString>& TargetPlatformNames)
{
	TargetPlatformNames = FAndroidGPUInfo::Get().TargetPlatformNames;
}

