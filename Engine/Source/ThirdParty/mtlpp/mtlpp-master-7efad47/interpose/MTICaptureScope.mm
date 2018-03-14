// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLCaptureScope.h>
#include "MTICaptureScope.hpp"

MTLPP_BEGIN

INTERPOSE_PROTOCOL_REGISTER(MTICaptureScopeTrace, id<MTLCaptureScope>);

MTICaptureScopeTrace::MTICaptureScopeTrace(id<MTLCaptureScope> C)
: IMPTable<id<MTLCaptureScope>, MTICaptureScopeTrace>(object_getClass(C))
{
}

INTERPOSE_DEFINITION_VOID(MTICaptureScopeTrace, beginScope, void)
{
	Original(Obj, Cmd);
}
INTERPOSE_DEFINITION_VOID(MTICaptureScopeTrace, endScope, void)
{
	Original(Obj, Cmd);
}
INTERPOSE_DEFINITION(MTICaptureScopeTrace, Setlabel, void, NSString* S)
{
	Original(Obj, Cmd, S);
}

MTLPP_END
