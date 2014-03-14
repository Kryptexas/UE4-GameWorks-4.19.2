// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "GameDelegates.h"

FGameDelegates& FGameDelegates::Get()
{
	// return the singleton object
	static FGameDelegates Singleton;
	return Singleton;
}
