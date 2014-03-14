// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealFrontendMain.h: Declares the UnrealFrontend application's main loop.
=============================================================================*/

#pragma once

#include "Core.h"
#include "Messaging.h"
#include "Slate.h"
#include "StandaloneRenderer.h"


/**
 * The application's main function.
 *
 * @param CommandLine - The application command line.
 *
 * @return Application's exit value.
 */
int32 UnrealFrontendMain( const TCHAR* CommandLine );
