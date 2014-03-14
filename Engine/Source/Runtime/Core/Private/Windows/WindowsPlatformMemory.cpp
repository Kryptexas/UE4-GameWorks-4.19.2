// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsPlatformMemory.cpp: Windows platform memory functions
=============================================================================*/

#include "CorePrivate.h"

#include "MallocTBB.h"
#include "MallocAnsi.h"
#include "GenericPlatformMemoryPoolStats.h"


#if !FORCE_ANSI_ALLOCATOR
#include "MallocBinned.h"
#endif

#include "AllowWindowsPlatformTypes.h"
#include <Psapi.h>
#include "HideWindowsPlatformTypes.h"
#pragma comment(lib, "psapi.lib")

/** Enable this to track down windows allocations not wrapped by our wrappers
int WindowsAllocHook(int nAllocType, void *pvData,
				  size_t nSize, int nBlockUse, long lRequest,
				  const unsigned char * szFileName, int nLine )
{
	if ((nAllocType == _HOOK_ALLOC || nAllocType == _HOOK_REALLOC) && nSize > 2048)
	{
		static int i = 0;
		i++;
	}
	return true;
}
*/

#include "GenericPlatformMemoryPoolStats.h"


void FWindowsPlatformMemory::Init()
{
	FGenericPlatformMemory::SetupMemoryPools();

#if PLATFORM_32BITS
	const int64 GB(1024*1024*1024);
	SET_MEMORY_STAT(MCR_Physical, 2*GB); //2Gb of physical memory on win32
#endif


	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	UE_LOG(LogInit, Log, TEXT("Memory total: Physical=%.1fGB (%dGB approx) Pagefile=%.1fGB Virtual=%.1fGB"), 
		float(MemoryConstants.TotalPhysical/1024.0/1024.0/1024.0),
		MemoryConstants.TotalPhysicalGB, 
		float((MemoryConstants.TotalVirtual-MemoryConstants.TotalPhysical)/1024.0/1024.0/1024.0), 
		float(MemoryConstants.TotalVirtual/1024.0/1024.0/1024.0) );
}

FMalloc* FWindowsPlatformMemory::BaseAllocator()
{
#if FORCE_ANSI_ALLOCATOR
	return new FMallocAnsi();
#elif WITH_EDITOR && TBB_ALLOCATOR_ALLOWED
	return new FMallocTBB();
#else
	return new FMallocBinned((uint32)(GetConstants().PageSize&MAX_uint32), (uint64)MAX_uint32+1);
#endif

//	_CrtSetAllocHook(WindowsAllocHook); // Enable to track down windows allocs not handled by our wrapper
}

FPlatformMemoryStats FWindowsPlatformMemory::GetStats()
{
	/**
	 *	GlobalMemoryStatusEx 
	 *	MEMORYSTATUSEX 
	 *		ullTotalPhys
	 *		ullAvailPhys
	 *		ullTotalVirtual
	 *		ullAvailVirtual
	 *		
	 *	GetProcessMemoryInfo
	 *	PROCESS_MEMORY_COUNTERS
	 *		WorkingSetSize
	 *		PagefileUsage
	 *	
	 *	GetPerformanceInfo
	 *		PPERFORMANCE_INFORMATION 
	 *		PageSize
	 */

	FPlatformMemoryStats MemoryStats;

	// Gather platform memory stats.
	MEMORYSTATUSEX MemoryStatusEx = {0};
	MemoryStatusEx.dwLength = sizeof( MemoryStatusEx );
	::GlobalMemoryStatusEx( &MemoryStatusEx );

	PROCESS_MEMORY_COUNTERS ProcessMemoryCounters = {0};
	::GetProcessMemoryInfo( ::GetCurrentProcess(), &ProcessMemoryCounters, sizeof(ProcessMemoryCounters) );

	MemoryStats.AvailablePhysical = MemoryStatusEx.ullAvailPhys;
	MemoryStats.AvailableVirtual = MemoryStatusEx.ullAvailVirtual;
	
	MemoryStats.WorkingSetSize = ProcessMemoryCounters.WorkingSetSize;
	MemoryStats.PeakWorkingSetSize = ProcessMemoryCounters.PeakWorkingSetSize;
	MemoryStats.PagefileUsage = ProcessMemoryCounters.PagefileUsage;

	return MemoryStats;
}

const FPlatformMemoryConstants& FWindowsPlatformMemory::GetConstants()
{
	static FPlatformMemoryConstants MemoryConstants;

	if( MemoryConstants.TotalPhysical == 0 )
	{
		// Gather platform memory constants.
		MEMORYSTATUSEX MemoryStatusEx = {0};
		MemoryStatusEx.dwLength = sizeof( MemoryStatusEx );
		::GlobalMemoryStatusEx( &MemoryStatusEx );

		PERFORMANCE_INFORMATION PerformanceInformation = {0};
		::GetPerformanceInfo( &PerformanceInformation, sizeof(PerformanceInformation) );

		MemoryConstants.TotalPhysical = MemoryStatusEx.ullTotalPhys;
		MemoryConstants.TotalVirtual = MemoryStatusEx.ullTotalVirtual;
		MemoryConstants.PageSize = PerformanceInformation.PageSize;

		MemoryConstants.TotalPhysicalGB = (MemoryConstants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
	}

	return MemoryConstants;	
}

void* FWindowsPlatformMemory::BinnedAllocFromOS( SIZE_T Size )
{
	return VirtualAlloc( NULL, Size, MEM_COMMIT, PAGE_READWRITE );
}

void FWindowsPlatformMemory::BinnedFreeToOS( void* Ptr )
{
	verify(VirtualFree( Ptr, 0, MEM_RELEASE ) != 0);
}
