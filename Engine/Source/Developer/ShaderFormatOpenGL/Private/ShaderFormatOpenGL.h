// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

enum GLSLVersion 
{
	GLSL_150,
	GLSL_430,
	GLSL_ES2,
	GLSL_ES2_WEBGL,
	GLSL_150_ES2,	// ES2 Emulation
	GLSL_ES2_IOS,
};

extern void CompileShader_Windows_OGL(const struct FShaderCompilerInput& Input,struct FShaderCompilerOutput& Output,const class FString& WorkingDirectory, GLSLVersion Version, bool bCompileMicrocode = true);
