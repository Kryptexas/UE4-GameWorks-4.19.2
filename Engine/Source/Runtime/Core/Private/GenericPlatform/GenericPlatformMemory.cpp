// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformMemory.cpp: Generic implementations of memory platform functions
=============================================================================*/

#include "CorePrivate.h"
#include "MallocAnsi.h"
#include "GenericPlatformMemoryPoolStats.h"


DEFINE_STAT(MCR_Physical);
DEFINE_STAT(MCR_GPU);
DEFINE_STAT(MCR_TexturePool);

void FGenericPlatformMemory::SetupMemoryPools()
{
	SET_MEMORY_STAT(MCR_Physical, 0); // "unlimited" physical memory, we still need to make this call to set the short name, etc
	SET_MEMORY_STAT(MCR_GPU, 0); // "unlimited" GPU memory, we still need to make this call to set the short name, etc
	SET_MEMORY_STAT(MCR_TexturePool, 0); // "unlimited" Texture memory, we still need to make this call to set the short name, etc
}

void FGenericPlatformMemory::Init()
{
	SetupMemoryPools();
	UE_LOG(LogHAL, Warning, TEXT("FGenericPlatformMemory::Init not implemented on this platform"));
}

void FGenericPlatformMemory::OnOutOfMemory(uint64 Size, uint32 Alignment)
{
	UE_LOG(LogHAL, Fatal, TEXT("Ran out of memory allocating %llu bytes with alignment %u"), Size, Alignment);
}

FMalloc* FGenericPlatformMemory::BaseAllocator()
{
	return new FMallocAnsi();
}

FPlatformMemoryStats FGenericPlatformMemory::GetStats()
{
	UE_LOG(LogHAL, Warning, TEXT("FGenericPlatformMemory::GetStats not implemented on this platform"));
	return FPlatformMemoryStats();
}


const FPlatformMemoryConstants& FGenericPlatformMemory::GetConstants()
{
	UE_LOG(LogHAL, Warning, TEXT("FGenericPlatformMemory::GetConstants not implemented on this platform"));
	static FPlatformMemoryConstants MemoryConstants;
	return MemoryConstants;
}

uint32 FGenericPlatformMemory::GetPhysicalGBRam()
{
	return FPlatformMemory::GetConstants().TotalPhysicalGB;
}


void* FGenericPlatformMemory::BinnedAllocFromOS( SIZE_T Size )
{
	UE_LOG(LogHAL, Error, TEXT("FGenericPlatformMemory::BinnedAllocFromOS not implemented on this platform"));
	return nullptr;
}

void FGenericPlatformMemory::BinnedFreeToOS( void* Ptr )
{
	UE_LOG(LogHAL, Error, TEXT("FGenericPlatformMemory::BinnedFreeToOS not implemented on this platform"));
}

void FGenericPlatformMemory::DumpStats( class FOutputDevice& Ar )
{
	const float InvMB = 1.0f / 1024.0f / 1024.0f;
	const FPlatformMemoryConstants& MemoryConstants = FPlatformMemory::GetConstants();
	FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
	Ar.Logf(TEXT("Platform Memory Status"));
	Ar.Logf(TEXT("Working Set: %.2f MB used, %.2f MB peak"), MemoryStats.WorkingSetSize*InvMB, MemoryStats.PeakWorkingSetSize*InvMB );
	Ar.Logf(TEXT("Physical Memory: %.2f MB allocated, %.2f MB total"), (MemoryConstants.TotalPhysical - MemoryStats.AvailablePhysical)*InvMB, MemoryConstants.TotalPhysical*InvMB );
	Ar.Logf(TEXT("Virtual Memory: %.2f MB allocated, %.2f MB total"), (MemoryConstants.TotalVirtual - MemoryStats.AvailableVirtual)*InvMB, MemoryConstants.TotalVirtual*InvMB );

}

void FGenericPlatformMemory::DumpPlatformAndAllocatorStats( class FOutputDevice& Ar )
{
	FPlatformMemory::DumpStats( Ar );
	GMalloc->DumpAllocations( Ar );
}

void FGenericPlatformMemory::Memswap( void* Ptr1, void* Ptr2, SIZE_T Size )
{
	void* Temp = FMemory_Alloca(Size);
	FPlatformMemory::Memcpy( Temp, Ptr1, Size );
	FPlatformMemory::Memcpy( Ptr1, Ptr2, Size );
	FPlatformMemory::Memcpy( Ptr2, Temp, Size );
}
