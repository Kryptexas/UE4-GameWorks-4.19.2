// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLCommandQueue.h>
#include "MTICommandQueue.hpp"
#include "MTICommandBuffer.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTICommandQueueTrace, id<MTLCommandQueue>);

void MTICommandQueueTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Label)
{
	Original(Obj, Cmd, Label);
}

id <MTLCommandBuffer> MTICommandQueueTrace::CommandBufferImpl(id Obj, SEL Cmd, Super::CommandBufferType::DefinedIMP Original)
{
	return MTICommandBufferTrace::Register(Original(Obj, Cmd));
}

id <MTLCommandBuffer> MTICommandQueueTrace::CommandBufferWithUnretainedReferencesImpl(id Obj, SEL Cmd, Super::CommandBufferWithUnretainedReferencesType::DefinedIMP Original)
{
	return MTICommandBufferTrace::Register(Original(Obj, Cmd));
}

void MTICommandQueueTrace::InsertDebugCaptureBoundaryImpl(id Obj, SEL Cmd, Super::InsertDebugCaptureBoundaryType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}


MTLPP_END
