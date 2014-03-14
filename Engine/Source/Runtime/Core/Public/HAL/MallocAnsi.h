// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocAnsi.h: ANSI memory allocator.
=============================================================================*/

#pragma once

#if _MSC_VER || PLATFORM_MAC
#define USE_ALIGNED_MALLOC 1
#else
//@todo gcc: this should be implemented more elegantly on other platforms
#define USE_ALIGNED_MALLOC 0
#endif

#if PLATFORM_IOS
#include "mach/mach.h"
#endif

//
// ANSI C memory allocator.
//
class FMallocAnsi : public FMalloc
{
	
public:
	/**
	 * Constructor enabling low fragmentation heap on platforms supporting it.
	 */
	FMallocAnsi()
	{
#if PLATFORM_WINDOWS
		// Enable low fragmentation heap - http://msdn2.microsoft.com/en-US/library/aa366750.aspx
		intptr_t	CrtHeapHandle	= _get_heap_handle();
		ULONG		EnableLFH		= 2;
		HeapSetInformation( (PVOID)CrtHeapHandle, HeapCompatibilityInformation, &EnableLFH, sizeof(EnableLFH) );
#endif
	}

	// FMalloc interface.
	virtual void* Malloc( SIZE_T Size, uint32 Alignment ) OVERRIDE
	{
		Alignment = FMath::Max(Size >= 16 ? (uint32)16 : (uint32)8, Alignment);

#if USE_ALIGNED_MALLOC
		void* Result = _aligned_malloc( Size, Alignment );
#else
		void* Ptr = malloc( Size + Alignment + sizeof(void*) + sizeof(SIZE_T) );
		check(Ptr);
		void* Result = Align( (uint8*)Ptr + sizeof(void*) + sizeof(SIZE_T), Alignment );
		*((void**)( (uint8*)Result - sizeof(void*)					))	= Ptr;
		*((SIZE_T*)( (uint8*)Result - sizeof(void*) - sizeof(SIZE_T)	))	= Size;
#endif

		if (Result == NULL)
		{
			FPlatformMemory::OnOutOfMemory(Size, Alignment);
		}
		return Result;
	}

	virtual void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) OVERRIDE
	{
		void* Result;
		Alignment = FMath::Max(NewSize >= 16 ? (uint32)16 : (uint32)8, Alignment);

#if USE_ALIGNED_MALLOC
		if( Ptr && NewSize )
		{
			Result = _aligned_realloc( Ptr, NewSize, Alignment );
		}
		else if( Ptr == NULL )
		{
			Result = _aligned_malloc( NewSize, Alignment );
		}
		else
		{
			_aligned_free( Ptr );
			Result = NULL;
		}
#else
		if( Ptr && NewSize )
		{
			// Can't use realloc as it might screw with alignment.
			Result = Malloc( NewSize, Alignment );
			FMemory::Memcpy( Result, Ptr, FMath::Min(NewSize, *((SIZE_T*)( (uint8*)Ptr - sizeof(void*) - sizeof(SIZE_T))) ) );
			Free( Ptr );
		}
		else if( Ptr == NULL )
		{
			Result = Malloc( NewSize, Alignment);
		}
		else
		{
			free( *((void**)((uint8*)Ptr-sizeof(void*))) );
			Result = NULL;
		}
#endif
		if (Result == NULL && NewSize != 0)
		{
			FPlatformMemory::OnOutOfMemory(NewSize, Alignment);
		}

		return Result;
	}

	virtual void Free( void* Ptr ) OVERRIDE
	{
#if USE_ALIGNED_MALLOC
		_aligned_free( Ptr );
#else
		if( Ptr )
		{
			free( *((void**)((uint8*)Ptr-sizeof(void*))) );
		}
#endif
	}

	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) OVERRIDE
	{ 
		return false;
	}

#if PLATFORM_MAC
	virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE
	{
		// @todo implement in platform memory
		// Just get memory information for the process and report the working set instead
		task_basic_info_64_data_t TaskInfo;
		mach_msg_type_number_t TaskInfoCount = TASK_BASIC_INFO_COUNT;
		task_info( mach_task_self(), TASK_BASIC_INFO, (task_info_t)&TaskInfo, &TaskInfoCount );
		MemStats.TotalUsed = MemStats.TotalAllocated = TaskInfo.resident_size;
	}
#elif PLATFORM_IOS
	virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE
	{
		// @todo implement in platform memory
		task_basic_info info;
		mach_msg_type_number_t size = sizeof(info);
		kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
		MemStats.TotalUsed = MemStats.TotalAllocated = info.resident_size;

		mach_port_t host_port = mach_host_self();
		mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
		vm_size_t pagesize;
		vm_statistics_data_t vm_stat;

		host_page_size(host_port, &pagesize);
		(void) host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
		MemStats.OSReportedFree = vm_stat.free_count * pagesize;
	}
#endif

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 *
	 * @return true as we're using system allocator
	 */
	virtual bool IsInternallyThreadSafe() const OVERRIDE
	{
#if PLATFORM_MAC
			return true;
#elif PLATFORM_IOS
			return true;
#else
			return false;
#endif
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() OVERRIDE
	{
#if PLATFORM_WINDOWS
		int32 Result = _heapchk();
		check(Result != _HEAPBADBEGIN);
		check(Result != _HEAPBADNODE);
		check(Result != _HEAPBADPTR);
		check(Result != _HEAPEMPTY);
		check(Result == _HEAPOK);
#else
		return true;
#endif
		return true;
	}

	virtual const TCHAR * GetDescriptiveName() OVERRIDE { return TEXT("ANSI"); }
};

