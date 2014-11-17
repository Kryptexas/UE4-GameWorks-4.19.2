// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsEvent.h: Declares the FEventWin class.
=============================================================================*/

#pragma once

#include "AllowWindowsPlatformTypes.h"

/**
 * Implements the Windows version of the FEvent interface.
 */
class FEventWin
	: public FEvent
{
public:

	/**
	 * Default constructor.
	 */
	FEventWin ()
		: Event(NULL)
	{
	}

	/**
	 * Destructor.
	 */
	virtual ~FEventWin ()
	{
		if (Event != NULL)
		{
			CloseHandle(Event);
		}
	}


public:

	virtual bool Create (bool bIsManualReset = false) override
	{
		// Create the event and default it to non-signaled
		Event = CreateEvent(NULL, bIsManualReset, 0, NULL);

		return Event != NULL;
	}

	virtual void Trigger () override
	{
		check(Event);

		SetEvent(Event);
	}

	virtual void Reset () override
	{
		check(Event);

		ResetEvent(Event);
	}

	virtual bool Wait (uint32 WaitTime, const bool bIgnoreThreadIdleStats = false) override;

private:

	// Holds the handle to the event
	HANDLE Event;
};

#include "HideWindowsPlatformTypes.h"
