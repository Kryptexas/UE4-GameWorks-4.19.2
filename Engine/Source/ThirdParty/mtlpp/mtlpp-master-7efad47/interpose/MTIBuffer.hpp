// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#ifndef MTIBuffer_hpp
#define MTIBuffer_hpp

#include "imp_Buffer.hpp"
#include "MTIResource.hpp"

MTLPP_BEGIN

struct MTIBufferTrace : public IMPTable<id<MTLBuffer>, MTIBufferTrace>, public MTIObjectTrace, public MTIResourceTrace
{
	typedef IMPTable<id<MTLBuffer>, MTIBufferTrace> Super;
	
	MTIBufferTrace()
	{
	}
	
	MTIBufferTrace(id<MTLBuffer> C)
	: IMPTable<id<MTLBuffer>, MTIBufferTrace>(object_getClass(C))
	{
	}
	
	static id<MTLBuffer> Register(id<MTLBuffer> Buffer);
	
	static void* ContentsImpl(id, SEL, Super::ContentsType::DefinedIMP);
	static void DidModifyRangeImpl(id, SEL, Super::DidModifyRangeType::DefinedIMP, NSRange);
	static id<MTLTexture> NewTextureWithDescriptorOffsetBytesPerRowImpl(id, SEL, Super::NewTextureWithDescriptorOffsetBytesPerRowType::DefinedIMP, MTLTextureDescriptor* Desc, NSUInteger Offset, NSUInteger BPR);
	
	INTERPOSE_DECLARATION(AddDebugMarkerRange, void, NSString*, NSRange);
	INTERPOSE_DECLARATION_VOID(RemoveAllDebugMarkers, void);
};

MTLPP_END

#endif /* MTIBuffer_hpp */
