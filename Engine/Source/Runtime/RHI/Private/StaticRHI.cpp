// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticRHI.cpp: Dynamically bound Render Hardware Interface implementation.
=============================================================================*/

#include "RHI.h"

#if USE_STATIC_RHI

extern STATIC_RHI_CLASS_NAME* CreateStaticRHI();


// Globals.
STATIC_RHI_CLASS_NAME* GStaticRHI = NULL;


void RHIInit(bool bHasEditorToken)
{
	if(!GStaticRHI)
	{		
		GStaticRHI = CreateStaticRHI();
	}
}

void RHIExit()
{
	// Destruct the static RHI.
	delete GStaticRHI;
	GStaticRHI = NULL;
}



#endif