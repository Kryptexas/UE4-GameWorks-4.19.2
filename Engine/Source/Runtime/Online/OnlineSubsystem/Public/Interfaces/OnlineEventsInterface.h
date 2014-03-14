// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

typedef FOnlineKeyValuePairs< FName, FVariantData > FOnlineEventParms;

/**
 *	IOnlineEvents - Interface class for events
 */
class IOnlineEvents
{
public:

	/**
	 * Trigger an event by name
	 *
	 * @param PlayerId	- Player to trigger the event for
	 * @param EventName - Name of the event
	 * @param Parms		- The parameter list to be passed into the event
	 *
	 * @return Whether the event was successfully triggered
	 */
	virtual bool TriggerEvent( const FUniqueNetId & PlayerId, const TCHAR * EventName, const FOnlineEventParms & Parms ) = 0;
};