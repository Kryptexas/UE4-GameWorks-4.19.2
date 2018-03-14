// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats.h"

/**
 * Generic result type for cryptographic functions.
 */
enum class EPlatformCryptoResult
{
	Success,
	Failure
};

/**
 * Stat group for implementations to use.
 */
DECLARE_STATS_GROUP(TEXT("PlatformCrypto"), STATGROUP_PlatformCrypto, STATCAT_Advanced);
