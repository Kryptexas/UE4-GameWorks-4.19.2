// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLStructuredBuffer.cpp: OpenGL Index buffer RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

FStructuredBufferRHIRef FOpenGLDynamicRHI::RHICreateStructuredBuffer(uint32 Stride,uint32 Size,FResourceArrayInterface* ResourceArray,uint32 InUsage)
{
	VERIFY_GL_SCOPE();

	const void *Data = NULL;

	// If a resource array was provided for the resource, create the resource pre-populated
	if(ResourceArray)
	{
		check(Size == ResourceArray->GetResourceDataSize());
		Data = ResourceArray->GetResourceData();
	}

	TRefCountPtr<FOpenGLStructuredBuffer> StructuredBuffer = new FOpenGLStructuredBuffer(Stride, Size, InUsage & BUF_AnyDynamic, Data);
	return StructuredBuffer.GetReference();
}

void* FOpenGLDynamicRHI::RHILockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBufferRHI,uint32 Offset,uint32 Size,EResourceLockMode LockMode)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(StructuredBuffer,StructuredBuffer);
	return StructuredBuffer->Lock(Offset, Size, LockMode == RLM_ReadOnly, StructuredBuffer->IsDynamic());
}

void FOpenGLDynamicRHI::RHIUnlockStructuredBuffer(FStructuredBufferRHIParamRef StructuredBufferRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(StructuredBuffer,StructuredBuffer);
	StructuredBuffer->Unlock();
}
