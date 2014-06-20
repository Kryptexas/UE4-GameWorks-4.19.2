// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include <windows.h>
#include "../../Public/Modules/ModuleVersion.h"
#include "ModuleVersionResource.h"

#define XSTR(s) #s
#define STRINGIFY(s) XSTR(s)

// Netral language
LANGUAGE LANG_NEUTRAL,SUBLANG_NEUTRAL

// Declare the engine API version string
#if !IS_MONOLITHIC
ID_MODULE_API_VERSION_RESOURCE RCDATA
{
	STRINGIFY(MODULE_API_VERSION) 0
}
#endif