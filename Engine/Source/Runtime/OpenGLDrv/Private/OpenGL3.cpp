// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGL3.cpp: OpenGL 3.2 implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

#if OPENGL_GL3

GLsizei FOpenGL3::NextTextureName = OPENGL_NAME_CACHE_SIZE;
GLuint FOpenGL3::TextureNamesCache[OPENGL_NAME_CACHE_SIZE];
GLsizei FOpenGL3::NextBufferName= OPENGL_NAME_CACHE_SIZE;
GLuint FOpenGL3::BufferNamesCache[OPENGL_NAME_CACHE_SIZE];

GLint FOpenGL3::TimestampQueryBits = 0;
bool FOpenGL3::bDebugContext = false;

void FOpenGL3::ProcessQueryGLInt()
{
#ifndef __clang__
#define LOG_AND_GET_GL_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetIntegerv(IntEnum, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT("  ") ## TEXT(#IntEnum) ## TEXT(": %d"), Dest);*/ } while(0)
#else
#define LOG_AND_GET_GL_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetIntegerv(IntEnum, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT("  " #IntEnum ": %d"), Dest);*/ } while(0)
#endif
	LOG_AND_GET_GL_INT(GL_MAX_VERTEX_UNIFORM_COMPONENTS, 0, MaxVertexUniformComponents);
	LOG_AND_GET_GL_INT(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, 0, MaxPixelUniformComponents);
	LOG_AND_GET_GL_INT(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, 0, MaxGeometryUniformComponents);

	LOG_AND_GET_GL_INT(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, 0, MaxGeometryTextureImageUnits);
	LOG_AND_GET_GL_INT(GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, 0, MaxHullTextureImageUnits);
	LOG_AND_GET_GL_INT(GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, 0, MaxDomainTextureImageUnits);

#ifndef __clang__
#define LOG_AND_GET_GL_QUERY_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetQueryiv(IntEnum, GL_QUERY_COUNTER_BITS, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT(" GL_QUERY_COUNTER_BITS: ") ## TEXT(#IntEnum) ## TEXT(": %d"), Dest);*/ } while(0)
#else
#define LOG_AND_GET_GL_QUERY_INT(IntEnum, Default, Dest) do { if (IntEnum) {glGetQueryiv(IntEnum, GL_QUERY_COUNTER_BITS, &Dest);} else {Dest = Default;} /*FPlatformMisc::LowLevelOutputDebugStringf(TEXT(" GL_QUERY_COUNTER_BITS: " #IntEnum ": %d"), Dest);*/ } while(0)
#endif
	LOG_AND_GET_GL_QUERY_INT(GL_TIMESTAMP, 0, TimestampQueryBits);
	
#undef LOG_AND_GET_GL_QUERY_INT
#undef LOG_AND_GET_GL_INT
}

void FOpenGL3::ProcessExtensions( const FString& ExtensionsString )
{
	ProcessQueryGLInt();
	FOpenGLBase::ProcessExtensions(ExtensionsString);
}

#endif
