// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLBuffer.h>
#include "MTIBuffer.hpp"
#include "MTITexture.hpp"

MTLPP_BEGIN

void* MTIBufferTrace::ContentsImpl(id Obj, SEL Cmd, Super::ContentsType::DefinedIMP Original)
{
	return Original(Obj, Cmd);
}

void MTIBufferTrace::DidModifyRangeImpl(id Obj, SEL Cmd, Super::DidModifyRangeType::DefinedIMP Original, NSRange Range)
{
	Original(Obj, Cmd, Range);
}

id<MTLTexture> MTIBufferTrace::NewTextureWithDescriptorOffsetBytesPerRowImpl(id Obj, SEL Cmd, Super::NewTextureWithDescriptorOffsetBytesPerRowType::DefinedIMP Original, MTLTextureDescriptor* Desc, NSUInteger Offset, NSUInteger BPR)
{
	return MTITextureTrace::Register(Original(Obj, Cmd, Desc, Offset, BPR));
}

INTERPOSE_DEFINITION(MTIBufferTrace, AddDebugMarkerRange, void, NSString* Str, NSRange R)
{
	Original(Obj, Cmd, Str, R);
}
INTERPOSE_DEFINITION_VOID(MTIBufferTrace, RemoveAllDebugMarkers, void)
{
	Original(Obj, Cmd);
}

INTERPOSE_PROTOCOL_REGISTER(MTIBufferTrace, id<MTLBuffer>);


MTLPP_END
