// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLCommandQueue.h>
#include "MTICaptureManager.hpp"
#include "MTICaptureScope.hpp"

MTLPP_BEGIN

INTERPOSE_CLASS_REGISTER(MTICaptureManagerTrace, MTLCaptureManager);

MTICaptureManagerTrace::MTICaptureManagerTrace(Class C)
: IMPTable<MTLCaptureManager*, MTICaptureManagerTrace>(C)
{
}

INTERPOSE_DEFINITION(MTICaptureManagerTrace, newCaptureScopeWithDevice, id<MTLCaptureScope>, id<MTLDevice> D)
{
	return MTICaptureScopeTrace::Register(Original(Obj, Cmd, D));
}
INTERPOSE_DEFINITION(MTICaptureManagerTrace, newCaptureScopeWithCommandQueue, id<MTLCaptureScope>, id<MTLCommandQueue> Q)
{
	return MTICaptureScopeTrace::Register(Original(Obj, Cmd, Q));
}
INTERPOSE_DEFINITION(MTICaptureManagerTrace, startCaptureWithDevice, void, id<MTLDevice> D)
{
	Original(Obj, Cmd, D);
}
INTERPOSE_DEFINITION(MTICaptureManagerTrace, startCaptureWithCommandQueue, void, id<MTLCommandQueue> Q)
{
	Original(Obj, Cmd, Q);
}
INTERPOSE_DEFINITION(MTICaptureManagerTrace, startCaptureWithScope, void, id<MTLCaptureScope> S)
{
	Original(Obj, Cmd, S);
}
INTERPOSE_DEFINITION_VOID(MTICaptureManagerTrace, stopCapture, void)
{
	Original(Obj, Cmd);
}
INTERPOSE_DEFINITION(MTICaptureManagerTrace, SetdefaultCaptureScope, void, id<MTLCaptureScope> S)
{
	return Original(Obj, Cmd, S);
}

MTLPP_END
