// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NullDrv.h: Public Null RHI definitions.
=============================================================================*/

#pragma once

// Dependencies.
#include "Core.h"
#include "RHI.h"
#include "NullRHI.h"

/** Implements the NullDrv module as a dynamic RHI providing module. */
class FNullDynamicRHIModule : public IDynamicRHIModule
{
public:

	// IModuleInterface
	virtual bool SupportsDynamicReloading() OVERRIDE { return false; }

	// IDynamicRHIModule
	virtual bool IsSupported() OVERRIDE;

	virtual FDynamicRHI* CreateRHI() OVERRIDE
	{
		return new FNullDynamicRHI();
	}
};