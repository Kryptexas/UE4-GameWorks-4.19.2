// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EmptyVertexBuffer.cpp: Empty vertex buffer RHI implementation.
=============================================================================*/

#include "EmptyRHIPrivate.h"


FEmptyVertexBuffer::FEmptyVertexBuffer(uint32 InSize, uint32 InUsage)
	: FRHIVertexBuffer(InSize, InUsage)
{
}

void* FEmptyVertexBuffer::Lock(EResourceLockMode LockMode, uint32 Size)
{
	return NULL;
}

void FEmptyVertexBuffer::Unlock()
{
	
}

FVertexBufferRHIRef FEmptyDynamicRHI::RHICreateVertexBuffer(uint32 Size, FResourceArrayInterface* ResourceArray, uint32 InUsage)
{
	// make the RHI object, which will allocate memory
	FEmptyVertexBuffer* VertexBuffer = new FEmptyVertexBuffer(Size, InUsage);

	if(ResourceArray)
	{
		check(Size == ResourceArray->GetResourceDataSize());

		// make a buffer usable by CPU
		void* Buffer = RHILockVertexBuffer(VertexBuffer, 0, Size, RLM_WriteOnly);

		// copy the contents of the given data into the buffer
		FMemory::Memcpy(Buffer, ResourceArray->GetResourceData(), Size);

		RHIUnlockVertexBuffer(VertexBuffer);

		// Discard the resource array's contents.
		ResourceArray->Discard();
	}

	return VertexBuffer;
}

void* FEmptyDynamicRHI::RHILockVertexBuffer(FVertexBufferRHIParamRef VertexBufferRHI, uint32 Offset, uint32 Size, EResourceLockMode LockMode)
{
	DYNAMIC_CAST_EMPTYRESOURCE(VertexBuffer,VertexBuffer);

	// default to vertex buffer memory
	return (uint8*)VertexBuffer->Lock(LockMode, Size) + Offset;
}

void FEmptyDynamicRHI::RHIUnlockVertexBuffer(FVertexBufferRHIParamRef VertexBufferRHI)
{
	DYNAMIC_CAST_EMPTYRESOURCE(VertexBuffer,VertexBuffer);

	VertexBuffer->Unlock();
}

void FEmptyDynamicRHI::RHICopyVertexBuffer(FVertexBufferRHIParamRef SourceBufferRHI,FVertexBufferRHIParamRef DestBufferRHI)
{

}