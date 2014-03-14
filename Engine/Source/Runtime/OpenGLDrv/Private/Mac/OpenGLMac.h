// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>

#ifndef GL_VERSION_3_3
#define glVertexAttribDivisor		glVertexAttribDivisorARB
#endif
#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif
#ifndef GL_VERSION_4_0
#define GL_TEXTURE_CUBE_MAP_ARRAY         0x9009
/** GL_ARB_draw_buffers_blend */
typedef void (APIENTRYP PFNGLBLENDEQUATIONIARBPROC) (GLuint buf, GLenum mode);
typedef void (APIENTRYP PFNGLBLENDEQUATIONSEPARATEIARBPROC) (GLuint buf, GLenum modeRGB, GLenum modeAlpha);
typedef void (APIENTRYP PFNGLBLENDFUNCIARBPROC) (GLuint buf, GLenum src, GLenum dst);
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATEIARBPROC) (GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
/** GL_EXT_debug_label */
typedef void (APIENTRYP PFNGLLABELOBJECTEXTPROC) (GLenum type, GLuint object, GLsizei length, const GLchar *label);
typedef void (APIENTRYP PFNGLGETOBJECTLABELEXTPROC) (GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label);
/** GL_EXT_debug_marker */
typedef void (APIENTRYP PFNGLINSERTEVENTMARKEREXTPROC) (GLsizei length, const char *marker);
typedef void (APIENTRYP PFNGLPUSHGROUPMARKEREXTPROC) (GLsizei length, const char *marker);
typedef void (APIENTRYP PFNGLPOPGROUPMARKEREXTPROC) (void);
/** GL_ARB_tessellation_shader */
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
typedef void (APIENTRYP PFNGLPATCHPARAMETERIPROC) (GLenum pname, GLint value);
typedef void (APIENTRYP PFNGLPATCHPARAMETERFVPROC) (GLenum pname, const GLfloat *values);
#endif
#ifndef GL_VERSION_4_1
typedef void (APIENTRYP PFNGLTEXSTORAGE2DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRYP PFNGLTEXSTORAGE3DPROC) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
#endif

#include "OpenGL3.h"


struct FMacOpenGL : public FOpenGL3
{
	// A driver bug with Nvidia cards on mac causes rendering with GL_LayerIndex to target different cubemap faces when rendering to mip > 0 to always go to the +X cube face
	static FORCEINLINE bool SupportsGSRenderTargetLayerSwitchingToMips() { return false; }

	static FORCEINLINE void InitDebugContext() UGL_OPTIONAL_VOID

	static FORCEINLINE void LabelObject(GLenum Type, GLuint Object, const ANSICHAR* Name)
	{
		if(glLabelObjectEXT)
		{
			glLabelObjectEXT(Type, Object, 0, Name);
		}
	}

	static FORCEINLINE void PushGroupMarker(const ANSICHAR* Name)
	{
		if(glPushGroupMarkerEXT)
		{
			glPushGroupMarkerEXT(0, Name);
		}
	}

	static FORCEINLINE void PopGroupMarker()
	{
		if(glPopGroupMarkerEXT)
		{
			glPopGroupMarkerEXT();
		}
	}
	
	static FORCEINLINE void TexStorage3D(GLenum Target, GLint Levels, GLint InternalFormat, GLsizei Width, GLsizei Height, GLsizei Depth, GLenum Format, GLenum Type)
	{
		if( glTexStorage3D )
		{
			glTexStorage3D( Target, Levels, InternalFormat, Width, Height, Depth);
			glFlush(); // Workaround for radr://15553950, TTP# 315197
		}
		else
		{
			const bool bArrayTexture = Target == GL_TEXTURE_2D_ARRAY || (bSupportsTextureCubeMapArray && Target == GL_TEXTURE_CUBE_MAP_ARRAY);
			
			for(uint32 MipIndex = 0; MipIndex < uint32(Levels); MipIndex++)
			{
				glTexImage3D(
							 Target,
							 MipIndex,
							 InternalFormat,
							 FMath::Max<uint32>(1,(Width >> MipIndex)),
							 FMath::Max<uint32>(1,(Height >> MipIndex)),
							 (bArrayTexture) ? Depth : FMath::Max<uint32>(1,(Depth >> MipIndex)),
							 0,
							 Format,
							 Type,
							 NULL
							 );
			}
		}
	}
	
	static FORCEINLINE bool TexStorage2D(GLenum Target, GLint Levels, GLint InternalFormat, GLsizei Width, GLsizei Height, GLenum Format, GLenum Type, uint32 Flags)
	{
		if( glTexStorage2D )
		{
			glTexStorage2D(Target, Levels, InternalFormat, Width, Height);
			glFlush(); // Workaround for radr://15553950, TTP# 315197
			return true;
		}
		else
		{
			return false;
		}
	}

	static FORCEINLINE bool SupportsSeparateAlphaBlend()				{ return bSupportsDrawBuffersBlend; }
	
	static FORCEINLINE void BlendFuncSeparatei(GLuint Buf, GLenum SrcRGB, GLenum DstRGB, GLenum SrcAlpha, GLenum DstAlpha)
	{
		if(glBlendFuncSeparatei)
		{
			glBlendFuncSeparatei(Buf, SrcRGB, DstRGB, SrcAlpha, DstAlpha);
		}
		else
		{
			UE_LOG(LogRHI, Fatal, TEXT("OpenGL state on draw requires setting different blend operation or factors to different render targets. This is not supported on Mac OS X GL 3.2 contexts!"));
		}
	}
	static FORCEINLINE void BlendEquationSeparatei(GLuint Buf, GLenum ModeRGB, GLenum ModeAlpha)
	{
		if(glBlendEquationSeparatei)
		{
			glBlendEquationSeparatei(Buf, ModeRGB, ModeAlpha);
		}
		else
		{
			UE_LOG(LogRHI, Fatal, TEXT("OpenGL state on draw requires setting different blend operation or factors to different render targets. This is not supported on Mac OS X GL 3.2 contexts!"));
		}
	}
	static FORCEINLINE void BlendFunci(GLuint Buf, GLenum Src, GLenum Dst)
	{
		if(glBlendFunci)
		{
			glBlendFunci(Buf, Src, Dst);
		}
		else
		{
			UE_LOG(LogRHI, Fatal, TEXT("OpenGL state on draw requires setting different blend operation or factors to different render targets. This is not supported on Mac OS X GL 3.2 contexts!"));
		}
	}
	static FORCEINLINE void BlendEquationi(GLuint Buf, GLenum Mode)
	{
		if(glBlendEquationi)
		{
			glBlendEquationi(Buf, Mode);
		}
		else
		{
			UE_LOG(LogRHI, Fatal, TEXT("OpenGL state on draw requires setting different blend operation or factors to different render targets. This is not supported on Mac OS X GL 3.2 contexts!"));
		}
	}
	
	static FORCEINLINE bool SupportsTessellation()						{ return bSupportsTessellationShader; }
	
	static FORCEINLINE void PatchParameteri(GLenum Pname, GLint Value)
	{
		if(glPatchParameteri)
		{
			glPatchParameteri(Pname, Value);
		}
		else
		{
			UE_LOG(LogRHI, Fatal, TEXT("Tessellation not supported on Mac OS X GL 3.2 contexts!"));
		}
	}
	
	static FORCEINLINE bool SupportsSeamlessCubeMap()					{ return true; }
	
	static void ProcessQueryGLInt();
	static void ProcessExtensions(const FString& ExtensionsString);
	
private:
	/** GL_ARB_texture_cube_map_array */
	static bool bSupportsTextureCubeMapArray;
	/** GL_ARB_texture_storage */
	static PFNGLTEXSTORAGE2DPROC glTexStorage2D;
	static PFNGLTEXSTORAGE3DPROC glTexStorage3D;
	/** GL_ARB_draw_buffers_blend */
	static bool bSupportsDrawBuffersBlend;
	static PFNGLBLENDFUNCSEPARATEIARBPROC glBlendFuncSeparatei;
	static PFNGLBLENDEQUATIONSEPARATEIARBPROC glBlendEquationSeparatei;
	static PFNGLBLENDFUNCIARBPROC glBlendFunci;
	static PFNGLBLENDEQUATIONIARBPROC glBlendEquationi;
	/** GL_EXT_debug_label */
	static PFNGLLABELOBJECTEXTPROC glLabelObjectEXT;
	/** GL_EXT_debug_marker */
	static PFNGLPUSHGROUPMARKEREXTPROC glPushGroupMarkerEXT;
	static PFNGLPOPGROUPMARKEREXTPROC glPopGroupMarkerEXT;
	/** GL_ARB_tessellation_shader */
	static bool bSupportsTessellationShader;
	static PFNGLPATCHPARAMETERIPROC glPatchParameteri;
};


typedef FMacOpenGL FOpenGL;
