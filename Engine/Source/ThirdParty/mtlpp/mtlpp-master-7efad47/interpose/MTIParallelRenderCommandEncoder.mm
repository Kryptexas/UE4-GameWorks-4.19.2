// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLParallelRenderCommandEncoder.h>
#include "MTIParallelRenderCommandEncoder.hpp"
#include "MTIRenderCommandEncoder.hpp"

MTLPP_BEGIN


INTERPOSE_PROTOCOL_REGISTER(MTIParallelRenderCommandEncoderTrace, id<MTLParallelRenderCommandEncoder>);

INTERPOSE_DEFINITION_VOID( MTIParallelRenderCommandEncoderTrace, RenderCommandEncoder,  id <MTLRenderCommandEncoder>)
{
	return MTIRenderCommandEncoderTrace::Register(Original(Obj, Cmd));
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, SetcolorstoreactionAtindex,  void,   MTLStoreAction storeAction,NSUInteger colorAttachmentIndex)
{
	Original(Obj, Cmd, storeAction, colorAttachmentIndex);
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, Setdepthstoreaction,  void,   MTLStoreAction storeAction)
{
	Original(Obj, Cmd, storeAction);
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, Setstencilstoreaction,  void,   MTLStoreAction storeAction)
{
	Original(Obj, Cmd, storeAction);
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, SetcolorstoreactionoptionsAtindex,  void,   MTLStoreActionOptions storeActionOptions,NSUInteger colorAttachmentIndex)
{
	Original(Obj, Cmd, storeActionOptions,colorAttachmentIndex);
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, Setdepthstoreactionoptions,  void,   MTLStoreActionOptions storeActionOptions)
{
	Original(Obj, Cmd, storeActionOptions);
}

INTERPOSE_DEFINITION( MTIParallelRenderCommandEncoderTrace, Setstencilstoreactionoptions,  void,   MTLStoreActionOptions storeActionOptions)
{
	Original(Obj, Cmd, storeActionOptions);
}


MTLPP_END
