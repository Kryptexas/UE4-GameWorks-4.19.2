// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include <Foundation/NSObject.h>
#include "MTIObject.hpp"

MTLPP_BEGIN

void MTIObjectTrace::RetainImpl(id Object, SEL Selector, void(*RetainPtr)(id,SEL))
{
	RetainPtr(Object, Selector);
}

void MTIObjectTrace::ReleaseImpl(id Object, SEL Selector, void(*ReleasePtr)(id,SEL))
{
	ReleasePtr(Object, Selector);
}

void MTIObjectTrace::DeallocImpl(id Object, SEL Selector, void(*DeallocPtr)(id,SEL))
{
	DeallocPtr(Object, Selector);
}



MTLPP_END
