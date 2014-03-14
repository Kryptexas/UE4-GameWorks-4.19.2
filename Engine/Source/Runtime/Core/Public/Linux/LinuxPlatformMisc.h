// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	LinuxPlatformMisc.h: Linux platform misc functions
==============================================================================================*/

#pragma once

struct CORE_API FLinuxCrashContext : public FGenericCrashContext
{
	/** Signal number */
	int32 Signal;
	
	/** Additional signal info */
	siginfo_t* Info;
	
	/** Thread context */
	ucontext_t*	Context;

	/** Symbols received via backtrace_symbols(), if any (note that we will need to clean it up) */
	char ** BacktraceSymbols;

	/** File descriptor needed for libelf to open (our own) binary */
	int ExeFd;

	/** Elf header as used by libelf (forward-declared in the same way as libelf does it, it's normally of Elf type) */
	struct _Elf* ElfHdr;

	/** DWARF handle used by libdwarf (forward-declared in the same way as libdwarf does it, it's normally of Dwarf_Debug type) */
	struct _Dwarf_Debug	* DebugInfo;

	/** Memory reserved for "exception" (signal) info */
	TCHAR SignalDescription[128];

	/** Memory reserved for minidump-style callstack info */
	char MinidumpCallstackInfo[16384];

	FLinuxCrashContext()
		:	Signal(0)
		,	Info(NULL)
		,	Context(NULL)
		,	BacktraceSymbols(NULL)
		,	ExeFd(-1)
		,	ElfHdr(NULL)
		,	DebugInfo(NULL)
	{
		SignalDescription[ 0 ] = 0;
		MinidumpCallstackInfo[ 0 ] = 0;
	}

	~FLinuxCrashContext();

	/**
	 * Inits the crash context from data provided by a signal handler.
	 *
	 * @param InSignal number (SIGSEGV, etc)
	 * @param InInfo additional info (e.g. address we tried to read, etc)
	 * @param InContext thread context
	 */
	void InitFromSignal(int32 InSignal, siginfo_t* InInfo, void* InContext);

	/**
	 * Gets information for the crash.
	 *
	 * @param Address the address to look up info for
	 * @param OutFunctionNamePtr pointer to function name (may be NULL). Caller doesn't have to free it, but need to consider it temporary (i.e. next GetInfoForAddress() call on any thread may change it).
	 * @param OutSourceFilePtr pointer to source filename (may be NULL). Caller doesn't have to free it, but need to consider it temporary (i.e. next GetInfoForAddress() call on any thread may change it).
	 * @param OutLineNumberPtr pointer to line in a source file (may be NULL). Caller doesn't have to free it, but need to consider it temporary (i.e. next GetInfoForAddress() call on any thread may change it).
	 *
	 * @return true if succeeded in getting the info. If false is returned, none of above parameters should be trusted to contain valid data!
	 */
	bool GetInfoForAddress(void * Address, const char **OutFunctionNamePtr, const char **OutSourceFilePtr, int *OutLineNumberPtr);

	/**
	 * Dumps all the data from crash context to the "minidump" report.
	 *
	 * @param DiagnosticsPath Path to put the file to
	 */
	void GenerateReport(const FString & DiagnosticsPath) const;
};

/**
 * Linux implementation of the misc OS functions
 */
struct CORE_API FLinuxMisc : public FGenericPlatformMisc
{
	static void PlatformInit();
	static void SetGracefulTerminationHandler();
	static void SetCrashHandler(void (* CrashHandler)(const FGenericCrashContext & Context));
#if !UE_BUILD_SHIPPING
	static bool IsDebuggerPresent();
	FORCEINLINE static void DebugBreak()
	{
		if( IsDebuggerPresent() )
		{
			raise(SIGTRAP);
		}
	}
#endif // !UE_BUILD_SHIPPING

	static const TCHAR* RootDir();
	static void NormalizePath(FString& InPath);

	FORCEINLINE static void MemoryBarrier()
	{
		__sync_synchronize();
	}

	static int32 NumberOfCores();
	static int32 NumberOfCoresIncludingHyperthreads();

	static const TCHAR* EngineDir()
	{
		return TEXT("../../../Engine/");
	}
};

typedef FLinuxMisc FPlatformMisc;