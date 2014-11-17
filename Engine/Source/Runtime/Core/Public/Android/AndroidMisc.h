// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	AndroidMisc.h: Android platform misc functions
==============================================================================================*/

#pragma once

//@todo android: this entire file

struct CORE_API FAndroidCrashContext : public FGenericCrashContext
{
	/** Signal number */
	int32 Signal;
	
	/** Additional signal info */
	siginfo* Info;
	
	/** Thread context */
	void* Context;

	FAndroidCrashContext()
		:	Signal(0)
		,	Info(NULL)
		,	Context(NULL)
	{
	}

	~FAndroidCrashContext()
	{
	}

	/**
	 * Inits the crash context from data provided by a signal handler.
	 *
	 * @param InSignal number (SIGSEGV, etc)
	 * @param InInfo additional info (e.g. address we tried to read, etc)
	 * @param InContext thread context
	 */
	void InitFromSignal(int32 InSignal, siginfo* InInfo, void* InContext)
	{
		Signal = InSignal;
		Info = InInfo;
		Context = InContext;
	}
};

/**
 * Android implementation of the misc OS functions
 */
struct CORE_API FAndroidMisc : public FGenericPlatformMisc
{
	static class GenericApplication* CreateApplication();

	static void LowLevelOutputDebugString(const TCHAR *Message);
	static void LocalPrint(const TCHAR *Message);
	static void PlatformPreInit();
	static void PlatformInit();
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength);
	static void* GetHardwareWindow();
	static void SetHardwareWindow(void* InWindow);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );
	static int32 NumberOfCores();
	static void LoadPreInitModules();
	static void BeforeRenderThreadStarts();
	static bool SupportsLocalCaching();
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext & Context));
	// NOTE: THIS FUNCTION IS DEFINED IN ANDROIDOPENGL.CPP
	static void GetValidTargetPlatforms(class TArray<class FString>& TargetPlatformNames);
	static bool GetUseVirtualJoysticks();
	static uint32 GetKeyMap( uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings );
	static const TCHAR* GetDefaultDeviceProfileName() { return TEXT("Android"); }

	// ANDROID ONLY:
	static void SetVersionInfo( FString AndroidVersion, FString DeviceMake, FString DeviceModel );
	static const FString GetAndroidVersion();
	static const FString GetDeviceMake();
	static const FString GetDeviceModel();
	static FString GetGPUFamily();
	static FString GetGLVersion();
	static bool SupportsFloatingPointRenderTargets();

#if !UE_BUILD_SHIPPING
	FORCEINLINE static bool IsDebuggerPresent()
	{
		//@todo Android
		return false;
	}

	FORCEINLINE static void DebugBreak()
	{
		if( IsDebuggerPresent() )
		{
			//@todo Android
			//Signal interupt to our process, this is caught by the main thread, this is not immediate but you can continue
			//when triggered by check macros you will need to inspect other threads for the appFailAssert call.
			//kill( getpid(), SIGINT );

			//If you want an immediate break use the trap instruction, continued execuction is halted
		}
	}
#endif

	FORCEINLINE static void MemoryBarrier()
	{
		__sync_synchronize();
	}


	static void* NativeWindow ; //raw platform Main window
	
	// run time compatibility information
	static FString AndroidVersion; // version of android we are running eg "4.0.4"
	static FString DeviceMake; // make of the device we are running on eg. "samsung"
	static FString DeviceModel; // model of the device we are running on eg "SAMSUNG-SGH-I437"

};

typedef FAndroidMisc FPlatformMisc;
