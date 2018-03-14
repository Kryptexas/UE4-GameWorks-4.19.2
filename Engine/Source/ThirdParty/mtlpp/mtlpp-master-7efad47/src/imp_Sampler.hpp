// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_Sampler_hpp
#define imp_Sampler_hpp

#include "imp_State.hpp"

MTLPP_BEGIN

template<>
struct IMPTable<id<MTLSamplerState>, void> : public IMPTableState<id<MTLSamplerState>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableState<id<MTLSamplerState>>(C)
	{
	}
};

template<typename InterposeClass>
struct IMPTable<id<MTLSamplerState>, InterposeClass> : public IMPTable<id<MTLSamplerState>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLSamplerState>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableState<id<MTLSamplerState>>::RegisterInterpose<InterposeClass>(C);
	}
};

MTLPP_END

#endif /* imp_Sampler_hpp */
