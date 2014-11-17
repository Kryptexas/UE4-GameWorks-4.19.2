// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RocketSupport.h: Declares the FRocketSupport class.
=============================================================================*/

#pragma once

/**
 * Core support for launching the engine in "Rocket" mode.
 */
class CORE_API FRocketSupport
{
public:
	/**
	 * Checks if we are running in 'Rocket' mode (Rocket game or Rocket editor).
	 *
	 * @param CmdLine	The optional command line to parse for the rocket switch
	 * 
	 * @return	True if we are in rocket mode, otherwise false
	 */
	static bool IsRocket( const TCHAR* CmdLine = FCommandLine::Get() );
};
