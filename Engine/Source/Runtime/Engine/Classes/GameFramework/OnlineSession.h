// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Everything a player controller will use to manage an online session.
 */

#pragma once
#include "OnlineSession.generated.h"

UCLASS(config=Game)
class ENGINE_API UOnlineSession : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/** Register all delegates needed to manage online sessions.  */
	virtual void RegisterOnlineDelegates() {};

	/** Tear down all delegates used to manage online sessions.	 */
	virtual void ClearOnlineDelegates() {};

	/** Called to tear down any online sessions and return to main menu	 */
	virtual void HandleDisconnect(UWorld *World, class UNetDriver *NetDriver);
};



