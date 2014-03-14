// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ConsoleOutputDevice.h: FOutputDeviceConsole definition.
=============================================================================*/

#pragma once

/**
 * This class servers as the base class for console window output.
 */
class CORE_API FOutputDeviceConsole : public FOutputDevice
{
protected:

	/** Ini file name to write console settings to. */
	FString IniFilename;

public:
	/**
	 * Shows or hides the console window. 
	 *
	 * @param ShowWindow	Whether to show (true) or hide (false) the console window.
	 */
	virtual void Show( bool ShowWindow )=0;

	/** 
	 * Returns whether console is currently shown or not
	 *
	 * @return true if console is shown, false otherwise
	 */
	virtual bool IsShown()=0;

	/** 
	 * Returns whether the application is already attached to a console window
	 *
	 * @return true if console is attahced, false otherwise
	 */
	virtual bool IsAttached()
	{
		return false;
	}

	/**
	 * Sets ini file name to write console settings to.
	 */
	void SetIniFilename(const TCHAR* InFilename) 
	{
		IniFilename = InFilename;
	}
};
