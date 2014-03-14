// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameSessionSettings.h: Declares the UGameSessionSettings class.
=============================================================================*/

#pragma once

#include "GameSessionSettings.generated.h"


UCLASS(config=Game)
class ENGINESETTINGS_API UGameSessionSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

	/**
	 * Maximum number of spectators allowed by this server.
	 */
	UPROPERTY(globalconfig, EditAnywhere, Category=GameSessionSettings)
	int32 MaxSpectators;

	/**
	 * Maximum number of players allowed by this server.
	 */
	UPROPERTY(globalconfig, EditAnywhere, Category=GameSessionSettings)
	int32 MaxPlayers;

    /**
	 * Is voice enabled always or via a push to talk key binding.
	 */
	UPROPERTY(globalconfig, EditAnywhere, Category=GameSessionSettings)
	uint32 bRequiresPushToTalk:1;
};
