// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformMisc.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsPlatformMisc.h"
#elif PLATFORM_PS4
#include "PS4/PS4Misc.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneMisc.h"
#elif PLATFORM_MAC
#include "Mac/MacPlatformMisc.h"
#elif PLATFORM_IOS
#include "IOS/IOSPlatformMisc.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidMisc.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformMisc.h"
#elif PLATFORM_LINUX
#include "Linux/LinuxPlatformMisc.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformMisc.h"
#endif

#ifndef UE_DEBUG_BREAK
#error UE_DEBUG_BREAK is not defined for this platform
#endif

#ifndef PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
#error PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING is not defined.
#endif

// Master switch for scoped named events
#define ENABLE_NAMED_EVENTS (!UE_BUILD_SHIPPING && 1)

#if ENABLE_NAMED_EVENTS

class CORE_API FScopedNamedEvent
{
public:

	FScopedNamedEvent(const struct FColor& Color, const TCHAR* Text)
	{
		FPlatformMisc::BeginNamedEvent(Color, Text);
	}

	FScopedNamedEvent(const struct FColor& Color, const ANSICHAR* Text)
	{
		FPlatformMisc::BeginNamedEvent(Color, Text);
	}

	~FScopedNamedEvent()
	{
		FPlatformMisc::EndNamedEvent();
	}	
};

// Lightweight scoped named event separate from stats system.  Will be available in test builds.  
// Events cost profiling overhead so use them judiciously in final code.

#if PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
#define NAMED_EVENT_STR(x) x
#else
#define NAMED_EVENT_STR(x) L##x
#endif

#define SCOPED_NAMED_EVENT(Name, Color)          FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_##Name##_)(Color, NAMED_EVENT_STR(#Name));
#define SCOPED_NAMED_EVENT_FSTRING(Text, Color)  FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_)         (Color, *Text);
#define SCOPED_NAMED_EVENT_TEXT(Text, Color)     FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_)         (Color, NAMED_EVENT_STR(Text));
#define SCOPED_NAMED_EVENT_F(Format, Color, ...) FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_)         (Color, *FString::Printf(Format, __VA_ARGS__));

#else

#define SCOPED_NAMED_EVENT(...)
#define SCOPED_NAMED_EVENT_FSTRING(...)
#define SCOPED_NAMED_EVENT_TEXT(...)
#define SCOPED_NAMED_EVENT_F(...)

#endif
