// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsClientTargetPlatformModule.cpp: Implements the FWindowsClientTargetPlatformModule class.
=============================================================================*/

#include "WindowsClientTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Windows target platform as a server.
 */
class FWindowsClientTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FWindowsClientTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( )
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericWindowsTargetPlatform<false, false, true>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FWindowsClientTargetPlatformModule, WindowsClientTargetPlatform);