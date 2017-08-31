// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformApplicationMisc.h"

typedef void (*UpdateCachedMacMenuStateProc)(void);

struct APPLICATIONCORE_API FMacPlatformApplicationMisc : public FGenericPlatformApplicationMisc
{
	static void PreInit();
	static void PostInit();
	static void TearDown();
	static void LoadPreInitModules();
	static class FOutputDeviceConsole* CreateConsoleOutputDevice();
	static class FOutputDeviceError* GetErrorOutputDevice();
	static class FFeedbackContext* GetFeedbackContext();
	static class GenericApplication* CreateApplication();
	static void RequestMinimize();
	static bool IsThisApplicationForeground();
	static bool ControlScreensaver(EScreenSaverAction Action);
	static void ActivateApplication();
	static void UpdateWindowMenu();
	static struct FLinearColor GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma = 1.0f);
	static float GetDPIScaleFactorAtPoint(float X, float Y);
	static void PumpMessages(bool bFromMainLoop);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);

	// MAC ONLY

	static UpdateCachedMacMenuStateProc UpdateCachedMacMenuState;

	static bool bChachedMacMenuStateNeedsUpdate;

	static id<NSObject> CommandletActivity;
};

typedef FMacPlatformApplicationMisc FPlatformApplicationMisc;
