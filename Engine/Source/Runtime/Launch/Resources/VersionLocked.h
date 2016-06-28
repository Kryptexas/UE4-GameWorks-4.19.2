// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// To lock a build to a specific version set VERSION_LOCKED to 1, and 
// fill out the details in the desired section below.

#define LOCKED_BUILD_VERSION 0
#define LOCKED_NETWORK_VERSION 0
#define LOCKED_REPLAY_VERSION 3008042

#if LOCKED_BUILD_VERSION

#define ENGINE_MAJOR_VERSION	4
#define ENGINE_MINOR_VERSION	12
#define ENGINE_PATCH_VERSION	0

#define ENGINE_IS_LICENSEE_VERSION 0

#define BUILT_FROM_CHANGELIST LOCKED_BUILD_VERSION
#define BRANCH_NAME "++Orion+Release-NA"

#define ENGINE_IS_PROMOTED_BUILD (BUILT_FROM_CHANGELIST > 0)

#define EPIC_COMPANY_NAME  "Epic Games, Inc."
#define EPIC_COPYRIGHT_STRING "Copyright 1998-2015 Epic Games, Inc. All Rights Reserved."
#define EPIC_PRODUCT_NAME "Unreal Engine"
#define EPIC_PRODUCT_IDENTIFIER "UnrealEngine"

#define BUILD_VERSION VERSION_TEXT(BRANCH_NAME) VERSION_TEXT("-CL-") VERSION_STRINGIFY(BUILT_FROM_CHANGELIST) 

#endif
