// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "IOSPlatformApplicationMisc.h"
#include "IOSApplication.h"
#include "IOSInputInterface.h"

FIOSApplication* FIOSPlatformApplicationMisc::CachedApplication = nullptr;

GenericApplication* FIOSPlatformApplicationMisc::CreateApplication()
{
	CachedApplication = FIOSApplication::CreateIOSApplication();
	return CachedApplication;
}

bool FIOSPlatformApplicationMisc::ControlScreensaver(EScreenSaverAction Action)
{
	IOSAppDelegate* AppDelegate = [IOSAppDelegate GetDelegate];
	[AppDelegate EnableIdleTimer : (Action == FGenericPlatformApplicationMisc::Enable)];
	return true;
}

uint32 FIOSPlatformApplicationMisc::GetCharKeyMap(uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings)
{
	return FGenericPlatformApplicationMisc::GetStandardPrintableKeyMap(KeyCodes, KeyNames, MaxMappings, true, true);
}

uint32 FIOSPlatformApplicationMisc::GetKeyMap( uint32* KeyCodes, FString* KeyNames, uint32 MaxMappings )
{
#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };
	
	uint32 NumMappings = 0;
	
	// we only handle a few "fake" keys from the IOS keyboard delegate stuff in IOSView.cpp
	if (KeyCodes && KeyNames && (MaxMappings > 0))
	{
		ADDKEYMAP(KEYCODE_ENTER, TEXT("Enter"));
		ADDKEYMAP(KEYCODE_BACKSPACE, TEXT("BackSpace"));
		ADDKEYMAP(KEYCODE_ESCAPE, TEXT("Escape"));
	}
	return NumMappings;
}

void FIOSPlatformApplicationMisc::ResetGamepadAssignments()
{
	UE_LOG(LogIOS, Warning, TEXT("Restting gamepad assignments is not allowed in IOS"))
}

void FIOSPlatformApplicationMisc::ResetGamepadAssignmentToController(int32 ControllerId)
{
	
}

bool FIOSPlatformApplicationMisc::IsControllerAssignedToGamepad(int32 ControllerId)
{
	FIOSInputInterface* InputInterface = (FIOSInputInterface*)CachedApplication->GetInputInterface();
	return InputInterface->IsControllerAssignedToGamepad(ControllerId);
}

void FIOSPlatformApplicationMisc::ClipboardCopy(const TCHAR* Str)
{
#if !PLATFORM_TVOS
	CFStringRef CocoaString = FPlatformString::TCHARToCFString(Str);
	UIPasteboard* Pasteboard = [UIPasteboard generalPasteboard];
	[Pasteboard setString:(NSString*)CocoaString];
#endif
}

void FIOSPlatformApplicationMisc::ClipboardPaste(class FString& Result)
{
#if !PLATFORM_TVOS
	UIPasteboard* Pasteboard = [UIPasteboard generalPasteboard];
	NSString* CocoaString = [Pasteboard string];
	if(CocoaString)
	{
		TArray<TCHAR> Ch;
		Ch.AddUninitialized([CocoaString length] + 1);
		FPlatformString::CFStringToTCHAR((CFStringRef)CocoaString, Ch.GetData());
		Result = Ch.GetData();
	}
	else
	{
		Result = TEXT("");
	}
#endif
}

