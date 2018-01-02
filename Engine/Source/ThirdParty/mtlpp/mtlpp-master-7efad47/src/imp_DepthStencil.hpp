// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_DepthStencil_hpp
#define imp_DepthStencil_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<id<MTLDepthStencilState>, void> : public IMPTableState<id<MTLDepthStencilState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLDepthStencilState>>(C)
	{
	}
};

template<typename InterposeClass>
struct IMPTable<id<MTLDepthStencilState>, InterposeClass> : public IMPTable<id<MTLDepthStencilState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLDepthStencilState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLDepthStencilState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_DepthStencil_hpp */
