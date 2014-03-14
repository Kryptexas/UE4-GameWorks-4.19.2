// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Only use for supported platforms
#if PLATFORM_SUPPORTS_JEMALLOC

#define MEM_TIME(st)

//
// Jason Evans' malloc (default in FreeBSD, NetBSD, used in Firefox, Facebook servers) 
// http://www.canonware.com/jemalloc/
//
class FMallocJemalloc : public FMalloc
{

private:

	/** Time spent on memory tasks, in seconds */
	double		MemTime;

	// Implementation. 
	void OutOfMemory()
	{
		UE_LOG(LogHAL, Fatal, TEXT("%s"), TEXT("Ran out of virtual memory. To prevent this condition, you must free up more space on your primary hard disk.") );
	}

public:

	FMallocJemalloc() :
		MemTime		( 0.0 )
	{
	}
	
	// Begin FMalloc interface.

	virtual void* Malloc( SIZE_T Size, uint32 Alignment ) OVERRIDE;
	virtual void* Realloc( void* Ptr, SIZE_T NewSize, uint32 Alignment ) OVERRIDE;
	virtual void Free( void* Ptr ) OVERRIDE;
	virtual void DumpAllocations( FOutputDevice& Ar ) OVERRIDE;
	CORE_API virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE;
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) OVERRIDE;
	virtual bool IsInternallyThreadSafe() const OVERRIDE {  return true; }
	virtual const TCHAR * GetDescriptiveName() OVERRIDE { return TEXT("jemalloc"); }

	// End FMalloc interface.

};

#endif // PLATFORM_SUPPORTS_JEMALLOC
