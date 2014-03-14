// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================	
	NullRHI.h: Null RHI definitions.
=============================================================================*/

#ifndef __NULLRHI_H__
#define __NULLRHI_H__

/** A null implementation of the dynamically bound RHI. */
class FNullDynamicRHI : public FDynamicRHI
{
public:

	FNullDynamicRHI();

	// FDynamicRHI interface.
	virtual void Init();

	// Implement the dynamic RHI interface using the null implementations defined in RHIMethods.h
	#define DEFINE_RHIMETHOD(Type,Name,ParameterTypesAndNames,ParameterNames,ReturnStatement,NullImplementation) \
		virtual Type Name ParameterTypesAndNames { NullImplementation; }
	#include "RHIMethods.h"
	#undef DEFINE_RHIMETHOD

private:

	/** Allocates a static buffer for RHI functions to return as a write destination. */
	static void* GetStaticBuffer();
};

#endif
