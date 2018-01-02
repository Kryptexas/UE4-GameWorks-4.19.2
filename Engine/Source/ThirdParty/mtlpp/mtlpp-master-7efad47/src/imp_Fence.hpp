// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_Fence_hpp
#define imp_Fence_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<id<MTLFence>, void> : public IMPTableState<id<MTLFence>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLFence>>(C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLFence>, setLabel:, SetLabel, void, NSString*);
};

template<typename InterposeClass>
struct IMPTable<id<MTLFence>, InterposeClass> : public IMPTable<id<MTLFence>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLFence>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLFence>>::RegisterInterpose<InterposeClass>(C);
		INTERPOSE_REGISTRATION(SetLabel, C);
	}
};

MTLPP_END

#endif /* imp_Fence_hpp */
