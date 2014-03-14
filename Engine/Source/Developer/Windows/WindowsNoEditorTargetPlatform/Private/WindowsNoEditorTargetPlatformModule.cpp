// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsNoEditorTargetPlatformModule.cpp: Implements the FWindowsNoEditorTargetPlatformModule class.
=============================================================================*/

#include "WindowsNoEditorTargetPlatformPrivatePCH.h"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Windows target platform (without editor).
 */
class FWindowsNoEditorTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	virtual ~FWindowsNoEditorTargetPlatformModule( )
	{
		Singleton = NULL;
	}

	virtual ITargetPlatform* GetTargetPlatform( )
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericWindowsTargetPlatform<false, false, false>();
		}

		return Singleton;
	}
};


IMPLEMENT_MODULE(FWindowsNoEditorTargetPlatformModule, WindowsNoEditorTargetPlatform);