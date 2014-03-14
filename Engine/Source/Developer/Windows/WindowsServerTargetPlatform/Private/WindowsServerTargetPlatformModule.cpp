// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsServerTargetPlatformModule.cpp: Implements the FWindowsServerTargetPlatformModule class.
=============================================================================*/

#include "WindowsServerTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Windows target platform as a server.
 */
class FWindowsServerTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FWindowsServerTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( )
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericWindowsTargetPlatform<false, true, false>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FWindowsServerTargetPlatformModule, WindowsServerTargetPlatform);