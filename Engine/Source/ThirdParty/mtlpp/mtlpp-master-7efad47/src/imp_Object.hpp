// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#ifndef imp_Object_hpp
#define imp_Object_hpp

#include "imp_SelectorCache.hpp"

MTLPP_BEGIN

template<typename ObjC>
struct IMPTableBase
{
	IMPTableBase()
	{
	}
	
	IMPTableBase(Class C)
	: Retain(C)
	, Release(C)
	, Dealloc(C)
	{
	}
	
	template<typename InterposeClass>
	void RegisterInterpose(Class C)
	{
		INTERPOSE_REGISTRATION(Retain, C);
		INTERPOSE_REGISTRATION(Release, C);
		INTERPOSE_REGISTRATION(Dealloc, C);
	}
	
	INTERPOSE_SELECTOR(ObjC, retain, Retain, void);
	INTERPOSE_SELECTOR(ObjC, release, Release, void);
	INTERPOSE_SELECTOR(ObjC, dealloc, Dealloc, void);
};

template<typename ObjC, typename Interpose>
struct IMPTable : public IMPTableBase<ObjC>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<ObjC>(C)
	{
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<ObjC>::template RegisterInterpose<Interpose>(C);
	}
};

MTLPP_END

#endif /* imp_Object_hpp */
