// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLArgumentEncoder.h>
#include "MTIArgumentEncoder.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTIArgumentEncoderTrace, id<MTLArgumentEncoder>);

INTERPOSE_DEFINITION_VOID(MTIArgumentEncoderTrace, Device, id<MTLDevice>)
{
	return Original(Obj, Cmd);
}
INTERPOSE_DEFINITION_VOID(MTIArgumentEncoderTrace, Label, NSString*)
{
	return Original(Obj, Cmd);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, SetLabel, void, NSString* S)
{
	Original(Obj, Cmd, S);
}
INTERPOSE_DEFINITION_VOID(MTIArgumentEncoderTrace, EncodedLength, NSUInteger)
{
	return Original(Obj, Cmd );
}
INTERPOSE_DEFINITION_VOID(MTIArgumentEncoderTrace, Alignment, NSUInteger)
{
	return Original(Obj, Cmd );
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setargumentbufferoffset, void, id <MTLBuffer> b, NSUInteger o)
{
	Original(Obj, Cmd, b, o);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setargumentbufferstartoffsetarrayelement, void, id<MTLBuffer> b, NSUInteger o, NSUInteger e)
{
	Original(Obj, Cmd, b, o, e);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setbufferoffsetatindex, void,  id <MTLBuffer> b, NSUInteger o, NSUInteger i)
{
	Original(Obj, Cmd, b, o, i);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setbuffersoffsetswithrange, void, const id <MTLBuffer>  * b , const NSUInteger * o, NSRange r)
{
	Original(Obj, Cmd, b, o, r);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Settextureatindex, void,  id <MTLTexture> t , NSUInteger i)
{
	Original(Obj, Cmd, t, i);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Settextureswithrange, void, const id <MTLTexture>  * t , NSRange r)
{
	Original(Obj, Cmd, t, r);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setsamplerstateatindex, void,  id <MTLSamplerState> s , NSUInteger i)
{
	Original(Obj, Cmd, s, i);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, Setsamplerstateswithrange, void, const id <MTLSamplerState>  * s, NSRange r)
{
	Original(Obj, Cmd, s, r);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, ConstantDataAtIndex, void*, NSUInteger i)
{
	return Original(Obj, Cmd, i);
}
INTERPOSE_DEFINITION(MTIArgumentEncoderTrace, NewArgumentEncoderForBufferAtIndex, id<MTLArgumentEncoder>, NSUInteger i)
{
	return Original(Obj, Cmd, i);
}

MTLPP_END
