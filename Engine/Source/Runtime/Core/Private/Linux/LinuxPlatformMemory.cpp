// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxPlatformMemory.cpp: Linux platform memory functions
=============================================================================*/

#include "CorePrivate.h"
#include "MallocAnsi.h"
#include "MallocJemalloc.h"
#include "MallocBinned.h"
#include <sys/sysinfo.h>
#include <unistd.h>		// sysconf

void FLinuxPlatformMemory::Init()
{
	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	UE_LOG(LogInit, Log, TEXT(" - Physical RAM available (not considering process quota): %d GB (%lu MB, %lu KB, %lu bytes)"), 
		MemoryConstants.TotalPhysicalGB, 
		MemoryConstants.TotalPhysical / ( 1024ULL * 1024ULL ), 
		MemoryConstants.TotalPhysical / 1024ULL, 
		MemoryConstants.TotalPhysical);
}

class FMalloc* FLinuxPlatformMemory::BaseAllocator()
{
#if FORCE_ANSI_ALLOCATOR
	return new FMallocAnsi();
#elif PLATFORM_SUPPORTS_JEMALLOC

	bool bUseJemalloc = false;

	// we get here before main due to global ctors, so need to do some hackery to get command line args
	if (FILE* CmdLineFile = fopen("/proc/self/cmdline", "r"))
	{
		char CmdLineBuffer[4096] = { 0 };
		FPlatformMemory::Memzero(CmdLineBuffer, sizeof(CmdLineBuffer));
		if (fgets(CmdLineBuffer, sizeof(CmdLineBuffer) - 2, CmdLineFile))	// -2 to guarantee that there are always two zeroes even if cmdline is too long
		{
			char * Arg = CmdLineBuffer;
			while (*Arg != 0 || Arg - CmdLineBuffer >= sizeof(CmdLineBuffer))
			{
				if (FCStringAnsi::Stricmp(Arg, "-jemalloc") == 0)
				{
					bUseJemalloc = true;
					break;
				}

				// advance till zero
				while(*Arg) 
				{
					++Arg;
				}
				++Arg;	// and skip the zero
			}
		}

		fclose(CmdLineFile);
	}

	printf( "Using %s.\n", bUseJemalloc ? "jemalloc" : "binned malloc" );

	if (bUseJemalloc)
	{
		return new FMallocJemalloc();
	}
	return new FMallocBinned(FPlatformMemory::GetConstants().PageSize & MAX_uint32, 0x100000000);

#else
	return new FMallocBinned(FPlatformMemory::GetConstants().PageSize & MAX_uint32, 0x100000000);
#endif // PLATFORM_SUPPORTS_JEMALLOC
}

void* FLinuxPlatformMemory::BinnedAllocFromOS( SIZE_T Size )
{	
	return valloc(Size);	// equivalent to memalign(sysconf(_SC_PAGESIZE),size).
}

void FLinuxPlatformMemory::BinnedFreeToOS( void* Ptr )
{
	return free(Ptr);
}

FPlatformMemoryStats FLinuxPlatformMemory::GetStats()
{
	FPlatformMemoryStats MemoryStats;

	// @todo

	return MemoryStats;
}

const FPlatformMemoryConstants& FLinuxPlatformMemory::GetConstants()
{
	static FPlatformMemoryConstants MemoryConstants;

	if( MemoryConstants.TotalPhysical == 0 )
	{
		// Gather platform memory stats.
		struct sysinfo SysInfo;
		unsigned long long MaxPhysicalRAMBytes = 0;

		if (0 == sysinfo(&SysInfo))
		{
			MaxPhysicalRAMBytes = static_cast< unsigned long long >( SysInfo.mem_unit ) * static_cast< unsigned long long >( SysInfo.totalram );
		}

		MemoryConstants.TotalPhysical = MaxPhysicalRAMBytes;
		MemoryConstants.TotalPhysicalGB = (MemoryConstants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
		MemoryConstants.PageSize = sysconf(_SC_PAGESIZE);
	}

	return MemoryConstants;	
}
