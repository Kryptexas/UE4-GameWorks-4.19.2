// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	OpenGLVertexBuffer.cpp: OpenGL vertex buffer RHI implementation.
=============================================================================*/

#include "OpenGLDrvPrivate.h"

FVertexBufferRHIRef FOpenGLDynamicRHI::RHICreateVertexBuffer(uint32 Size,FResourceArrayInterface* ResourceArray,uint32 InUsage)
{
	VERIFY_GL_SCOPE();

	const void *Data = NULL;

	// If a resource array was provided for the resource, create the resource pre-populated
	if(ResourceArray)
	{
		check(Size == ResourceArray->GetResourceDataSize());
		Data = ResourceArray->GetResourceData();
	}

	TRefCountPtr<FOpenGLVertexBuffer> VertexBuffer = new FOpenGLVertexBuffer(0, Size, InUsage, Data);
	return VertexBuffer.GetReference();
}

void* FOpenGLDynamicRHI::RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBufferRHI,uint32 Offset,uint32 Size,EResourceLockMode LockMode)
{
	check(Size > 0);

	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(VertexBuffer,VertexBuffer);
	if( VertexBuffer->GetUsage() & BUF_ZeroStride )
	{
		check( Offset + Size <= VertexBuffer->GetSize() );
		// We assume we are only using the first elements from the VB, so we can later on read this memory and create an expanded version of this zero stride buffer
		check(Offset == 0);
		return (void*)( (uint8*)VertexBuffer->GetZeroStrideBuffer() + Offset );
	}
	else
	{
		return VertexBuffer->Lock(Offset, Size, LockMode == RLM_ReadOnly, VertexBuffer->IsDynamic());
	}
}

void FOpenGLDynamicRHI::RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBufferRHI)
{
	VERIFY_GL_SCOPE();
	DYNAMIC_CAST_OPENGLRESOURCE(VertexBuffer,VertexBuffer);
	if( !( VertexBuffer->GetUsage() & BUF_ZeroStride ) )
	{
		VertexBuffer->Unlock();
	}
}

void FOpenGLDynamicRHI::RHICopyVertexBuffer(FVertexBufferRHIParamRef SourceBufferRHI,FVertexBufferRHIParamRef DestBufferRHI)
{
	VERIFY_GL_SCOPE();
	check( FOpenGL::SupportsCopyBuffer() );
	DYNAMIC_CAST_OPENGLRESOURCE(VertexBuffer,SourceBuffer);
	DYNAMIC_CAST_OPENGLRESOURCE(VertexBuffer,DestBuffer);
	check(SourceBuffer->GetSize() == DestBuffer->GetSize());

	glBindBuffer(GL_COPY_READ_BUFFER,SourceBuffer->Resource);
	glBindBuffer(GL_COPY_WRITE_BUFFER,DestBuffer->Resource);
	FOpenGL::CopyBufferSubData(GL_COPY_READ_BUFFER,GL_COPY_WRITE_BUFFER,0,0,SourceBuffer->GetSize());
	glBindBuffer(GL_COPY_READ_BUFFER,0);
	glBindBuffer(GL_COPY_WRITE_BUFFER,0);
}
