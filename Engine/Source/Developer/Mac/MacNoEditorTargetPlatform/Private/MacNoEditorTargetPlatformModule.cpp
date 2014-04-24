// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacNoEditorTargetPlatformModule.cpp: Implements the FMacNoEditorTargetPlatformModule class.
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

#include "MacNoEditorTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Mac target platform (without editor).
 */
class FMacNoEditorTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FMacNoEditorTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( )
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericMacTargetPlatform<false, false>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FMacNoEditorTargetPlatformModule, MacNoEditorTargetPlatform);