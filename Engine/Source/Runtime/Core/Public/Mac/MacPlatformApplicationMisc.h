// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct CORE_API FMacPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static class GenericApplication* CreateApplication();
	static bool ControlScreensaver(EScreenSaverAction Action);
	static uint32 GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings );
	static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static struct FLinearColor GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma = 1.0f);
	static float GetDPIScaleFactorAtPoint(float X, float Y);
	static void PumpMessages(bool bFromMainLoop);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);
};

typedef FMacPlatformApplicationMisc FPlatformApplicationMisc;
