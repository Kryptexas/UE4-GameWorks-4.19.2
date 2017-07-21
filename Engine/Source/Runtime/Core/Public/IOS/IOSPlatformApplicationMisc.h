// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct CORE_API FIOSPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static class GenericApplication* CreateApplication();
	static bool ControlScreensaver(EScreenSaverAction Action);
	static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static uint32 GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings );

	static void ResetGamepadAssignments();
	static void ResetGamepadAssignmentToController(int32 ControllerId);
	static bool IsControllerAssignedToGamepad(int32 ControllerId);

	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);

private:
	static class FIOSApplication* CachedApplication;
};

typedef FIOSPlatformApplicationMisc FPlatformApplicationMisc;
