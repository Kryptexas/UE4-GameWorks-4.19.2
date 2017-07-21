// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct CORE_API FLinuxPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static void Init();
	static bool InitSDL();
	static void TearDown();
	static uint32 WindowStyle();
	static class GenericApplication* CreateApplication();
	static void PumpMessages(bool bFromMainLoop);
	static bool ControlScreensaver(EScreenSaverAction Action);
	static uint32 GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings );
	static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);
};

typedef FLinuxPlatformApplicationMisc FPlatformApplicationMisc;
