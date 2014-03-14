// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "StandaloneRenderer.h"

// not needed with ES2
#if !PLATFORM_USES_ES2

#if PLATFORM_WINDOWS
// buffers
extern PFNGLGENBUFFERSARBPROC				glGenBuffers;				
extern PFNGLBINDBUFFERARBPROC				glBindBuffer;	
extern PFNGLBUFFERDATAARBPROC				glBufferData;	
extern PFNGLDELETEBUFFERSARBPROC			glDeleteBuffers;
extern PFNGLMAPBUFFERARBPROC				glMapBuffer;
extern PFNGLUNMAPBUFFERARBPROC				glUnmapBuffer;
extern PFNGLDRAWRANGEELEMENTSPROC			glDrawRangeElements;

// Blending / texturing
extern PFNGLBLENDEQUATIONPROC				glBlendEquation;
extern PFNGLACTIVETEXTUREARBPROC			glActiveTexture;

// shaders
extern PFNGLCREATESHADERPROC				glCreateShader;
extern PFNGLSHADERSOURCEPROC				glShaderSource;
extern PFNGLCOMPILESHADERPROC				glCompileShader;
extern PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog;
extern PFNGLCREATEPROGRAMPROC				glCreateProgram;
extern PFNGLATTACHSHADERPROC				glAttachShader;
extern PFNGLDETACHSHADERPROC				glDetachShader;
extern PFNGLLINKPROGRAMPROC					glLinkProgram;
extern PFNGLGETPROGRAMINFOLOGPROC			glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC					glUseProgram;
extern PFNGLDELETESHADERPROC				glDeleteShader;
extern PFNGLDELETEPROGRAMPROC				glDeleteProgram;
extern PFNGLGETSHADERIVPROC					glGetShaderiv;
extern PFNGLGETPROGRAMIVPROC				glGetProgramiv;
extern PFNGLGETUNIFORMLOCATIONPROC			glGetUniformLocation;
extern PFNGLUNIFORM1FPROC					glUniform1f;
extern PFNGLUNIFORM2FPROC					glUniform2f;
extern PFNGLUNIFORM3FPROC					glUniform3f;
extern PFNGLUNIFORM3FVPROC					glUniform3fv;
extern PFNGLUNIFORM4FPROC					glUniform4f;
extern PFNGLUNIFORM4FVPROC					glUniform4fv;
extern PFNGLUNIFORM1IPROC					glUniform1i;
extern PFNGLUNIFORMMATRIX4FVPROC			glUniformMatrix4fv;
extern PFNGLVERTEXATTRIBPOINTERPROC			glVertexAttribPointer;
extern PFNGLBINDATTRIBLOCATIONPROC			glBindAttribLocation;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC		glEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC	glDisableVertexAttribArray;
extern PFNGLGETACTIVEATTRIBPROC				glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC			glGetActiveUniform;
extern PFNGLGETATTACHEDSHADERSPROC			glGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC			glGetAttribLocation;

extern PFNWGLCREATECONTEXTATTRIBSARBPROC	wglCreateContextAttribsARB;

#endif

void LoadOpenGLExtensions();

#endif //!PLATFORM_IOS