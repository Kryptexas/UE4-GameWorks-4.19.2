// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGL4.h: Public OpenGL 4.3 definitions for non-common functionality
=============================================================================*/

#pragma once

#define OPENGL_GL4		1

#include "OpenGL3.h"


struct FOpenGL4 : public FOpenGL3
{
	static FORCEINLINE bool SupportsSeparateAlphaBlend()				{ return true; }
	static FORCEINLINE bool SupportsTessellation()						{ return true; }
	static FORCEINLINE bool SupportsComputeShaders()					{ return true; }

	// Optional

	// Required
	static FORCEINLINE void BlendFuncSeparatei(GLuint Buf, GLenum SrcRGB, GLenum DstRGB, GLenum SrcAlpha, GLenum DstAlpha)
	{
		glBlendFuncSeparatei(Buf, SrcRGB, DstRGB, SrcAlpha, DstAlpha);
	}
	static FORCEINLINE void BlendEquationSeparatei(GLuint Buf, GLenum ModeRGB, GLenum ModeAlpha)
	{
		glBlendEquationSeparatei(Buf, ModeRGB, ModeAlpha);
	}
	static FORCEINLINE void BlendFunci(GLuint Buf, GLenum Src, GLenum Dst)
	{
		glBlendFunci(Buf, Src, Dst);
	}
	static FORCEINLINE void BlendEquationi(GLuint Buf, GLenum Mode)
	{
		glBlendEquationi(Buf, Mode);
	}
	static FORCEINLINE void PatchParameteri(GLenum Pname, GLint Value)
	{
		glPatchParameteri(Pname, Value);
	}
	static FORCEINLINE void BindImageTexture(GLuint Unit, GLuint Texture, GLint Level, GLboolean Layered, GLint Layer, GLenum Access, GLenum Format)
	{
		glBindImageTexture(Unit, Texture, Level, Layered, Layer, Access, Format);
	}
	static FORCEINLINE void DispatchCompute(GLuint NumGroupsX, GLuint NumGroupsY, GLuint NumGroupsZ)
	{
		glDispatchCompute(NumGroupsX, NumGroupsY, NumGroupsZ);
	}
	static FORCEINLINE void MemoryBarrier(GLbitfield Barriers)
	{
		glMemoryBarrier(Barriers);
	}

	static void ProcessQueryGLInt();
	static void ProcessExtensions( const FString& ExtensionsString );

	static FORCEINLINE GLint GetMaxComputeTextureImageUnits() { check(MaxComputeTextureImageUnits != -1); return MaxComputeTextureImageUnits; }
	static FORCEINLINE GLint GetMaxComputeUniformComponents() { check(MaxComputeUniformComponents != -1); return MaxComputeUniformComponents; }

protected:
	static GLint MaxComputeTextureImageUnits;
	static GLint MaxComputeUniformComponents;
};
