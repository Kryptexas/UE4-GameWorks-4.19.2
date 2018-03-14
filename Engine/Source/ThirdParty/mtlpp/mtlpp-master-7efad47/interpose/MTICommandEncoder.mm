// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLCommandEncoder.h>
#include "MTICommandEncoder.hpp"

MTLPP_BEGIN

void MTICommandEncoderTrace::SetLabelImpl(id Obj, SEL Cmd, Super::SetLabelType::DefinedIMP Original, NSString* Label)
{
	Original(Obj, Cmd, Label);
}

void MTICommandEncoderTrace::EndEncodingImpl(id Obj, SEL Cmd, Super::EndEncodingType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}

void MTICommandEncoderTrace::InsertDebugSignpostImpl(id Obj, SEL Cmd, Super::InsertDebugSignpostType::DefinedIMP Original, NSString* S)
{
	Original(Obj, Cmd, S);
}

void MTICommandEncoderTrace::PushDebugGroupImpl(id Obj, SEL Cmd, Super::PushDebugGroupType::DefinedIMP Original, NSString* S)
{
	Original(Obj, Cmd, S);
}
void MTICommandEncoderTrace::PopDebugGroupImpl(id Obj, SEL Cmd, Super::PopDebugGroupType::DefinedIMP Original)
{
	Original(Obj, Cmd);
}


MTLPP_END
