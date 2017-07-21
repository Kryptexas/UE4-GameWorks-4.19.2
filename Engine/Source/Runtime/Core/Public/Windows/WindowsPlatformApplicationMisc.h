// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

struct CORE_API FWindowsPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static class GenericApplication* CreateApplication();
	static int32 GetAppIcon();
	static void PumpMessages(bool bFromMainLoop);
	static void PreventScreenSaver();
	static uint32 GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings );
	static uint32 GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static struct FLinearColor GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma = 1.0f);
	static bool GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle);
	static float GetDPIScaleFactorAtPoint(float X, float Y);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);
};

typedef FWindowsPlatformApplicationMisc FPlatformApplicationMisc;
