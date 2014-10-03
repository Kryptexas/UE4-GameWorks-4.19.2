// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

// glccp-parse.cpp: Wraps the Bison generated file for the C preprocessor

#include "ShaderCompilerCommon.h"

// Bison will generate this warning: error C4065: switch statement contains 'default' but no 'case' labels
#pragma warning(disable: 4065)
#include "glcpp-parse.inl"
