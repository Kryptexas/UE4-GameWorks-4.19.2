// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRemoteSettings.h: Declares the USlateRemoteSettings class.
=============================================================================*/

#pragma once

#include "SlateRemoteSettings.generated.h"


UCLASS(config=Engine)
class USlateRemoteSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Whether the Slate Remote server is enabled.
	 */
	UPROPERTY(config, EditAnywhere, Category=RemoteServer)
	bool EnableRemoteServer;

	/**
	 * The IP endpoint to listen to when the Remote Server runs in the Editor.
	 */
	UPROPERTY(config, EditAnywhere, Category=RemoteServer)
	FString EditorServerEndpoint;

	/**
	 * The IP endpoint to listen to when the Remote Server runs in a game.
	 */
	UPROPERTY(config, EditAnywhere, Category=RemoteServer)
	FString GameServerEndpoint;
};
