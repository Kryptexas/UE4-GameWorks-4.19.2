// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	HTML5Misc.h: HTML5 platform misc functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformMisc.h"
#include "CoreTypes.h"
#include "HTML5/HTML5DebugLogging.h"
#include "HTML5/HTML5SystemIncludes.h"
#include <emscripten/emscripten.h>

#if UE_BUILD_SHIPPING
#define UE_DEBUG_BREAK() ((void)0)
#else
#define UE_DEBUG_BREAK() (FHTML5Misc::DebugBreakInternal())
#endif

/**
 * HTML5 implementation of the misc OS functions
 */
struct CORE_API FHTML5Misc : public FGenericPlatformMisc
{
	static void PlatformInit();
	static const TCHAR* GetPlatformFeaturesModuleName();
	static FString GetDefaultLocale();
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext& Context));
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );
	FORCEINLINE static int32 NumberOfCores()
	{
		return 1;
	}

	static bool AllowThreadHeartBeat()
	{
		return false;
	}

	FORCEINLINE static void MemoryBarrier()
	{
		// Do nothing on x86; the spec requires load/store ordering even in the absence of a memory barrier.

		// @todo HTML5: Will this be applicable for final?
	}
	/** Return true if a debugger is present */
	FORCEINLINE static bool IsDebuggerPresent()
	{
		return true;
	}

	/** Break into the debugger, if IsDebuggerPresent returns true, otherwise do nothing  */
	FORCEINLINE static void DebugBreakInternal()
	{
		if (IsDebuggerPresent())
		{
			emscripten_log(255, "DebugBreak() called!");
			EM_ASM(
				var callstack = new Error;
				throw callstack.stack;
			);
		}
	}

	DEPRECATED(4.19, "FPlatformMisc::DebugBreak is deprecated. Use the UE_DEBUG_BREAK() macro instead.")
	FORCEINLINE static void DebugBreak()
	{
		UE_DEBUG_BREAK();
	}

	/** Break into debugger. Returning false allows this function to be used in conditionals. */
	DEPRECATED(4.19, "FPlatformMisc::DebugBreakReturningFalse is deprecated. Use the (UE_DEBUG_BREAK(), false) expression instead.")
	FORCEINLINE static bool DebugBreakReturningFalse()
	{
		UE_DEBUG_BREAK();
		return false;
	}

	/** Prompts for remote debugging if debugger is not attached. Regardless of result, breaks into debugger afterwards. Returns false for use in conditionals. */
	DEPRECATED(4.19, "FPlatformMisc::DebugBreakAndPromptForRemoteReturningFalse() is deprecated.")
	static FORCEINLINE bool DebugBreakAndPromptForRemoteReturningFalse(bool bIsEnsure = false)
	{
#if !UE_BUILD_SHIPPING
		if (!IsDebuggerPresent())
		{
			PromptForRemoteDebugging(bIsEnsure);
		}

		UE_DEBUG_BREAK();
#endif

		return false;
	}

	static void LocalPrint(const TCHAR* Str);
};

typedef FHTML5Misc FPlatformMisc;
