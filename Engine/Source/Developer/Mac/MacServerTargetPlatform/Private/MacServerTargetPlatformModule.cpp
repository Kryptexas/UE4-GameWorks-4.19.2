// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacServerTargetPlatformModule.cpp: Implements the FMacServerTargetPlatformModule class.
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

#include "MacServerTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Mac target platform (without editor).
 */
class FMacServerTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FMacServerTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( ) OVERRIDE
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericMacTargetPlatform<false, true, false>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FMacServerTargetPlatformModule, MacServerTargetPlatform);