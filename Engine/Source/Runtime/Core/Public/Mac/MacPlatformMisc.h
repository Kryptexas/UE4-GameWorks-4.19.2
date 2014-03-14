// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MacPlatformMisc.h: Mac platform misc functions
==============================================================================================*/

#pragma once

/**
* Mac implementation of the misc OS functions
**/
struct CORE_API FMacPlatformMisc : public FGenericPlatformMisc
{
	static void PlatformPreInit();
	static void PlatformInit();
	static void PlatformPostInit();
	static class GenericApplication* CreateApplication();
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength);
	static TArray<uint8> GetMacAddress();
	static void SubmitErrorReport( const TCHAR* InErrorHist, EErrorReportMode::Type InMode );
#if !UE_BUILD_SHIPPING
	FORCEINLINE static bool IsDebuggerPresent()
	{
		// Based on http://developer.apple.com/library/mac/#qa/qa1361/_index.html

		struct kinfo_proc Info = { 0 };
		int32 Mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
		SIZE_T Size = sizeof(Info);

		sysctl( Mib, sizeof( Mib ) / sizeof( *Mib ), &Info, &Size, NULL, 0 );

		return ( Info.kp_proc.p_flag & P_TRACED ) != 0;
	}
	FORCEINLINE static void DebugBreak()
	{
		if( IsDebuggerPresent() )
		{
			__asm__ ( "int $3" );
		}
	}
#endif

	FORCEINLINE static void MemoryBarrier()
	{
		OSMemoryBarrier();
	}

	static void PumpMessages(bool bFromMainLoop);
	static uint32 GetKeyMap( uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings );
	static uint32 GetCharKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings);
	static void RequestExit(bool Force);
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);
	static void ClipboardCopy(const TCHAR* Str);
	static void ClipboardPaste(class FString& Dest);
	static void CreateGuid(class FGuid& Result);
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );
	static void PreventScreenSaver();
	static void ShowGameWindow( void* StaticWndProc );
	static bool CommandLineCommands();
	static int32 NumberOfCores();
	static int32 NumberOfCoresIncludingHyperthreads();
	static void LoadPreInitModules();


	/** 
	 * Determines the shader format for the plarform
	 *
	 * @return	Returns the shader format to be used by that platform
	 */
	FORCEINLINE static const TCHAR* GetNullRHIShaderFormat() 
	{ 
		return TEXT("GLSL_150"); 
	}

	/**
	 * Sample the displayed pixel color from anywhere on the screen using the OS
	 *
	 * @param	InScreenPos		The screen coords to sample for current pixel color
	 * @param	InGamma			Optional gamma correction to apply to the screen color
	 *
	 * @return					The color of the pixel displayed at the chosen location
	 */
	static struct FLinearColor GetScreenPixelColor(const FVector2D& InScreenPos, float InGamma = 1.0f);

	/**
	 * Uses cpuid instruction to get the vendor string
	 *
	 * @return	CPU vendor name
	 */
	static FString GetCPUVendor();

	/**
	 * Converts Y position from Slate (left-top origin) to Cocoa (left-bottom origin) coordinates
	 *
	 * @param	YPosition		Position to convert
	 *
	 * @return					Converted position
	 */
	static int32 ConvertSlateYPositionToCocoa(int32 YPosition);
	
	static void SetGracefulTerminationHandler();
	
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext & Context));

	static FString GetDefaultLocale();
};

#ifdef __OBJC__

class FScopeAutoreleasePool
{
public:

	FScopeAutoreleasePool()
	{
		Pool = [[NSAutoreleasePool alloc] init];
	}

	~FScopeAutoreleasePool()
	{
		[Pool release];
	}

private:

	NSAutoreleasePool*	Pool;
};

#define SCOPED_AUTORELEASE_POOL const FScopeAutoreleasePool Pool;

#endif // __OBJC__

typedef FMacPlatformMisc FPlatformMisc;

enum EMacModifierKeys
{
	MMK_RightCommand	= 0xF754,
	MMK_LeftCommand		= 0xF755,
	MMK_LeftShift		= 0xF756,
	MMK_CapsLock		= 0xF757,
	MMK_LeftAlt			= 0xF758,
	MMK_LeftControl		= 0xF759,
	MMK_RightShift		= 0xF760,
	MMK_RightAlt		= 0xF761,
	MMK_RightControl	= 0xF762
};

struct CORE_API FMacCrashContext : public FApplePlatformCrashContext
{
	/**
	 * Dumps all the data from crash context to the "minidump" report.
	 * @param DiagnosticsPath Path to put the file to
	 */
	void GenerateReport(char const* DiagnosticsPath) const;
	
	/** Mimics Windows WER format */
	void GenerateWindowsErrorReport(char const* WERPath) const;
	
	/** Creates (fake so far) minidump */
	void GenerateMinidump(char const* MinidumpCallstackInfo, char const* Path) const;
	
	/** Generates information for crash reporter */
	void GenerateCrashInfoAndLaunchReporter() const;
};

