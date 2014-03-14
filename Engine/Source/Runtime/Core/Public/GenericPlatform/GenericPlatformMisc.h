// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformMisc.h: Generic platform misc classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once

class FText;

namespace EBuildConfigurations
{
	/**
	 * Enumerates build configurations.
	 */
	enum Type
	{
		/**
		 * Unknown build configuration.
		 */
		Unknown,

		/**
		 * Debug build.
		 */
		Debug,

		/**
		 * DebugGame build.
		 */
		DebugGame,

		/**
		 * Development build.
		 */
		Development,

		/**
		 * Shipping build.
		 */
		Shipping,

		/**
		 * Test build.
		 */
		Test
	};

	/**
	 * Returns the string representation of the specified EBuildConfiguration value.
	 *
	 * @param Configuration - The string to get the EBuildConfiguration::Type for.
	 *
	 * @return An EBuildConfiguration::Type value.
	 */
	CORE_API EBuildConfigurations::Type FromString( const FString& Configuration );

	/**
	 * Returns the string representation of the specified EBuildConfiguration value.
	 *
	 * @param Configuration - The value to get the string for.
	 *
	 * @return The string representation.
	 */
	CORE_API const TCHAR* ToString( EBuildConfigurations::Type Configuration );

	/**
	 * Returns the localized text representation of the specified EBuildConfiguration value.
	 *
	 * @param Configuration - The value to get the text for.
	 *
	 * @return The localized Build configuration text
	 */
	CORE_API FText ToText( EBuildConfigurations::Type Configuration );
}


namespace EBuildTargets
{
	/**
	 * Enumerates build targets.
	 */
	enum Type
	{
		/**
		 * Unknown build target.
		 */
		Unknown,

		/**
		 * Editor target.
		 */
		Editor,

		/**
		 * Game target.
		 */
		Game,

		/**
		 * Server target.
		 */
		Server
	};

	/**
	 * Returns the string representation of the specified EBuildTarget value.
	 *
	 * @param Target - The string to get the EBuildTarget::Type for.
	 *
	 * @return An EBuildTarget::Type value.
	 */
	CORE_API EBuildTargets::Type FromString( const FString& Target );

	/**
	 * Returns the string representation of the specified EBuildTarget value.
	 *
	 * @param Target - The value to get the string for.
	 *
	 * @return The string representation.
	 */
	CORE_API const TCHAR* ToString( EBuildTargets::Type Target );
}


namespace EErrorReportMode
{
	/** 
	 * Enumerates supported error reporting modes.
	 */
	enum Type
	{
		/** Displays a call stack with an interactive dialog for entering repro steps, etc. */
		Interactive,

		/** Unattended mode.  No repro steps, just submits data straight to the server */
		Unattended,

		/** Same as unattended, but displays a balloon window in the system tray to let the user know */
		Balloon,
	};
}


namespace EAppMsgType
{
	/**
	 * Enumerates supported message dialog button types.
	 */
	enum Type
	{
		Ok,
		YesNo,
		OkCancel,
		YesNoCancel,
		CancelRetryContinue,
		YesNoYesAllNoAll,
		YesNoYesAllNoAllCancel,
		YesNoYesAll,
	};
}


namespace EAppReturnType
{
	/**
	 * Enumerates message dialog return types.
	 */
	enum Type
	{
		No,
		Yes,
		YesAll,
		NoAll,
		Cancel,
		Ok,
		Retry,
		Continue,
	};
}

struct CORE_API FGenericCrashContext
{
};

struct CORE_API FGenericMemoryWarningContext
{
};


/**
* Generic implementation for most platforms
**/
struct CORE_API FGenericPlatformMisc
{
	/**
	* Called during appInit() after cmd line setup
	*/
	static void PlatformPreInit()
	{
	}
	static void PlatformInit()
	{
	}
	static void PlatformPostInit()
	{
	}

	static class GenericApplication* CreateApplication();

	static void* GetHardwareWindow()
	{
		return NULL;
	}

	/**
	 * Set/restore the Console Interrupt (Control-C, Control-Break, Close) handler
	 */
	static void SetGracefulTerminationHandler()
	{
	}

	/**
	 * Installs handler for the unexpected (due to error) termination of the program,
     * including, but not limited to, crashes.
	 *
	 */
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext & Context))
	{
	}

	/**
	 * Retrieve a environment variable from the system
	 *
	 * @param VariableName The name of the variable (ie "Path")
	 * @param Result The string to copy the value of the variable into
	 * @param ResultLength The size of the Result string
	 */
	static void GetEnvironmentVariable(const TCHAR* VariableName, TCHAR* Result, int32 ResultLength)
	{
		*Result = 0;
	}

	/**
	 * Retrieve the Mac address of the current adapter.
	 * 
	 * @return array of bytes representing the Mac address, or empty array if unable to determine.
	 */
	static TArray<uint8> GetMacAddress();

	/**
	 * Retrieve the Mac address of the current adapter as a string.
	 * 
	 * @return String representing the Mac address, or empty string.
	 */
	static FString GetMacAddressString();

	/**
	 * Retrieve the Mac address of the current adapter as a hashed string (privacy)
	 *
	 * @return String representing the hashed Mac address, or empty string.
	 */
	static FString GetHashedMacAddressString();

	/**
	 * Returns a unique string for device identification
	 * 
	 * @return the unique string generated by this platform for this device
	 */
	static FString GetUniqueDeviceId();

	/** Submits a crash report to a central server (release builds only) */
	static void SubmitErrorReport( const TCHAR* InErrorHist, EErrorReportMode::Type InMode );

	/** Return true if a debugger is present */
	FORCEINLINE static bool IsDebuggerPresent()
	{
#if UE_BUILD_SHIPPING
		return 0; 
#else
		return 1; // unknown platforms return true so that they can crash into a debugger
#endif
	}

	/** Break into the debugger, if IsDebuggerPresent returns true, otherwise do nothing  */
	FORCEINLINE static void DebugBreak()
	{
		if (IsDebuggerPresent())
		{
			*((int32*)3) = 13; // unknown platforms crash into the debugger
		}
	}

	static bool SupportsMessaging()
	{
		return true;
	}

	static bool SupportsLocalCaching()
	{
		return true;
	}

	/**
	 * Enforces strict memory load/store ordering across the memory barrier call.
	 */
	static void MemoryBarrier();

	/**
	 * Handles IO failure by ending gameplay.
	 *
	 * @param Filename	If not NULL, name of the file the I/O error occured with
	 */
	void static HandleIOFailure( const TCHAR* Filename );

	/**
	 * Set a handler to be called when there is a memory warning from the OS 
	 *
	 * @param Handler	The handler to call
	 */
	static void SetMemoryWarningHandler(void (* Handler)(const FGenericMemoryWarningContext & Context))
	{
	}
	
	/**
	 *	Pumps Windows messages.
	 *	@param bFromMainLoop if true, this is from the main loop, otherwise we are spinning waiting for the render thread
	 */
	FORCEINLINE static void PumpMessages(bool bFromMainLoop)
	{
	}

	FORCEINLINE static uint32 GetKeyMap( uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings )
	{
		return 0;
	}

	FORCEINLINE static uint32 GetCharKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings)
	{
		return 0;
	}

	FORCEINLINE static uint32 GetLastError()
	{
		return 0;
	}

	FORCEINLINE static void RaiseException( uint32 ExceptionCode )
	{
#if HACK_HEADER_GENERATOR
		// We want Unreal Header Tool to throw an exception but in normal runtime code 
		// we don't support exception handling
		throw( ExceptionCode );
#else	
		*((uint32*)3) = ExceptionCode;
#endif
	}

protected:
	/**
	* Retrieves some standard key code mappings (usually called by a subclass's GetCharKeyMap)
	 *
	 * @param OutKeyMap Key map to add to.
	 * @param bMapUppercaseKeys If true, will map A, B, C, etc to EKeys::A, EKeys::B, EKeys::C
	 * @param bMapLowercaseKeys If true, will map a, b, c, etc to EKeys::A, EKeys::B, EKeys::C
	 */
	static uint32 GetStandardPrintableKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings, bool bMapUppercaseKeys, bool bMapLowercaseKeys);



public:
	/**
	 * Platform specific function for adding a named event that can be viewed in PIX
	 */
	FORCEINLINE static void BeginNamedEvent(const class FColor& Color,const TCHAR* Text)
	{
	}

	FORCEINLINE static void BeginNamedEvent(const class FColor& Color,const ANSICHAR* Text)
	{
	}

	/**
	 * Platform specific function for closing a named event that can be viewed in PIX
	 */
	FORCEINLINE static void EndNamedEvent()
	{
	}

	/**
	 * Rebuild the commandline if needed
	 *
	 * @param NewCommandLine The commandline to fill out
	 *
	 * @return true if NewCommandLine should be pushed to FCommandLine
	 */
	FORCEINLINE static bool ResetCommandLine(TCHAR NewCommandLine[16384])
	{
		return 0;
	}

	/** 
	 *	Retrieve the entry for the given string from the platforms equivalent to the Windows Registry
	 *
	 *	@param	InRegistryKey		The registry key to query
	 *	@param	InValueName			The name of the value from that key
	 *  @param	bPerUserSetting		Whether to get the per-user registry key rather than the local machine's registry key
	 *	@param	OutValue			The value found
	 *
	 *	@return	bool				true if the entry was found (and OutValue contains the result), false if not
	 */
	static bool GetRegistryString(const FString& InRegistryKey, const FString& InValueName, bool bPerUserSetting, FString& OutValue);


	 /** Sends a message to a remote tool, and debugger consoles */
	static void LowLevelOutputDebugString(const TCHAR *Message);
	static void VARARGS LowLevelOutputDebugStringf(const TCHAR *Format, ... );

	/** Prints string to the default output */
	static void LocalPrint( const TCHAR* Str );

	/**
	 * Requests application exit.
	 *
	 * @param	Force	If true, perform immediate exit (dangerous because config code isn't flushed, etc).
	 *                  If false, request clean main-loop exit from the platform specific code.
	 */
	static void RequestExit( bool Force );

	/**
	 * Returns the last system error code in string form.  NOTE: Only one return value is valid at a time!
	 *
	 * @param OutBuffer the buffer to be filled with the error message
	 * @param BufferLength the size of the buffer in character count
	 * @param Error the error code to convert to string form
	 */
	static const TCHAR* GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error);

	/** Copies text to the operating system clipboard. */
	static void ClipboardCopy(const TCHAR* Str);
	/** Pastes in text from the operating system clipboard. */
	static void ClipboardPaste(class FString& Dest);

	/** Create a new globally unique identifier. **/
	static void CreateGuid(class FGuid& Result);

	/** 
	 * Show a message box if possible, otherwise print a message and return the default
	 * @param MsgType - what sort of options are provided
	 * @param Text - specific message
	 * @param Caption - string indicating the title of the message box
	 * @return Very strange convention...not really EAppReturnType, see implementation

	**/
	static EAppReturnType::Type MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption );

	/**
	 * Prevents screen-saver from kicking in by moving the mouse by 0 pixels. This works even on
	 * Vista in the presence of a group policy for password protected screen saver.
	 */
	static void PreventScreenSaver()
	{
	}

	/*
	 *	Shows the intial game window in the proper position and size.
	 *	It also changes the window proc from StartupWindowProc to
	 *	UWindowsClient::StaticWndProc.
	 *	This function doesn't have any effect if called a second time.
	 */
	static void ShowGameWindow( void* StaticWndProc )
	{
	}
	/**
	 * Handles Game Explorer, Firewall and FirstInstall commands, typically from the installer
	 * @returns false if the game cannot continue.
	 */
	static bool CommandLineCommands()
	{
		return 1;
	}

	/**
	 * Detects whether we're running in a 64-bit operating system.
	 *
	 * @return	true if we're running in a 64-bit operating system
	 */
	FORCEINLINE static bool Is64bitOperatingSystem()
	{
		return !!PLATFORM_64BITS;
	}

	/**
	 * Checks structure of the path against platform formatting requirements
	 * return - true if path is formatted validly
	 */
	static bool IsValidAbsolutePathFormat(const FString& Path)
	{
		return 1;
	}

	/** 
	 * Platform-specific normalization of path
	 * E.g. on Linux/Unix platforms, replaces ~ with user home directory, so ~/.config becomes /home/joe/.config (or /Users/Joe/.config)
	 */
	static void NormalizePath(FString& InPath)
	{
	}

	/**
	 * return the number of hardware CPU cores
	 */
	static int32 NumberOfCores()
	{
		return 1;
	}

	/**
	 * return the number of logical CPU cores
	 */
	static int32 NumberOfCoresIncludingHyperthreads();

	/**
	 * Return the number of worker threads we should spawn, based on number of cores
	 */
	static int32 NumberOfWorkerThreadsToSpawn();

	/**
	 * Return the platform specific async IO system, or NULL if the standard one should be used.
	 */
	static struct FAsyncIOSystemBase* GetPlatformSpecificAsyncIOSystem()
	{
		return NULL;
	}

	/** Return the name of the platform features module. Can be NULL if there are no extra features for this platform */
	static const TCHAR* GetPlatformFeaturesModuleName()
	{
		// by deafult, no module
		return NULL;
	}

	/** Get the application root directory. */
	static const TCHAR* RootDir();

	/** Get the engine directory */
	static const TCHAR* EngineDir();

	/**
	 *	Return the GameDir
	 */
	static const TCHAR* GameDir();

	/**
	 * Load the preinit modules required by this platform, typically they are the renderer modules
	 */
	static void LoadPreInitModules()
	{
	}
	/**
	 * Load the platform-specific startup modules
	 */
	static void LoadStartupModules()
	{
	}

	static const TCHAR* GetUBTPlatform();

	/** 
	 * Determines the shader format for the plarform
	 *
	 * @return	Returns the shader format to be used by that platform
	 */
	static const TCHAR* GetNullRHIShaderFormat();

	/** 
	 * Returns the platform specific chunk based install interface
	 *
	 * @return	Returns the platform specific chunk based install implementation
	 */
	static class IPlatformChunkInstall* GetPlatformChunkInstall();

	/**
	 * Has the OS execute a command and path pair (such as launch a browser)
	 *
	 * @param ComandType OS hint as to the type of command 
	 * @param Command the command to execute
	 *
	 * @return whether the command was successful or not
	 */
	static bool OsExecute(const TCHAR* CommandType, const TCHAR* Command)
	{
		return false;
	}

	/**
	 * @return true if this build is meant for release to retail
	 */
	static bool IsPackagedForDistribution()
	{
#if UE_BUILD_SHIPPING
		return true;
#else
		return false;
#endif
	}

	/**
	 * Searches for a window that matches the window name or the title starts with a particular text. When
	 * found, it returns the title text for that window
	 *
	 * @param TitleStartsWith an alternative search method that knows part of the title text
	 * @param OutTitle the string the data is copied into
	 *
	 * @return whether the window was found and the text copied or not
	 */
	static bool GetWindowTitleMatchingText(const TCHAR* TitleStartsWith, FString& OutTitle)
	{
		return false;
	}

	static FString GetDefaultLocale();

	/**
	 *	Platform-specific Exec function
	 *
	 *  @param	InWorld		World context
	 *	@param	Cmd			The command to execute
	 *	@param	Out			The output device to utilize
	 *
	 *	@return	bool		true if command was processed, false if not
	 */
	static bool Exec(class UWorld* InWorld, const TCHAR* Cmd, class FOutputDevice& Out)
	{
		return false;
	}

	/**
	 * Sample the displayed pixel color from anywhere on the screen using the OS
	 *
	 * @param	InScreenPos		The screen coords to sample for current pixel color
	 * @param	InGamma			Optional gamma correction to apply to the screen color
	 *
	 * @return					The color of the pixel displayed at the chosen location
	 */
	static struct FLinearColor GetScreenPixelColor(const struct FVector2D& InScreenPos, float InGamma = 1.0f);

#if !UE_BUILD_SHIPPING
	static void SetShouldPromptForRemoteDebugging(bool bInShouldPrompt)
	{
		bShouldPromptForRemoteDebugging = bInShouldPrompt;
	}

	static void SetShouldPromptForRemoteDebugOnEnsure(bool bInShouldPrompt)
	{
		bPromptForRemoteDebugOnEnsure = bInShouldPrompt;
	}
#endif	//#if !UE_BUILD_SHIPPING

	static void PromptForRemoteDebugging(bool bIsEnsure)
	{
	}

	FORCEINLINE static void PrefetchBlock(const void* InPtr, int32 NumBytes = 1)
	{
	}

	/** Platform-specific instruction prefetch */
	FORCEINLINE static void Prefetch(void const* x, int32 offset = 0)
	{	
	}

	/**
	 * Gets the default profile name. Used if there is no device profile specified
	 *
	 * @return the default profile name.
	 */
	static const TCHAR* GetDefaultDeviceProfileName();

	/** 
	 * Allows a game/program/etc to control the game directory in a special place (for instance, monolithic programs that don't have .uprojects)
	 */
	static void SetOverrideGameDir(const FString& InOverrideDir);

	/**
	 * Return an ordered list of target platforms this runtime can support (ie Android_DXT, Android
	 * would mean that it prefers Android_DXT, but can use Android as well)
	 */
	static void GetValidTargetPlatforms(class TArray<class FString>& TargetPlatformNames);

	/**
	 * Returns whether the platform wants to use a touch screen for virtual joysticks.
	 */
	static bool GetUseVirtualJoysticks()
	{
		return PLATFORM_HAS_TOUCH_MAIN_SCREEN;
	}

#if !UE_BUILD_SHIPPING
protected:
	/** Whether the user should be prompted to allow for a remote debugger to be attached */
	static bool bShouldPromptForRemoteDebugging;
	/** Whether the user should be prompted to allow for a remote debugger to be attached on an ensure */
	static bool bPromptForRemoteDebugOnEnsure;
#endif	//#if !UE_BUILD_SHIPPING
};

