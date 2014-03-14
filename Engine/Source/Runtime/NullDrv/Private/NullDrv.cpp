// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NullDrv.cpp: Unreal Null RHI library implementation.
=============================================================================*/

#include "NullDrvPrivate.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FNullDynamicRHIModule, NullDrv);

#include "RenderCore.h"
#include "RenderResource.h"

bool FNullDynamicRHIModule::IsSupported()
{
	return true;
}