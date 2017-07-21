// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct CORE_API FHTML5PlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static class GenericApplication* CreateApplication();
	static uint32 GetKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
};

typedef FHTML5PlatformApplicationMisc FPlatformApplicationMisc;
