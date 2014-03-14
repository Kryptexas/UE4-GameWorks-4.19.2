// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLDevice.cpp: OpenGL device RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

#include "HardwareInfo.h"

extern GLint GMaxOpenGLColorSamples;
extern GLint GMaxOpenGLDepthSamples;
extern GLint GMaxOpenGLIntegerSamples;
extern GLint GMaxOpenGLTextureFilterAnisotropic;

/** OpenGL texture format table. */
FOpenGLTextureFormat GOpenGLTextureFormats[PF_MAX];

/** Device is necessary for vertex buffers, so they can reach the global device on destruction, and tell it to reset vertex array caches */
static FOpenGLDynamicRHI* PrivateOpenGLDevicePtr = NULL;

void OnQueryCreation( FOpenGLRenderQuery* Query )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->RegisterQuery( Query );
}

void OnQueryDeletion( FOpenGLRenderQuery* Query )
{
	if(PrivateOpenGLDevicePtr)
	{
		PrivateOpenGLDevicePtr->UnregisterQuery( Query );
	}
}

void OnQueryInvalidation( void )
{
	if(PrivateOpenGLDevicePtr)
	{
		PrivateOpenGLDevicePtr->InvalidateQueries();
	}
}

void OnProgramDeletion( GLint ProgramResource )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->OnProgramDeletion( ProgramResource );
}

void OnVertexBufferDeletion( GLuint VertexBufferResource )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->OnVertexBufferDeletion( VertexBufferResource );
}

void OnIndexBufferDeletion( GLuint IndexBufferResource )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->OnIndexBufferDeletion( IndexBufferResource );
}

void OnPixelBufferDeletion( GLuint PixelBufferResource )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->OnPixelBufferDeletion( PixelBufferResource );
}

void OnUniformBufferDeletion( GLuint UniformBufferResource, uint32 AllocatedSize, bool bStreamDraw )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->OnUniformBufferDeletion( UniformBufferResource, AllocatedSize, bStreamDraw );
}

void CachedBindArrayBuffer( GLuint Buffer )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->CachedBindArrayBuffer(PrivateOpenGLDevicePtr->GetContextStateForCurrentContext(),Buffer);
}

void CachedBindElementArrayBuffer( GLuint Buffer )
{
	check(PrivateOpenGLDevicePtr);
	PrivateOpenGLDevicePtr->CachedBindElementArrayBuffer(PrivateOpenGLDevicePtr->GetContextStateForCurrentContext(),Buffer);
}

void CachedBindPixelUnpackBuffer( GLuint Buffer )
{
	check(PrivateOpenGLDevicePtr);
	if ( FOpenGL::SupportsPixelBuffers() )
	{
		PrivateOpenGLDevicePtr->CachedBindPixelUnpackBuffer(PrivateOpenGLDevicePtr->GetContextStateForCurrentContext(),Buffer);
	}
}

void CachedBindUniformBuffer( GLuint Buffer )
{
	check(PrivateOpenGLDevicePtr);
	if (FOpenGL::SupportsUniformBuffers())
	{
		PrivateOpenGLDevicePtr->CachedBindUniformBuffer(PrivateOpenGLDevicePtr->GetContextStateForCurrentContext(),Buffer);
	}
}

bool IsUniformBufferBound( GLuint Buffer )
{
	check(PrivateOpenGLDevicePtr);
	return PrivateOpenGLDevicePtr->IsUniformBufferBound(PrivateOpenGLDevicePtr->GetContextStateForCurrentContext(),Buffer);
}

extern void BeginFrame_UniformBufferPoolCleanup();

FOpenGLContextState& FOpenGLDynamicRHI::GetContextStateForCurrentContext()
{
	int32 ContextType = (int32)PlatformOpenGLCurrentContext(PlatformDevice);
	check(ContextType >= 0);
	if (ContextType == CONTEXT_Rendering)
	{
		return RenderingContextState;
	}
	else
	{
		return SharedContextState;
	}
}

void FOpenGLDynamicRHI::RHIBeginFrame()
{
	RHIPrivateBeginFrame();
	BeginFrame_UniformBufferPoolCleanup();
	GPUProfilingData.BeginFrame(this);

#if PLATFORM_ANDROID //adding #if since not sure if this is required for any other platform.
	//we need to differential between 0 (backbuffer) and lastcolorRT.
	FOpenGLContextState& ContextState = GetContextStateForCurrentContext();
	ContextState.LastES2ColorRT = 0xFFFFFFFF;
	ContextState.LastES2DepthRT = 0xFFFFFFFF;
	PendingState.DepthStencil = 0 ;
#endif
}

void FOpenGLDynamicRHI::RHIEndFrame()
{
	GPUProfilingData.EndFrame();
}

void FOpenGLDynamicRHI::RHIBeginScene()
{
}

void FOpenGLDynamicRHI::RHIEndScene()
{
}

bool GDisableOpenGLDebugOutput = false;

#ifdef GL_ARB_debug_output

/**
 * Map GL_DEBUG_SOURCE_*_ARB to a human-readable string.
 */
static const TCHAR* GetOpenGLDebugSourceStringARB(GLenum Source)
{
	static const TCHAR* SourceStrings[] =
	{
		TEXT("API"),
		TEXT("System"),
		TEXT("ShaderCompiler"),
		TEXT("ThirdParty"),
		TEXT("Application"),
		TEXT("Other")
	};

	if (Source >= GL_DEBUG_SOURCE_API_ARB && Source <= GL_DEBUG_SOURCE_OTHER_ARB)
	{
		return SourceStrings[Source - GL_DEBUG_SOURCE_API_ARB];
	}
	return TEXT("Unknown");
}

/**
 * Map GL_DEBUG_TYPE_*_ARB to a human-readable string.
 */
static const TCHAR* GetOpenGLDebugTypeStringARB(GLenum Type)
{
	static const TCHAR* TypeStrings[] =
	{
		TEXT("Error"),
		TEXT("Deprecated"),
		TEXT("UndefinedBehavior"),
		TEXT("Portability"),
		TEXT("Performance"),
		TEXT("Other")
	};

	if (Type >= GL_DEBUG_TYPE_ERROR_ARB && Type <= GL_DEBUG_TYPE_OTHER_ARB)
	{
		return TypeStrings[Type - GL_DEBUG_TYPE_ERROR_ARB];
	}
#ifdef GL_KHR_debug	
	{
		static const TCHAR* TypeStrings[] =
		{
			TEXT("Marker"),
			TEXT("PushGroup"),
			TEXT("PopGroup"),
		};

		if (Type >= GL_DEBUG_TYPE_MARKER && Type <= GL_DEBUG_TYPE_POP_GROUP)
		{
			return TypeStrings[Type - GL_DEBUG_TYPE_MARKER];
		}
	}
#endif
	return TEXT("Unknown");
}

/**
 * Map GL_DEBUG_SEVERITY_*_ARB to a human-readable string.
 */
static const TCHAR* GetOpenGLDebugSeverityStringARB(GLenum Severity)
{
	static const TCHAR* SeverityStrings[] =
	{
		TEXT("High"),
		TEXT("Medium"),
		TEXT("Low")
	};

	if (Severity >= GL_DEBUG_SEVERITY_HIGH_ARB && Severity <= GL_DEBUG_SEVERITY_LOW_ARB)
	{
		return SeverityStrings[Severity - GL_DEBUG_SEVERITY_HIGH_ARB];
	}
#ifdef GL_KHR_debug
	if(Severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return TEXT("Notification");
#endif
	return TEXT("Unknown");
}

/**
 * OpenGL debug message callback. Conforms to GLDEBUGPROCARB.
 */
static void APIENTRY OpenGLDebugMessageCallbackARB(
	GLenum Source,
	GLenum Type,
	GLuint Id,
	GLenum Severity,
	GLsizei Length,
	const GLchar* Message,
	GLvoid* UserParam)
{
	if (GDisableOpenGLDebugOutput)
		return;
#if !NO_LOGGING
	const TCHAR* SourceStr = GetOpenGLDebugSourceStringARB(Source);
	const TCHAR* TypeStr = GetOpenGLDebugTypeStringARB(Type);
	const TCHAR* SeverityStr = GetOpenGLDebugSeverityStringARB(Severity);

	ELogVerbosity::Type Verbosity = ELogVerbosity::Warning;
	if (Type == GL_DEBUG_TYPE_ERROR_ARB && Severity == GL_DEBUG_SEVERITY_HIGH_ARB)
	{
		Verbosity = ELogVerbosity::Fatal;
	}

	if ((Verbosity & ELogVerbosity::VerbosityMask) <= FLogCategoryLogRHI::CompileTimeVerbosity)
	{
		if (!LogRHI.IsSuppressed(Verbosity))
		{
			FMsg::Logf(__FILE__, __LINE__, LogRHI.GetCategoryName(), Verbosity,
				TEXT("[%s][%s][%s][%u] %s"),
				SourceStr,
				TypeStr,
				SeverityStr,
				Id,
				ANSI_TO_TCHAR(Message)
				);
		}
	}
#endif
}

#endif // GL_ARB_debug_output

#ifdef GL_AMD_debug_output

/**
 * Map GL_DEBUG_CATEGORY_*_AMD to a human-readable string.
 */
static const TCHAR* GetOpenGLDebugCategoryStringAMD(GLenum Category)
{
	static const TCHAR* CategoryStrings[] =
	{
		TEXT("API"),
		TEXT("System"),
		TEXT("Deprecation"),
		TEXT("UndefinedBehavior"),
		TEXT("Performance"),
		TEXT("ShaderCompiler"),
		TEXT("Application"),
		TEXT("Other")
	};

	if (Category >= GL_DEBUG_CATEGORY_API_ERROR_AMD && Category <= GL_DEBUG_CATEGORY_OTHER_AMD)
	{
		return CategoryStrings[Category - GL_DEBUG_CATEGORY_API_ERROR_AMD];
	}
	return TEXT("Unknown");
}

/**
 * Map GL_DEBUG_SEVERITY_*_AMD to a human-readable string.
 */
static const TCHAR* GetOpenGLDebugSeverityStringAMD(GLenum Severity)
{
	static const TCHAR* SeverityStrings[] =
	{
		TEXT("High"),
		TEXT("Medium"),
		TEXT("Low")
	};

	if (Severity >= GL_DEBUG_SEVERITY_HIGH_AMD && Severity <= GL_DEBUG_SEVERITY_LOW_AMD)
	{
		return SeverityStrings[Severity - GL_DEBUG_SEVERITY_HIGH_AMD];
	}
	return TEXT("Unknown");
}

/**
 * OpenGL debug message callback. Conforms to GLDEBUGPROCAMD.
 */
static void APIENTRY OpenGLDebugMessageCallbackAMD(
	GLuint Id,
	GLenum Category,
	GLenum Severity,
	GLsizei Length,
	const GLchar* Message,
	GLvoid* UserParam)
{
#if !NO_LOGGING
	const TCHAR* CategoryStr = GetOpenGLDebugCategoryStringAMD(Category);
	const TCHAR* SeverityStr = GetOpenGLDebugSeverityStringAMD(Severity);
	
	ELogVerbosity::Type Verbosity = ELogVerbosity::Warning;
	if (Severity == GL_DEBUG_SEVERITY_HIGH_AMD)
	{
		Verbosity = ELogVerbosity::Fatal;
	}

	if ((Verbosity & ELogVerbosity::VerbosityMask) <= FLogCategoryLogRHI::CompileTimeVerbosity)
	{
		if (!LogRHI.IsSuppressed(Verbosity))
		{
			FMsg::Logf(__FILE__, __LINE__, LogRHI.GetCategoryName(), Verbosity,
				TEXT("[%s][%s][%u] %s"),
				CategoryStr,
				SeverityStr,
				Id,
				ANSI_TO_TCHAR(Message)
				);
		}
	}
#endif
}

#endif // GL_AMD_debug_output

#if PLATFORM_WINDOWS
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT_ProcAddress = NULL;
#endif


static inline void SetupTextureFormat( EPixelFormat Format, const FOpenGLTextureFormat& GLFormat)
{
	GOpenGLTextureFormats[Format] = GLFormat;
	GPixelFormats[Format].Supported = (GLFormat.Format != GL_NONE && GLFormat.InternalFormat != GL_NONE);
}


void InitDebugContext() 
{
	// Set the debug output callback if the driver supports it.
	VERIFY_GL(__FUNCTION__);
	bool bDebugOutputInitialized = false;
#ifdef GL_ARB_debug_output
	if (glDebugMessageCallbackARB)
	{
		glDebugMessageCallbackARB(OpenGLDebugMessageCallbackARB, /*UserParam=*/ NULL);
		bDebugOutputInitialized = (glGetError() == GL_NO_ERROR);
	}
#endif // GL_ARB_debug_output
#ifdef GL_AMD_debug_output
	if (glDebugMessageCallbackAMD && !bDebugOutputInitialized)
	{
		glDebugMessageCallbackAMD(OpenGLDebugMessageCallbackAMD, /*UserParam=*/ NULL);
		bDebugOutputInitialized = (glGetError() == GL_NO_ERROR);
	}
#endif // GL_AMD_debug_output
	if (!bDebugOutputInitialized)
	{
		UE_LOG(LogRHI,Warning,TEXT("OpenGL debug output extension not supported!"));
	}

	// this is to suppress feeding back of the debug markers and groups to the log, since those originate in the app anyways...
#if ENABLE_OPENGL_DEBUG_GROUPS && GL_ARB_debug_output && GL_KHR_debug
	if(glDebugMessageControlARB)
	{
		glDebugMessageControlARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_MARKER, GL_DONT_CARE, 0, NULL, GL_FALSE);
		glDebugMessageControlARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0, NULL, GL_FALSE);
		glDebugMessageControlARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0, NULL, GL_FALSE);
		UE_LOG(LogRHI,Warning,TEXT("disabling reporting back of debug groups and markers to the OpenGL debug output callback"));
	}
#endif
}

/**
 * Initialize RHI capabilities for the current OpenGL context.
 */
static void InitRHICapabilitiesForGL()
{
	VERIFY_GL_SCOPE();

	GTexturePoolSize = 0;
	GPoolSizeVRAMPercentage = 0;

	// GL vendor and version information.
#ifndef __clang__
	#define LOG_GL_STRING(StringEnum) UE_LOG(LogRHI, Log, TEXT("  ") ## TEXT(#StringEnum) ## TEXT(": %s"), ANSI_TO_TCHAR((const ANSICHAR*)glGetString(StringEnum)))
#else
	#define LOG_GL_STRING(StringEnum) UE_LOG(LogRHI, Log, TEXT("  " #StringEnum ": %s"), ANSI_TO_TCHAR((const ANSICHAR*)glGetString(StringEnum)))
#endif
	UE_LOG(LogRHI, Log, TEXT("Initializing OpenGL RHI"));
	LOG_GL_STRING(GL_VENDOR);
	LOG_GL_STRING(GL_RENDERER);
	LOG_GL_STRING(GL_VERSION);
	LOG_GL_STRING(GL_SHADING_LANGUAGE_VERSION);
	#undef LOG_GL_STRING

	GRHIAdapterName = FOpenGL::GetAdapterName();

	// Log all supported extensions.
#if PLATFORM_WINDOWS
	bool bWindowsSwapControlExtensionPresent = false;
#endif
	{
		GLint ExtensionCount = 0;
		FString ExtensionsString = TEXT("");
		if ( FOpenGL::SupportsIndexedExtensions() )
		{
			glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
			for (int32 ExtensionIndex = 0; ExtensionIndex < ExtensionCount; ++ExtensionIndex)
			{
				const ANSICHAR* ExtensionString = FOpenGL::GetStringIndexed(GL_EXTENSIONS, ExtensionIndex);
#if PLATFORM_WINDOWS
				if (strcmp(ExtensionString, "WGL_EXT_swap_control") == 0)
				{
					bWindowsSwapControlExtensionPresent = true;
				}
#endif
				ExtensionsString += TEXT(" ");
				ExtensionsString += ANSI_TO_TCHAR(ExtensionString);
			}
		}
		else
		{
			const ANSICHAR* GlGetStringOutput = (const ANSICHAR*) glGetString( GL_EXTENSIONS );
			if (GlGetStringOutput)
			{
				ExtensionsString += GlGetStringOutput;
				ExtensionsString += TEXT(" ");
			}
		}

		// Log supported GL extensions
		UE_LOG(LogRHI, Log, TEXT("OpenGL Extensions:"));
		TArray<FString> GLExtensionArray;
		ExtensionsString.ParseIntoArray(&GLExtensionArray, TEXT(" "), true);
		for (int ExtIndex = 0; ExtIndex < GLExtensionArray.Num(); ExtIndex++)
		{
			UE_LOG(LogRHI, Log, TEXT("  %s"), *GLExtensionArray[ExtIndex]);
		}

		FOpenGL::ProcessExtensions(ExtensionsString);
	}

#if PLATFORM_WINDOWS
	#pragma warning(push)
	#pragma warning(disable:4191)
	if (!bWindowsSwapControlExtensionPresent)
	{
		// Disable warning C4191: 'type cast' : unsafe conversion from 'PROC' to 'XXX' while getting GL entry points.
		PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT_ProcAddress =
			(PFNWGLGETEXTENSIONSSTRINGEXTPROC) wglGetProcAddress("wglGetExtensionsStringEXT");

		if (strstr(wglGetExtensionsStringEXT_ProcAddress(), "WGL_EXT_swap_control") != NULL)
		{
			bWindowsSwapControlExtensionPresent = true;
		}
	}

	if (bWindowsSwapControlExtensionPresent)
	{
		wglSwapIntervalEXT_ProcAddress = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
	}
	#pragma warning(pop)
#endif

	// Set debug flag if context was setup with debugging
	FOpenGL::InitDebugContext();

	// Log and get various limits.
#ifndef __clang__
	#define LOG_AND_GET_GL_INT(IntEnum,Default) GLint Value_##IntEnum; if (IntEnum) {glGetIntegerv(IntEnum, &Value_##IntEnum);} else {Value_##IntEnum = Default;} UE_LOG(LogRHI, Log, TEXT("  ") ## TEXT(#IntEnum) ## TEXT(": %d"), Value_##IntEnum)
#else
	#define LOG_AND_GET_GL_INT(IntEnum,Default) GLint Value_##IntEnum; if (IntEnum) {glGetIntegerv(IntEnum, &Value_##IntEnum);} else {Value_##IntEnum = Default;} UE_LOG(LogRHI, Log, TEXT("  " #IntEnum ": %d"), Value_##IntEnum)
#endif
	LOG_AND_GET_GL_INT(GL_MAX_TEXTURE_SIZE,0);
	LOG_AND_GET_GL_INT(GL_MAX_CUBE_MAP_TEXTURE_SIZE,0);
#if GL_MAX_ARRAY_TEXTURE_LAYERS
	LOG_AND_GET_GL_INT(GL_MAX_ARRAY_TEXTURE_LAYERS,0);
#endif
#if GL_MAX_3D_TEXTURE_SIZE
	LOG_AND_GET_GL_INT(GL_MAX_3D_TEXTURE_SIZE,0);
#endif 
	LOG_AND_GET_GL_INT(GL_MAX_RENDERBUFFER_SIZE,0);
	LOG_AND_GET_GL_INT(GL_MAX_TEXTURE_IMAGE_UNITS,0);
	if (FOpenGL::SupportsDrawBuffers())
	{
		LOG_AND_GET_GL_INT(GL_MAX_DRAW_BUFFERS,1);
	}
	LOG_AND_GET_GL_INT(GL_MAX_COLOR_ATTACHMENTS,1);
	LOG_AND_GET_GL_INT(GL_MAX_SAMPLES,1);
	LOG_AND_GET_GL_INT(GL_MAX_COLOR_TEXTURE_SAMPLES,1);
	LOG_AND_GET_GL_INT(GL_MAX_DEPTH_TEXTURE_SAMPLES,1);
	LOG_AND_GET_GL_INT(GL_MAX_INTEGER_SAMPLES,1);
	LOG_AND_GET_GL_INT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,0);
	LOG_AND_GET_GL_INT(GL_MAX_VERTEX_ATTRIBS,0);
	if( FOpenGL::SupportsTextureFilterAnisotropic())
	{
		LOG_AND_GET_GL_INT(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,0);
		GMaxOpenGLTextureFilterAnisotropic = Value_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT;
	}
#undef LOG_AND_GET_GL_INT

	GMaxOpenGLColorSamples = Value_GL_MAX_COLOR_TEXTURE_SAMPLES;
	GMaxOpenGLDepthSamples = Value_GL_MAX_DEPTH_TEXTURE_SAMPLES;
	GMaxOpenGLIntegerSamples = Value_GL_MAX_INTEGER_SAMPLES;

	// Verify some assumptions.
	check(Value_GL_MAX_COLOR_ATTACHMENTS >= MaxSimultaneousRenderTargets || !FOpenGL::SupportsMultipleRenderTargets());

	// We don't check for compressed formats right now because vendors have not
	// done a great job reporting what is actually supported:
	//   OSX/NVIDIA doesn't claim to support SRGB DXT formats even though they work correctly.
	//   Windows/AMD sometimes claim to support no compressed formats even though they all work correctly.
#if 0
	// Check compressed texture formats.
	bool bSupportsCompressedTextures = true;
	{
		FString CompressedFormatsString = TEXT("  GL_COMPRESSED_TEXTURE_FORMATS:");
		TArray<GLint> CompressedFormats;
		GLint NumCompressedFormats = 0;
		glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &NumCompressedFormats);
		CompressedFormats.Empty(NumCompressedFormats);
		CompressedFormats.AddZeroed(NumCompressedFormats);
		glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, CompressedFormats.GetTypedData());

		#define CHECK_COMPRESSED_FORMAT(GLName) if (CompressedFormats.Contains(GLName)) { CompressedFormatsString += TEXT(" ") TEXT(#GLName); } else { bSupportsCompressedTextures = false; }
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT);
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT);
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
		CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
		//CHECK_COMPRESSED_FORMAT(GL_COMPRESSED_RG_RGTC2);
		#undef CHECK_COMPRESSED_FORMAT

		// ATI does not report that it supports RGTC2, but the 3.2 core profile mandates it.
		// For now assume it is supported and bring it up with ATI if it really isn't(?!)
		CompressedFormatsString += TEXT(" GL_COMPRESSED_RG_RGTC2");

		UE_LOG(LogRHI, Log, *CompressedFormatsString);
	}
	check(bSupportsCompressedTextures);
#endif

	// Set capabilities.
	// NV_MS 12/04/2012: Mac should default to 3.2 core, windows creates 3.2/4.3 context based on command line
	const GLint MajorVersion = FOpenGL::GetMajorVersion();
	const GLint MinorVersion = FOpenGL::GetMinorVersion();

	// Shader platform & RHI feature level
	GRHIFeatureLevel = FOpenGL::GetFeatureLevel();
	GRHIShaderPlatform = FOpenGL::GetShaderPlatform();

	GMaxTextureMipCount = FMath::CeilLogTwo(Value_GL_MAX_TEXTURE_SIZE) + 1;
	GMaxTextureMipCount = FMath::Min<int32>(MAX_TEXTURE_MIP_COUNT, GMaxTextureMipCount);
	GMaxTextureDimensions = Value_GL_MAX_TEXTURE_SIZE;
	GMaxCubeTextureDimensions = Value_GL_MAX_CUBE_MAP_TEXTURE_SIZE;
#if GL_MAX_ARRAY_TEXTURE_LAYERS
	GMaxTextureArrayLayers = Value_GL_MAX_ARRAY_TEXTURE_LAYERS;
#endif

	GSupportsGSRenderTargetLayerSwitchingToMips = FOpenGL::SupportsGSRenderTargetLayerSwitchingToMips();
	GSupportsRenderDepthTargetableShaderResources = true;
	GSupportsVertexTextureFetch = true;
	GSupportsRenderTargetFormat_PF_G8 = true;

	GSupportsRenderTargetFormat_PF_FloatRGBA = FOpenGL::SupportsColorBufferHalfFloat();

	GSupportsShaderFramebufferFetch = FOpenGL::SupportsShaderFramebufferFetch();
	GPixelCenterOffset = 0.0f;
	GMaxShadowDepthBufferSizeX = FMath::Min<int32>(Value_GL_MAX_RENDERBUFFER_SIZE, 4096); // Limit to the D3D11 max.
	GMaxShadowDepthBufferSizeY = FMath::Min<int32>(Value_GL_MAX_RENDERBUFFER_SIZE, 4096);
	GSupportsVertexInstancing = true;
	GHardwareHiddenSurfaceRemoval = FOpenGL::HasHardwareHiddenSurfaceRemoval();

	GShaderPlatformForFeatureLevel[ERHIFeatureLevel::ES2] = (GRHIFeatureLevel == ERHIFeatureLevel::ES2) ? GRHIShaderPlatform : SP_OPENGL_PCES2;
	GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM3] = SP_NumPlatforms;
	GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM4] = SP_OPENGL_SM4;
	GShaderPlatformForFeatureLevel[ERHIFeatureLevel::SM5] = SP_OPENGL_SM5;

	// Set to same values as in DX11, as for the time being clip space adjustment are done entirely
	// in HLSLCC-generated shader code and OpenGLDrv.
	GMinClipZ = 0.0f;
	GProjectionSignY = 1.0f;

	// Disable texture streaming on ES2 unless we have the GL_APPLE_copy_texture_levels extension
	if(GRHIFeatureLevel == ERHIFeatureLevel::ES2 && !FOpenGL::SupportsCopyTextureLevels() )
	{
		GRHISupportsTextureStreaming = false;
	}
	else
	{
		GRHISupportsTextureStreaming = true;
	}

	GVertexElementTypeSupport.SetSupported(VET_Half2, FOpenGL::SupportsVertexHalfFloat());

	for (int32 PF = 0; PF < PF_MAX; ++PF)
	{
		SetupTextureFormat(EPixelFormat(PF), FOpenGLTextureFormat());
	}

	GLenum depthFormat = FOpenGL::GetDepthFormat();

	// Initialize the platform pixel format map.						 InternalFormat						InternalFormatSRGB							Format				Type								bCompressed
	SetupTextureFormat( PF_Unknown,				FOpenGLTextureFormat());
	SetupTextureFormat( PF_A32B32G32R32F,		FOpenGLTextureFormat(GL_RGBA32F,						GL_NONE,									GL_RGBA,				GL_FLOAT,							false));
	SetupTextureFormat( PF_UYVY,				FOpenGLTextureFormat());
	SetupTextureFormat( PF_ShadowDepth,			FOpenGLTextureFormat(GL_DEPTH_COMPONENT16,				GL_NONE,									GL_DEPTH_COMPONENT,		GL_UNSIGNED_INT,					false));
	SetupTextureFormat( PF_A16B16G16R16,		FOpenGLTextureFormat(GL_RGBA16,							GL_RGBA16,									GL_RGBA,				GL_UNSIGNED_SHORT,					false));
	SetupTextureFormat( PF_D24,					FOpenGLTextureFormat(depthFormat,						GL_NONE,									GL_DEPTH_COMPONENT,		GL_UNSIGNED_INT,					false));
	SetupTextureFormat( PF_A1,					FOpenGLTextureFormat());
	SetupTextureFormat( PF_R16G16B16A16_UINT,	FOpenGLTextureFormat(GL_RGBA16UI,						GL_NONE,									GL_RGBA,				GL_UNSIGNED_SHORT,					false));
	SetupTextureFormat( PF_R16G16B16A16_SINT,	FOpenGLTextureFormat(GL_RGBA16I,						GL_NONE,									GL_RGBA,				GL_SHORT,							false));
	SetupTextureFormat( PF_R5G6B5_UNORM,		FOpenGLTextureFormat());
#if PLATFORM_DESKTOP
	SetupTextureFormat( PF_B8G8R8A8,			FOpenGLTextureFormat(GL_RGBA8,							GL_SRGB8_ALPHA8,							GL_BGRA,				GL_UNSIGNED_INT_8_8_8_8_REV,		false));
	SetupTextureFormat( PF_G8,					FOpenGLTextureFormat(GL_R8,								GL_SRGB8,									GL_RED,					GL_UNSIGNED_BYTE,					false));
	SetupTextureFormat( PF_G16,					FOpenGLTextureFormat(GL_R16,							GL_R16,										GL_RED,					GL_UNSIGNED_SHORT,					false)); // Not supported for rendering.
	SetupTextureFormat( PF_DepthStencil,		FOpenGLTextureFormat(GL_DEPTH24_STENCIL8,				GL_NONE,									GL_DEPTH_STENCIL,		GL_UNSIGNED_INT_24_8,				false));
	SetupTextureFormat( PF_R32_FLOAT,			FOpenGLTextureFormat(GL_R32F,							GL_R32F,									GL_RED,					GL_FLOAT,							false));
	SetupTextureFormat( PF_G16R16,				FOpenGLTextureFormat(GL_RG16,							GL_RG16,									GL_RG,					GL_UNSIGNED_SHORT,					false));
	SetupTextureFormat( PF_G16R16F,				FOpenGLTextureFormat(GL_RG16F,							GL_RG16F,									GL_RG,					GL_HALF_FLOAT,						false));
	SetupTextureFormat( PF_G16R16F_FILTER,		FOpenGLTextureFormat(GL_RG16F,							GL_RG16F,									GL_RG,					GL_HALF_FLOAT,						false));
	SetupTextureFormat( PF_G32R32F,				FOpenGLTextureFormat(GL_RG32F,							GL_RG32F,									GL_RG,					GL_FLOAT,							false));
	SetupTextureFormat( PF_A2B10G10R10,			FOpenGLTextureFormat(GL_RGB10_A2,						GL_RGB10_A2,								GL_RGBA,				GL_UNSIGNED_INT_2_10_10_10_REV,		false));
	SetupTextureFormat( PF_R16F,				FOpenGLTextureFormat(GL_R16F,							GL_R16F,									GL_RED,					GL_HALF_FLOAT,						false));
	SetupTextureFormat( PF_R16F_FILTER,			FOpenGLTextureFormat(GL_R16F,							GL_R16F,									GL_RED,					GL_HALF_FLOAT,						false));
	SetupTextureFormat( PF_FloatRGB,			FOpenGLTextureFormat(GL_R11F_G11F_B10F,					GL_R11F_G11F_B10F,							GL_RGB,					GL_UNSIGNED_INT_10F_11F_11F_REV,	false));
	SetupTextureFormat( PF_V8U8,				FOpenGLTextureFormat(GL_RG8_SNORM,						GL_NONE,									GL_RG,					GL_BYTE,							false));
	SetupTextureFormat( PF_R8G8,				FOpenGLTextureFormat(GL_RG8,							GL_NONE,									GL_RG,					GL_UNSIGNED_BYTE,					false));
	SetupTextureFormat( PF_BC5,					FOpenGLTextureFormat(GL_COMPRESSED_RG_RGTC2,			GL_COMPRESSED_RG_RGTC2,						GL_RG,					GL_UNSIGNED_BYTE,					true));
	SetupTextureFormat( PF_A8,					FOpenGLTextureFormat(GL_R8,								GL_NONE,									GL_RED,					GL_UNSIGNED_BYTE,					false));
	SetupTextureFormat( PF_R32_UINT,			FOpenGLTextureFormat(GL_R32UI,							GL_NONE,									GL_RED,					GL_UNSIGNED_INT,					false));
	SetupTextureFormat( PF_R32_SINT,			FOpenGLTextureFormat(GL_R32I,							GL_NONE,									GL_RED,					GL_INT,								false));
	SetupTextureFormat( PF_R16_UINT,			FOpenGLTextureFormat(GL_R16UI,							GL_NONE,									GL_RED,					GL_UNSIGNED_SHORT,					false));
	SetupTextureFormat( PF_R16_SINT,			FOpenGLTextureFormat(GL_R16I,							GL_NONE,									GL_RED,					GL_SHORT,							false));
	SetupTextureFormat( PF_R8G8B8A8,			FOpenGLTextureFormat(GL_RGBA8,							GL_SRGB8_ALPHA8,							GL_RGBA,				GL_UNSIGNED_INT_8_8_8_8_REV,		false));
	SetupTextureFormat( PF_FloatRGBA,			FOpenGLTextureFormat(GL_RGBA16F,						GL_RGBA16F,									GL_RGBA,				GL_HALF_FLOAT,						false));
	SetupTextureFormat( PF_FloatR11G11B10,		FOpenGLTextureFormat(GL_R11F_G11F_B10F,					GL_R11F_G11F_B10F,							GL_RGB,					GL_UNSIGNED_INT,					false));
#else
	GLuint BGRA8888 = FOpenGL::SupportsBGRA8888() ? GL_BGRA_EXT : GL_RGBA;
	GLuint RGBA8 = FOpenGL::SupportsRGBA8() ? GL_RGBA8_OES : GL_RGBA;
	#if PLATFORM_ANDROID
		SetupTextureFormat( PF_B8G8R8A8,			FOpenGLTextureFormat(GL_BGRA,			FOpenGL::SupportsSRGB() ? GL_SRGB_ALPHA_EXT : GL_BGRA,		BGRA8888,				GL_UNSIGNED_BYTE,					false));
	#else
		SetupTextureFormat( PF_B8G8R8A8,			FOpenGLTextureFormat(GL_RGBA,			FOpenGL::SupportsSRGB() ? GL_SRGB_ALPHA_EXT : GL_RGBA,		GL_BGRA8_EXT,			FOpenGL::SupportsSRGB() ? GL_SRGB8_ALPHA8_EXT : GL_BGRA8_EXT,		BGRA8888,				GL_UNSIGNED_BYTE,					false));
	#endif
	SetupTextureFormat( PF_R8G8B8A8,			FOpenGLTextureFormat(RGBA8,				FOpenGL::SupportsSRGB() ? GL_SRGB_ALPHA_EXT : RGBA8,		GL_RGBA8,				FOpenGL::SupportsSRGB() ? GL_SRGB8_ALPHA8_EXT : GL_RGBA8,			GL_RGBA,				GL_UNSIGNED_BYTE,					false));
	#if PLATFORM_IOS
		SetupTextureFormat( PF_G8,					FOpenGLTextureFormat(GL_LUMINANCE,		GL_LUMINANCE,		GL_LUMINANCE8_EXT,			GL_LUMINANCE8_EXT,							GL_LUMINANCE,			GL_UNSIGNED_BYTE,					false));
		SetupTextureFormat( PF_A8,					FOpenGLTextureFormat(GL_ALPHA,			GL_ALPHA,			GL_ALPHA8_EXT,				GL_ALPHA8_EXT,								GL_ALPHA,				GL_UNSIGNED_BYTE,					false));
	#else
		SetupTextureFormat( PF_G8,					FOpenGLTextureFormat(GL_LUMINANCE,		GL_LUMINANCE,		GL_LUMINANCE,				GL_LUMINANCE,								GL_LUMINANCE,			GL_UNSIGNED_BYTE,					false));
		SetupTextureFormat( PF_A8,					FOpenGLTextureFormat(GL_ALPHA,			GL_ALPHA,			GL_ALPHA,					GL_ALPHA,									GL_ALPHA,				GL_UNSIGNED_BYTE,					false));
	#endif
	if (GSupportsRenderTargetFormat_PF_FloatRGBA)
	{
		if (FOpenGL::SupportsTextureHalfFloat())
		{
#if PLATFORM_ANDROID
			SetupTextureFormat( PF_FloatRGBA,		FOpenGLTextureFormat(GL_RGBA,			GL_RGBA,			GL_RGBA16F_EXT,				GL_RGBA16F_EXT,								GL_RGBA,				GL_HALF_FLOAT_OES,					false));
#else
			SetupTextureFormat( PF_FloatRGBA,		FOpenGLTextureFormat(GL_RGBA,			GL_RGBA,			GL_RGBA,					GL_HALF_FLOAT_OES,					false));
#endif
		}
	}
	if (FOpenGL::SupportsPackedDepthStencil())
	{
		SetupTextureFormat( PF_DepthStencil,	FOpenGLTextureFormat(GL_DEPTH_STENCIL_OES,	GL_NONE,			GL_DEPTH_STENCIL_OES,		GL_UNSIGNED_INT_24_8_OES,			false));
	}
	else
	{
		// @todo android: This is cheating by not setting a stencil anywhere, need that! And Shield is still rendering black scene
		SetupTextureFormat( PF_DepthStencil,	FOpenGLTextureFormat(GL_DEPTH_COMPONENT,	GL_NONE,			GL_DEPTH_COMPONENT,			GL_UNSIGNED_INT,					false));
	}
#endif

	if ( FOpenGL::SupportsDXT() )
	{
		// WebGL does not support SRGB versions of DXTn texture formats! Run with SRGB formats disabled.  Will need to make sure
		// sRGB is always emulated if it's needed.
		if (!FOpenGL::SupportsSRGB())
		{
			SetupTextureFormat( PF_DXT1,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
			SetupTextureFormat( PF_DXT3,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
			SetupTextureFormat( PF_DXT5,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
		}
		else
		{
			SetupTextureFormat( PF_DXT1,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
			SetupTextureFormat( PF_DXT3,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
			SetupTextureFormat( PF_DXT5,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,	GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
		}
	}
	if ( FOpenGL::SupportsPVRTC() )
	{
		SetupTextureFormat( PF_PVRTC2,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,	GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,	GL_RGBA,				GL_UNSIGNED_BYTE,					true));
		SetupTextureFormat( PF_PVRTC4,			FOpenGLTextureFormat(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,	GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,	GL_RGBA,				GL_UNSIGNED_BYTE,					true));
	}
	if ( FOpenGL::SupportsATITC() )
	{
		SetupTextureFormat( PF_ATC_RGB,			FOpenGLTextureFormat(GL_ATC_RGB_AMD,						GL_ATC_RGB_AMD,							GL_RGBA,				GL_UNSIGNED_BYTE,					true));
		SetupTextureFormat( PF_ATC_RGBA_E,		FOpenGLTextureFormat(GL_ATC_RGBA_EXPLICIT_ALPHA_AMD,		GL_ATC_RGBA_EXPLICIT_ALPHA_AMD,			GL_RGBA,				GL_UNSIGNED_BYTE,					true));
		SetupTextureFormat( PF_ATC_RGBA_I,		FOpenGLTextureFormat(GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD,	GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD,		GL_RGBA,				GL_UNSIGNED_BYTE,					true));
	}
	if ( FOpenGL::SupportsETC1() )
	{
		SetupTextureFormat( PF_ETC1,			FOpenGLTextureFormat(GL_ETC1_RGB8_OES,						GL_ETC1_RGB8_OES,						GL_RGBA,				GL_UNSIGNED_BYTE,					true));
	}
#if ANDROID
	if ( FOpenGL::SupportsETC2() )
	{
		SetupTextureFormat( PF_ETC2_RGB,		FOpenGLTextureFormat(GL_COMPRESSED_RGB8_ETC2,		FOpenGL::SupportsSRGB() ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2,					GL_RGBA,	GL_UNSIGNED_BYTE,	true));
		SetupTextureFormat( PF_ETC2_RGBA,		FOpenGLTextureFormat(GL_COMPRESSED_RGBA8_ETC2_EAC,	FOpenGL::SupportsSRGB() ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC,	GL_RGBA,	GL_UNSIGNED_BYTE,	true));
	}
#endif

	// Some formats need to know how large a block is.
	GPixelFormats[ PF_DepthStencil		].BlockBytes	 = 4;
	GPixelFormats[ PF_FloatRGB			].BlockBytes	 = 4;
	GPixelFormats[ PF_FloatRGBA			].BlockBytes	 = 8;
}

FOpenGLDynamicRHI::FOpenGLDynamicRHI()
:	bRevertToSharedContextAfterDrawingViewport(false)
,	bIsRenderingContextAcquired(false)
,	PlatformDevice(NULL)
,	GPUProfilingData(this)
{
	// This should be called once at the start 
	check( IsInGameThread() );
	check( !GIsThreadedRendering );

	PlatformInitOpenGL();
	PlatformDevice = PlatformCreateOpenGLDevice();
	VERIFY_GL_SCOPE();
	InitRHICapabilitiesForGL();

	check(PlatformOpenGLCurrentContext(PlatformDevice) == CONTEXT_Shared);

	PrivateOpenGLDevicePtr = this;
}

extern void DestroyShadersAndPrograms();

FOpenGLDynamicRHI::~FOpenGLDynamicRHI()
{
	check(IsInGameThread() && IsInRenderingThread()); // require that the render thread has been shut down

	Cleanup();

	DestroyShadersAndPrograms();
	PlatformDestroyOpenGLDevice(PlatformDevice);

	PrivateOpenGLDevicePtr = NULL;
}

void FOpenGLDynamicRHI::Init()
{
	check(!GIsRHIInitialized);
	VERIFY_GL_SCOPE();

	InitializeStateResources();

	// Create a default point sampler state for internal use.
	FSamplerStateInitializerRHI PointSamplerStateParams(SF_Point,AM_Clamp,AM_Clamp,AM_Clamp);
	PointSamplerState = this->RHICreateSamplerState(PointSamplerStateParams);

	// Allocate vertex and index buffers for DrawPrimitiveUP calls.
	DynamicVertexBuffers.Init(CalcDynamicBufferSize(1));
	DynamicIndexBuffers.Init(CalcDynamicBufferSize(1));

	// Notify all initialized FRenderResources that there's a valid RHI device to create their RHI resources for now.
	for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
	{
		ResourceIt->InitDynamicRHI();
	}
	for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
	{
		ResourceIt->InitRHI();
	}

	// Flush here since we might be switching to a different context/thread for rendering
	glFlush();

	FHardwareInfo::RegisterHardwareInfo( NAME_RHI, TEXT( "OpenGL" ) );

	if(FOpenGL::SupportsSeamlessCubeMap())
	{
		PendingState.bSeamlessCubemapEnabled = true;
	}

	// Set the RHI initialized flag.
	GIsRHIInitialized = true;
}

void FOpenGLDynamicRHI::Cleanup()
{
	if(GIsRHIInitialized)
	{
		// Reset the RHI initialized flag.
		GIsRHIInitialized = false;

		GPUProfilingData.Cleanup();

		// Ask all initialized FRenderResources to release their RHI resources.
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseRHI();
		}
		for(TLinkedList<FRenderResource*>::TIterator ResourceIt(FRenderResource::GetResourceList());ResourceIt;ResourceIt.Next())
		{
			ResourceIt->ReleaseDynamicRHI();
		}
	}

	// Release dynamic vertex and index buffers.
	DynamicVertexBuffers.Cleanup();
	DynamicIndexBuffers.Cleanup();

	FreeZeroStrideBuffers();

	// Release the point sampler state.
	PointSamplerState.SafeRelease();

	extern void EmptyGLSamplerStateCache();
	EmptyGLSamplerStateCache();

	// Release zero-filled dummy uniform buffer, if it exists.
	if (PendingState.ZeroFilledDummyUniformBuffer)
	{
		glDeleteBuffers(1, &PendingState.ZeroFilledDummyUniformBuffer);
		PendingState.ZeroFilledDummyUniformBuffer = 0;
		DecrementBufferMemory(GL_UNIFORM_BUFFER, false, ZERO_FILLED_DUMMY_UNIFORM_BUFFER_SIZE);
	}

	// Release pending shader
	PendingState.BoundShaderState.SafeRelease();
	check(!IsValidRef(PendingState.BoundShaderState));

	PendingState.CleanupResources();
	SharedContextState.CleanupResources();
	RenderingContextState.CleanupResources();
}

void FOpenGLDynamicRHI::RHIFlushResources()
{
	PlatformFlushIfNeeded();
}

void FOpenGLDynamicRHI::RHIAcquireThreadOwnership()
{
	check(!bRevertToSharedContextAfterDrawingViewport);	// if this is true, then main thread is rendering using our context right now.
	PlatformRenderingContextSetup(PlatformDevice);
	PlatformRebindResources(PlatformDevice);
	bIsRenderingContextAcquired = true;
	VERIFY_GL(RHIAcquireThreadOwnership);
}

void FOpenGLDynamicRHI::RHIReleaseThreadOwnership()
{
	VERIFY_GL(RHIReleaseThreadOwnership);
	bIsRenderingContextAcquired = false;
	PlatformNULLContextSetup();
}

void FOpenGLDynamicRHI::RegisterQuery( FOpenGLRenderQuery* Query )
{
	FScopeLock Lock(&QueriesListCriticalSection);
	Queries.Add(Query);
}

void FOpenGLDynamicRHI::UnregisterQuery( FOpenGLRenderQuery* Query )
{
	FScopeLock Lock(&QueriesListCriticalSection);
	Queries.RemoveSingleSwap(Query);
}

void FOpenGLDynamicRHI::RHIAutomaticCacheFlushAfterComputeShader(bool bEnable)
{
	// Nothing to do here...
}

void FOpenGLDynamicRHI::RHIFlushComputeShaderCache()
{
	// Nothing to do here...
}

void FOpenGLDynamicRHI::InvalidateQueries( void )
{
	{
	FScopeLock Lock(&QueriesListCriticalSection);
	PendingState.RunningOcclusionQuery = 0;
	for( int32 Index = 0; Index < Queries.Num(); ++Index )
	{
		Queries[Index]->bInvalidResource = true;
	}
}

	{
		FScopeLock Lock(&TimerQueriesListCriticalSection);
		
		for( int32 Index = 0; Index < TimerQueries.Num(); ++Index )
		{
			TimerQueries[Index]->bInvalidResource = true;
		}
	}
}

bool FOpenGLDynamicRHIModule::IsSupported()
{
	return PlatformInitOpenGL();
}
