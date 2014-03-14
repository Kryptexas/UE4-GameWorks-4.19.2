// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformStackWalk.h: Generic platform stack walking functions...do not do anything
==============================================================================================*/

#pragma once

/** @name ObjectFlags
* Flags used to control the output from stack tracing
*/
typedef uint32 EVerbosityFlags;

#define VF_DISPLAY_BASIC		0x00000000
#define VF_DISPLAY_FILENAME		0x00000001
#define VF_DISPLAY_MODULE		0x00000002
#define VF_DISPLAY_ALL			0xffffffff

#define PLATFORM_SUPPORTS_STACK_SYMBOLS 0

/**
* This is used to capture all of the module information needed to load pdb's.
* @todo, the memory profiler should not be using this as platform agnostic
*/
struct FStackWalkModuleInfo
{
	uint64 BaseOfImage;
	uint32 ImageSize;
	uint32 TimeDateStamp;
	TCHAR ModuleName[32];
	TCHAR ImageName[256];
	TCHAR LoadedImageName[256];
	uint32 PdbSig;
	uint32 PdbAge;
	struct
	{
		unsigned long  Data1;
		unsigned short Data2;
		unsigned short Data3;
		unsigned char  Data4[8];
	} PdbSig70;
};



/**
* Generic implementation for most platforms
**/
struct CORE_API FGenericPlatformStackWalk
{
	/**
	 * Initializes stack traversal and symbol. Must be called before any other stack/symbol functions. Safe to reenter.
	 */
	static bool InitStackWalking()
	{
		return 1;
	}

	/**
	 * Converts the passed in program counter address to a human readable string and appends it to the passed in one.
	 * @warning: The code assumes that HumanReadableString is large enough to contain the information.
	 *
	 * @param	CurrentCallDepth		Depth of the call, if known (-1 if not - note that some platforms may not return meaningful information in the latter case)
	 * @param	ProgramCounter			Address to look symbol information up for
	 * @param	HumanReadableString		String to concatenate information with
	 * @param	HumanReadableStringSize size of string in characters
	 * @param	VerbosityFlags			Bit field of requested data for output. -1 for all output.
	 * @param	Context					Pointer to crash context, if any
	 * @return	true if the symbol was found, otherwise false
	 */ 
	static bool ProgramCounterToHumanReadableString( int32 CurrentCallDepth, uint64 ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, EVerbosityFlags VerbosityFlags = VF_DISPLAY_ALL, FGenericCrashContext* Context = NULL );

#if PLATFORM_SUPPORTS_STACK_SYMBOLS // never true here, but we provide this function signature for reference
	/**
	 * Converts the passed in program counter address to a symbol info struct, filling in module and filename, line number and displacement.
	 * @warning: The code assumes that the destination strings are big enough
	 *
	 * @param	ProgramCounter					Address to look symbol information up for
	 * @param	OutProgramCounterSymbolInfo		symbol information associated with program counter
	 */
	 static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo&  OutProgramCounterSymbolInfo);
#endif

	/**
	 * Capture a stack backtrace and optionally use the passed in exception pointers.
	 *
	 * @param	BackTrace			[out] Pointer to array to take backtrace
	 * @param	MaxDepth			Entries in BackTrace array
	 * @param	Context				Optional thread context information
	 */
	static void CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context = NULL );

	/**
	 * Walks the stack and appends the human readable string to the passed in one.
	 * @warning: The code assumes that HumanReadableString is large enough to contain the information.
	 *
	 * @param	HumanReadableString	String to concatenate information with
	 * @param	HumanReadableStringSize size of string in characters
	 * @param	IgnoreCount			Number of stack entries to ignore (some are guaranteed to be in the stack walking code)
	 * @param	Context				Optional thread context information
	 */ 
	static void StackWalkAndDump( ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, int32 IgnoreCount, void* Context = NULL );

	/**
	 * Returns the number of modules loaded by the currently running process.
	 */
	FORCEINLINE static int32 GetProcessModuleCount()
	{
		return 0;
	}

	/**
	 * Gets the signature for every module loaded by the currently running process.
	 *
	 * @param	ModuleSignatures		An array to retrieve the module signatures.
	 * @param	ModuleSignaturesSize	The size of the array pointed to by ModuleSignatures.
	 *
	 * @return	The number of modules copied into ModuleSignatures
	 */
	FORCEINLINE int32 GetProcessModuleSignatures(FStackWalkModuleInfo *ModuleSignatures, const int32 ModuleSignaturesSize)
	{
		return 0;
	}


};
