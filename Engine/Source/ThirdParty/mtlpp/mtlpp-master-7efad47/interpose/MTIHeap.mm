// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLHeap.h>
#include "MTIHeap.hpp"
#include "MTIBuffer.hpp"
#include "MTITexture.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIHeapTrace, id<MTLHeap>);

void MTIHeapTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Ptr)
{
	Original(Obj, Cmd, Ptr);
}
MTLPurgeableState MTIHeapTrace::SetPurgeableStateImpl(id Obj, SEL Cmd, Super::SetPurgeableStateType::DefinedIMP Original, MTLPurgeableState S)
{
	return Original(Obj, Cmd, S);
}
NSUInteger MTIHeapTrace::MaxAvailableSizeWithAlignmentImpl(id Obj, SEL Cmd, Super::MaxAvailableSizeWithAlignmentType::DefinedIMP Original, NSUInteger A)
{
	return Original(Obj, Cmd, A);
}
id<MTLBuffer> MTIHeapTrace::NewBufferWithLengthImpl(id Obj, SEL Cmd, Super::NewBufferWithLengthType::DefinedIMP Original, NSUInteger L, MTLResourceOptions O)
{
	return MTIBufferTrace::Register(Original(Obj, Cmd, L, O));
}
id<MTLTexture> MTIHeapTrace::NewTextureWithDescriptorImpl(id Obj, SEL Cmd, Super::NewTextureWithDescriptorType::DefinedIMP Original, MTLTextureDescriptor* D)
{
	return MTITextureTrace::Register(Original(Obj, Cmd, D));
}

MTLPP_END
