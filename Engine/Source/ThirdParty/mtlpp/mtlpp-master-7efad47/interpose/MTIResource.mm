// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#import <Metal/MTLResource.h>
#include "MTIResource.hpp"

MTLPP_BEGIN

void MTIResourceTrace::SetLabelImpl(id Obj, SEL Cmd, MTIResourceTrace::Super::SetLabelType::DefinedIMP Orignal, NSString* Ptr)
{
	Orignal(Obj, Cmd, Ptr);
}

MTLPurgeableState MTIResourceTrace::SetPurgeableStateImpl(id Obj, SEL Cmd, MTIResourceTrace::Super::SetPurgeableStateType::DefinedIMP Orignal, MTLPurgeableState State)
{
	return Orignal(Obj, Cmd, State);
}

void MTIResourceTrace::MakeAliasableImpl(id Obj, SEL Cmd, MTIResourceTrace::Super::MakeAliasableType::DefinedIMP Orignal)
{
	Orignal(Obj, Cmd);
}

MTLPP_END

