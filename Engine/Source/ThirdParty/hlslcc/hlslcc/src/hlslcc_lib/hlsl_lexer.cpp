// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// hlsl_lexer.cpp: Wraps the Flex generated file for the HLSL parser

#include "ShaderCompilerCommon.h"
#if PLATFORM_WINDOWS
#define strdup _strdup
#endif
#define register
#include "hlsl_lexer.inl"
