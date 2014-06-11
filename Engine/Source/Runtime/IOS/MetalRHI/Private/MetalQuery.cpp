// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalQuery.cpp: Metal query RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"

FMetalRenderQuery::FMetalRenderQuery(ERenderQueryType InQueryType)
{

}

FMetalRenderQuery::~FMetalRenderQuery()
{

}


void FMetalRenderQuery::Begin()
{
	// these only need to be 8 byte aligned (of course, a normal allocation oafter this will be 256 bytes, making large holes if we don't do all 
	// query allocations at once)
	Offset = FMetalManager::Get()->AllocateFromQueryBuffer();

	// reset the query to 0
	uint32* QueryMemory = (uint32*)((uint8*)[FMetalManager::Get()->GetQueryBuffer() contents] + Offset);
	*QueryMemory = 0;

	// remember which command buffer we are in
	CommandBufferIndex = FMetalManager::Get()->GetCommandBufferIndex();

	[FMetalManager::GetContext() setVisibilityResultMode:MTLVisibilityResultModeBoolean offset:Offset];
}

void FMetalRenderQuery::End()
{
	// switch back to non-occlusion rendering
	[FMetalManager::GetContext() setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:0];
}



FRenderQueryRHIRef FMetalDynamicRHI::RHICreateRenderQuery(ERenderQueryType QueryType)
{
	return new FMetalRenderQuery(QueryType);
}

void FMetalDynamicRHI::RHIResetRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	DYNAMIC_CAST_METGALRESOURCE(RenderQuery,Query);

//	NOT_SUPPORTED("RHIResetRenderQuery");
}

bool FMetalDynamicRHI::RHIGetRenderQueryResult(FRenderQueryRHIParamRef QueryRHI,uint64& OutNumPixels,bool bWait)
{
	DYNAMIC_CAST_METGALRESOURCE(RenderQuery, Query);

	uint32* QueryMemory = (uint32*)((uint8*)[FMetalManager::Get()->GetQueryBuffer() contents] + Query->Offset);

	// timer queries are used for Benchmarks which can stall a bit more
	double Timeout = 0.0;
	if (bWait)
	{
		Timeout = 0.5f;//(Query->QueryType == RQT_AbsoluteTime) ? 2.0 : 0.5;
	}
	if (!FMetalManager::Get()->WaitForCommandBufferComplete(Query->CommandBufferIndex, Timeout))
	{
		return false;
	}

	// at this point, we are ready to read the value!
	OutNumPixels = *QueryMemory;
	return true;
}
