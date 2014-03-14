// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLIndexBuffer.cpp: OpenGL Index buffer RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

FIndexBufferRHIRef FOpenGLDynamicRHI::RHICreateIndexBuffer(uint32 Stride,uint32 Size,FResourceArrayInterface* ResourceArray,uint32 InUsage)
{
	VERIFY_GL_SCOPE();

	const void *Data = NULL;

	// If a resource array was provided for the resource, create the resource pre-populated
	if(ResourceArray)
	{
		check(Size == ResourceArray->GetResourceDataSize());
		Data = ResourceArray->GetResourceData();
	}

	TRefCountPtr<FOpenGLIndexBuffer> IndexBuffer = new FOpenGLIndexBuffer(Stride, Size, InUsage, Data);
	return IndexBuffer.GetReference();
}

void* FOpenGLDynamicRHI::RHILockIndexBuffer(FIndexBufferRHIParamRef IndexBufferRHI,uint32 Offset,uint32 Size,EResourceLockMode LockMode)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(IndexBuffer,IndexBuffer);
	return IndexBuffer->Lock(Offset, Size, LockMode == RLM_ReadOnly, IndexBuffer->IsDynamic());
}

void FOpenGLDynamicRHI::RHIUnlockIndexBuffer(FIndexBufferRHIParamRef IndexBufferRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(IndexBuffer,IndexBuffer);
	IndexBuffer->Unlock();
}
