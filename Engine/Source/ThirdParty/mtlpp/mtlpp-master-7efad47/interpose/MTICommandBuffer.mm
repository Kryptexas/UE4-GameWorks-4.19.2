// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLCommandBuffer.h>
#include "MTICommandBuffer.hpp"
#include "MTIBlitCommandEncoder.hpp"
#include "MTIRenderCommandEncoder.hpp"
#include "MTIParallelRenderCommandEncoder.hpp"
#include "MTIComputeCommandEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTICommandBufferTrace, id<MTLCommandBuffer>);

void MTICommandBufferTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Label)
{
	Original(Obj, Cmd, Label);
}
void MTICommandBufferTrace::EnqueueImpl(id Obj, SEL Cmd, Super::EnqueueType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
void MTICommandBufferTrace::CommitImpl(id Obj, SEL Cmd, Super::CommitType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
void MTICommandBufferTrace::AddScheduledHandlerImpl(id Obj, SEL Cmd, Super::AddScheduledHandlerType::DefinedIMP Original, MTLCommandBufferHandler H)
{
	Original(Obj, Cmd, H);
}
void MTICommandBufferTrace::PresentDrawableImpl(id Obj, SEL Cmd, Super::PresentDrawableType::DefinedIMP Original, id<MTLDrawable> D)
{
	Original(Obj, Cmd, D);
}
void MTICommandBufferTrace::PresentDrawableAtTimeImpl(id Obj, SEL Cmd, Super::PresentDrawableAtTimeType::DefinedIMP Original, id<MTLDrawable> D, CFTimeInterval T)
{
	Original(Obj, Cmd, D, T);
}
void MTICommandBufferTrace::WaitUntilScheduledImpl(id Obj, SEL Cmd, Super::WaitUntilScheduledType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
void MTICommandBufferTrace::AddCompletedHandlerImpl(id Obj, SEL Cmd, Super::AddCompletedHandlerType::DefinedIMP Original, MTLCommandBufferHandler H)
{
	Original(Obj, Cmd, H);
}
void MTICommandBufferTrace::WaitUntilCompletedImpl(id Obj, SEL Cmd, Super::WaitUntilCompletedType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
id<MTLBlitCommandEncoder> MTICommandBufferTrace::BlitCommandEncoderImpl(id Obj, SEL Cmd, Super::BlitCommandEncoderType::DefinedIMP Original)
{
	return MTIBlitCommandEncoderTrace::Register(Original(Obj, Cmd));
}
id<MTLRenderCommandEncoder> MTICommandBufferTrace::RenderCommandEncoderWithDescriptorImpl(id Obj, SEL Cmd, Super::RenderCommandEncoderWithDescriptorType::DefinedIMP Original, MTLRenderPassDescriptor* D)
{
	return MTIRenderCommandEncoderTrace::Register(Original(Obj, Cmd, D));
}
id<MTLComputeCommandEncoder> MTICommandBufferTrace::ComputeCommandEncoderImpl(id Obj, SEL Cmd, Super::ComputeCommandEncoderType::DefinedIMP Original)
{
	return MTIComputeCommandEncoderTrace::Register(Original(Obj, Cmd));
}
id<MTLParallelRenderCommandEncoder> MTICommandBufferTrace::ParallelRenderCommandEncoderWithDescriptorImpl(id Obj, SEL Cmd, Super::ParallelRenderCommandEncoderWithDescriptorType::DefinedIMP Original, MTLRenderPassDescriptor* D)
{
	return MTIParallelRenderCommandEncoderTrace::Register(Original(Obj, Cmd, D));
}
void MTICommandBufferTrace::PushDebugGroupImpl(id Obj, SEL Cmd, Super::PushDebugGroupType::DefinedIMP Original, NSString* S)
{
	Original(Obj, Cmd, S);
}
void MTICommandBufferTrace::PopDebugGroupImpl(id Obj, SEL Cmd, Super::PopDebugGroupType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}
INTERPOSE_DEFINITION(MTICommandBufferTrace, PresentDrawableAfterMinimumDuration, void, id<MTLDrawable> D, CFTimeInterval I)
{
	Original(Obj, Cmd, D, I);
}


MTLPP_END
