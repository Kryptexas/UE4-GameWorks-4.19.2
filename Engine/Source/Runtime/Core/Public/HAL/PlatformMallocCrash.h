// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "HAL/Platform.h"

#include "GenericPlatform/GenericPlatformMallocCrash.h"

#if PLATFORM_USES_STACKBASED_MALLOC_CRASH
typedef FGenericStackBasedMallocCrash FPlatformMallocCrash;
#else
typedef FGenericPlatformMallocCrash FPlatformMallocCrash;
#endif
