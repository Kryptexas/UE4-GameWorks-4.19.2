// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateRemoteClasses.cpp: Implements the module's script classes.
=============================================================================*/

#include "SlateRemotePrivatePCH.h"


USlateRemoteSettings::USlateRemoteSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
	, EnableRemoteServer(true)
	, EditorServerEndpoint(SLATE_REMOTE_SERVER_DEFAULT_EDITOR_ENDPOINT.ToString())
	, GameServerEndpoint(SLATE_REMOTE_SERVER_DEFAULT_GAME_ENDPOINT.ToString())
{ }
