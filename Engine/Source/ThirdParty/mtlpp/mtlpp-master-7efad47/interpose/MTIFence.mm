// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLFence.h>
#include "MTIFence.hpp"

MTLPP_BEGIN

void MTIFenceTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Orignal, NSString* Ptr)
{
	Orignal(Obj, Cmd, Ptr);
}


INTERPOSE_PROTOCOL_REGISTER(MTIFenceTrace, id<MTLFence>);

MTLPP_END
