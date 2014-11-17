// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacClientTargetPlatformModule.cpp: Implements the FMacClientTargetPlatformModule class.
=============================================================================*/

#if PLATFORM_MAC
#define FVector FVectorWorkaround
#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif
#undef check
#undef verify
#undef FVector
#endif

#include "MacClientTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Mac target platform (without editor).
 */
class FMacClientTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FMacClientTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( ) OVERRIDE
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericMacTargetPlatform<false, false, true>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FMacClientTargetPlatformModule, MacClientTargetPlatform);