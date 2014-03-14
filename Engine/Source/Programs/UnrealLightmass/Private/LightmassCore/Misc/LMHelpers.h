// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LMHelpers.h: Mostly defines
=============================================================================*/

#pragma once

namespace Lightmass
{

class FLightmassLog : public FOutputDevice
{
public:

	FLightmassLog();
	~FLightmassLog();

	// BEGIN FOutputDevice Interface 
	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE;
	virtual void Flush() OVERRIDE;
	// END FOutputDevice Interface

	/**
	 * Returns the filename of the log file.
	 */
	const TCHAR* GetLogFilename() const
	{
		return Filename;
	}

	/**
	 * Singleton interface.
	 */
	static FLightmassLog* Get();

private:

	/** Handle to log file */
	FArchive*	File;

	/** Filename of the log file. */
	TCHAR	Filename[ MAX_PATH ];
};

}
