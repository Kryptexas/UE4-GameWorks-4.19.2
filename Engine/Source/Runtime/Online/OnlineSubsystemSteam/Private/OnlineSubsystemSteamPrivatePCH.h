// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine.h"
#include "OnlineSubsystemSteamModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystem.h"
#include "SocketSubsystem.h"

#include "ModuleManager.h"

#define INVALID_INDEX -1

/** Name of the current Steam SDK version in use (matches directory name) */
#define STEAM_SDK_VER TEXT("Steamv128")

/** FName declaration of Steam subsystem */
#define STEAM_SUBSYSTEM FName(TEXT("STEAM"))
/** URL Prefix when using Steam socket connection */
#define STEAM_URL_PREFIX TEXT("steam.")
/** Filename containing the appid during development */
#define STEAMAPPIDFILENAME TEXT("steam_appid.txt")

/** pre-pended to all steam logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("STEAM: ")

// @todo Steam: Steam headers trigger secure-C-runtime warnings in Visual C++. Rather than mess with _CRT_SECURE_NO_WARNINGS, we'll just
//	disable the warnings locally. Remove when this is fixed in the SDK
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif

// Steamworks SDK headers
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"

#if WITH_STEAMGC
#include "gc_client.h"
#endif

// @todo Steam: See above
#ifdef _MSC_VER
#pragma warning(pop)
#endif

