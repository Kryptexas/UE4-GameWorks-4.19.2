// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

#include "OnlineSubsystemTwitch.h"
#include "OnlineSubsystemTwitchModule.h"

/** pre-pended to all Twitch logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("TWITCH: ")
