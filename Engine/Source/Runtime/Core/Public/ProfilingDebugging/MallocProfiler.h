// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocProfiler.h: Memory profiling support.
=============================================================================*/
#pragma once

#include "UMemoryDefines.h"
#include "CompressedGrowableBuffer.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProfilingDebugging, Log, All);


#if USE_MALLOC_PROFILER

/*=============================================================================
	Malloc profiler enumerations
=============================================================================*/

/**
 * The lower 2 bits of a pointer are piggy-bagged to store what kind of data follows it. This enum lists
 * the possible types.
 */
enum EProfilingPayloadType
{
	TYPE_Malloc  = 0,
	TYPE_Free	 = 1,
	TYPE_Realloc = 2,
	TYPE_Other   = 3,
	// Don't add more than 4 values - we only have 2 bits to store this.
};

/**
 *  The the case of TYPE_Other, this enum determines the subtype of the token.
 */
enum EProfilingPayloadSubType
{
	// Core marker types

	/** Marker used to determine when malloc profiler stream has ended. */
	SUBTYPE_EndOfStreamMarker					= 0,

	/** Marker used to determine when we need to read data from the next file. */
	SUBTYPE_EndOfFileMarker						= 1,

	/** Marker used to determine when a snapshot has been added. */
	SUBTYPE_SnapshotMarker						= 2,

	/** Marker used to determine when a new frame has started. */
	SUBTYPE_FrameTimeMarker						= 3,

	/** Not used. Only for backward compatibility. Use a new snapshot marker instead. */
	SUBTYPE_TextMarker							= 4,

	// Marker types for periodic non-GMalloc memory status updates. Only for backward compatibility, replaced with SUBTYPE_MemoryAllocationStats

	/** Marker used to store the total amount of memory used by the game. */
	SUBTYPE_TotalUsed							= 5,

	/** Marker used to store the total amount of memory allocated from the OS. */
	SUBTYPE_TotalAllocated						= 6,

	/** Marker used to store the allocated in use by the application virtual memory. */
	SUBTYPE_CPUUsed								= 7,

	/** Marker used to store the allocated from the OS/allocator, but not used by the application. */
	SUBTYPE_CPUSlack							= 8,

	/** Marker used to store the alignment waste from a pooled allocator plus book keeping overhead. */
	SUBTYPE_CPUWaste							= 9,

	/** Marker used to store the allocated in use by the application physical memory. */
	SUBTYPE_GPUUsed								= 10,

	/** Marker used to store the allocated from the OS, but not used by the application. */
	SUBTYPE_GPUSlack							= 11,

	/** Marker used to store the alignment waste from a pooled allocator plus book keeping overhead. */
	SUBTYPE_GPUWaste							= 12,

	/** Marker used to store the overhead of the operating system. */
	SUBTYPE_OSOverhead							= 13,

	/** Marker used to store the size of loaded executable, stack, static, and global object size. */
	SUBTYPE_ImageSize							= 14,

	/// Version 3
	// Marker types for automatic snapshots.

	/** Marker used to determine when engine has started the cleaning process before loading a new level. */
	SUBTYPE_SnapshotMarker_LoadMap_Start		= 21,

	/** Marker used to determine when a new level has started loading. */
	SUBTYPE_SnapshotMarker_LoadMap_Mid			= 22,

	/** Marker used to determine when a new level has been loaded. */
	SUBTYPE_SnapshotMarker_LoadMap_End			= 23,

	/** Marker used to determine when garbage collection has started. */
    SUBTYPE_SnapshotMarker_GC_Start				= 24,

	/** Marker used to determine when garbage collection has finished. */
    SUBTYPE_SnapshotMarker_GC_End		        = 25,

	/** Marker used to determine when a new streaming level has been requested to load. */
	SUBTYPE_SnapshotMarker_LevelStream_Start	= 26,

	/** Marker used to determine when a previously streamed level has been made visible. */
	SUBTYPE_SnapshotMarker_LevelStream_End		= 27,

	/** Marker used to store a generic malloc statistics. @see FMemoryAllocationStats_DEPRECATED. */
	SUBTYPE_MemoryAllocationStats				= 31,

	/** Start licensee-specific subtypes from here. */
	SUBTYPE_LicenseeBase						= 50,

	/** Unknown the subtype of the token. */
	SUBTYPE_Unknown,
};

/** Whether we are performing symbol lookup at runtime or not.					*/
#define SERIALIZE_SYMBOL_INFO PLATFORM_SUPPORTS_STACK_SYMBOLS

/*=============================================================================
	CallStack address information.
=============================================================================*/

/**
 * Helper structure encapsulating a single address/ point in a callstack
 */
struct FCallStackAddressInfo
{
	/** Program counter address of entry.			*/
	uint64	ProgramCounter;
#if SERIALIZE_SYMBOL_INFO
	/** Index into name table for filename.			*/
	int32		FilenameNameTableIndex;
	/** Index into name table for function name.	*/
	int32		FunctionNameTableIndex;
	/** Line number in file.						*/
	int32		LineNumber;
#endif

	/**
	 * Serialization operator.
	 *
	 * @param	Ar			Archive to serialize to
	 * @param	AddressInfo	AddressInfo to serialize
	 * @return	Passed in archive
	 */
	friend FArchive& operator << ( FArchive& Ar, FCallStackAddressInfo AddressInfo )
	{
		Ar	<< AddressInfo.ProgramCounter
#if SERIALIZE_SYMBOL_INFO
			<< AddressInfo.FilenameNameTableIndex
			<< AddressInfo.FunctionNameTableIndex
			<< AddressInfo.LineNumber
#endif
			;
		return Ar;
	}
};

/*=============================================================================
	FMallocProfilerBufferedFileWriter
=============================================================================*/

/**
 * Special buffered file writer, used to serialize data before GFileManager is initialized.
 */
class FMallocProfilerBufferedFileWriter : public FArchive
{
public:
	/** Internal file writer used to serialize to HDD. */
	FArchive*		FileWriter;
	/** Buffered data being serialized before GGameName has been set up. */
	TArray<uint8>	BufferedData;
	/** Timestamped filename with path.	*/
	FString		FullFilepath;
	/** Timestamped file path for the memory captures, just add extension. */
	FString			BaseFilePath;

	/**
	 * Constructor. Called before GMalloc is initialized!!!
	 */
	FMallocProfilerBufferedFileWriter();

	/**
	 * Destructor, cleaning up FileWriter.
	 */
	virtual ~FMallocProfilerBufferedFileWriter();

	// FArchive interface.
	virtual void Serialize( void* V, int64 Length );
	virtual void Seek( int64 InPos );
	virtual bool Close();
	virtual int64 Tell();

	/** Returns the allocated size for use in untracked memory calculations. */
	uint32 GetAllocatedSize();
};

/*=============================================================================
	FMallocProfiler
=============================================================================*/

/** This is an utility class that handles specific malloc profiler mutex locking. */
class FScopedMallocProfilerLock
{	
	/** Copy constructor hidden on purpose. */
	FScopedMallocProfilerLock(FScopedMallocProfilerLock* InScopeLock);

	/** Assignment operator hidden on purpose. */
	FScopedMallocProfilerLock& operator=(FScopedMallocProfilerLock& InScopedMutexLock) { return *this; }

public:
	/** Constructor that performs a lock on the malloc profiler tracking methods. */
	FScopedMallocProfilerLock();

	/** Destructor that performs a release on the malloc profiler tracking methods. */
	~FScopedMallocProfilerLock();
};

/**
 * Memory profiling malloc, routing allocations to real malloc and writing information on all 
 * operations to a file for analysis by a standalone tool.
 */
class CORE_API FMallocProfiler : public FMalloc
{
	friend class	FMallocGcmProfiler;
	friend class	FScopedMallocProfilerLock;
	friend class	FMallocProfilerBufferedFileWriter;

protected:
	/** Malloc we're based on, aka using under the hood												*/
	FMalloc*								UsedMalloc;
	/** Whether or not EndProfiling()  has been Ended.  Once it has been ended it has ended most operations are no longer valid **/
	bool									bEndProfilingHasBeenCalled;
	/** Time malloc profiler was created. Used to convert arbitrary double time to relative float.	*/
	double									StartTime;
	/** File writer used to serialize data.															*/
	FMallocProfilerBufferedFileWriter		BufferedFileWriter;

	/** Critical section to sequence tracking.														*/
	FCriticalSection						CriticalSection;

	/** Mapping from program counter to index in program counter array.								*/
	TMap<uint64,int32>							ProgramCounterToIndexMap;
	/** Array of unique call stack address infos.													*/
	TArray<struct FCallStackAddressInfo>	CallStackAddressInfoArray;

	/** Mapping from callstack CRC to offset in call stack info buffer.								*/
	TMap<uint32,int32>							CRCToCallStackIndexMap;
	/** Buffer of unique call stack infos.															*/
	FCompressedGrowableBuffer				CallStackInfoBuffer;

	/** Mapping from name to index in name array.													*/
	TMap<FString,int32>						NameToNameTableIndexMap;
	/** Array of unique names.																		*/
	TArray<FString>							NameArray;

	/** Whether the output file has been closed. */
	bool									bOutputFileClosed;

	/** Critical section used to detect when malloc profiler is inside one of the tracking functions. */
	FCriticalSection						SyncObject;

	/** Whether operations should be tracked. false e.g. in tracking internal functions.			*/
	int32										SyncObjectLockCount;

	/** The currently executing thread's id. */
	uint32									ThreadId;

	/** Simple count of memory operations															*/
	uint64									MemoryOperationCount;

	/** Returns true if malloc profiler is outside one of the tracking methods, returns false otherwise. */
	bool IsOutsideTrackingFunction() const
	{
		const uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		return (SyncObjectLockCount == 0) || (ThreadId != CurrentThreadId);
	}

	/** 
	 * Returns index of callstack, captured by this function into array of callstack infos. If not found, adds it.
	 *
	 * @return index into callstack array
	 */
	int32 GetCallStackIndex();

	/**
	 * Returns index of passed in program counter into program counter array. If not found, adds it.
	 *
	 * @param	ProgramCounter	Program counter to find index for
	 * @return	Index of passed in program counter if it's not NULL, INDEX_NONE otherwise
	 */
	int32 GetProgramCounterIndex( uint64 ProgramCounter );

	/**
	 * Returns index of passed in name into name array. If not found, adds it.
	 *
	 * @param	Name	Name to find index for
	 * @return	Index of passed in name
	 */
	int32 GetNameTableIndex( const FString& Name );

	/**
	 * Returns index of passed in name into name array. If not found, adds it.
	 *
	 * @param	Name	Name to find index for
	 * @return	Index of passed in name
	 */
	int32 GetNameTableIndex( const FName& Name )
	{
		return GetNameTableIndex(Name.ToString());
	}

	/**
	 * Tracks malloc operation.
	 *
	 * @param	Ptr		Allocated pointer 
	 * @param	Size	Size of allocated pointer
	 */
	void TrackMalloc( void* Ptr, uint32 Size );
	
	/**
	 * Tracks free operation
	 *
	 * @param	Ptr		Freed pointer
	 */
	void TrackFree( void* Ptr );
	
	/**
	 * Tracks realloc operation
	 *
	 * @param	OldPtr	Previous pointer
	 * @param	NewPtr	New pointer
	 * @param	NewSize	New size of allocation
	 */
	void TrackRealloc( void* OldPtr, void* NewPtr, uint32 NewSize );

	/**
	 * Tracks memory allocations stats every 1024 memory operations. Used for time line view in memory profiler app.
	 * Expects to be inside of the critical section
	 */
	void TrackSpecialMemory();

	/**
	 * Ends profiling operation and closes file.
	 */
	void EndProfiling();

	/**
	 * Embeds token into stream to snapshot memory at this point.
	 */
	void SnapshotMemory(EProfilingPayloadSubType SubType, const FString& MarkerName);

	/**
	 * Embeds float token into stream (e.g. delta time).
	 */
	void EmbedFloatMarker(EProfilingPayloadSubType SubType, float DeltaTime);

	/**
	 * Embeds token into stream to snapshot memory at this point.
	 */
	void EmbedDwordMarker(EProfilingPayloadSubType SubType, uint32 Info);

	/** 
	 * Writes additional memory stats for a snapshot like memory allocations stats, list of all loaded levels and platform dependent memory metrics.
	 */
	void WriteAdditionalSnapshotMemoryStats();

	/** 
	 * Writes memory allocations stats. 
	 */
	void WriteMemoryAllocationStats();

	/** 
	 * Writes names of currently loaded levels. 
	 * Only to be called from within the mutex / scope lock of the FMallocProifler.
	 *
	 * @param	InWorld		World Context.
	 */
	virtual void WriteLoadedLevels( UWorld* InWorld );

	/** 
	 * Gather texture memory stats. 
	 */
	virtual void GetTexturePoolSize( FMemoryAllocationStats_DEPRECATED& MemoryStats );

	/** 
		Added for untracked memory calculation
		Note that this includes all the memory used by dependent malloc profilers, such as FMallocGcmProfiler,
		so they don't need their own version of this function. 
	*/
	int32 CalculateMemoryProfilingOverhead();

public:
	/** Snapshot taken when engine has started the cleaning process before loading a new level. */
	static void SnapshotMemoryLoadMapStart(const FString& MapName);

	/** Snapshot taken when a new level has started loading. */
	static void SnapshotMemoryLoadMapMid(const FString& MapName);

	/** Snapshot taken when a new level has been loaded. */
	static void SnapshotMemoryLoadMapEnd(const FString& MapName);

	/** Snapshot taken when garbage collection has started. */
	static void SnapshotMemoryGCStart();

	/** Snapshot taken when garbage collection has finished. */
	static void SnapshotMemoryGCEnd();

	/** Snapshot taken when a new streaming level has been requested to load. */
	static void SnapshotMemoryLevelStreamStart(const FString& LevelName);

	/** Snapshot taken when a previously  streamed level has been made visible. */
	static void SnapshotMemoryLevelStreamEnd(const FString& LevelName);

	/** Returns malloc we're based on. */
	virtual FMalloc* GetInnermostMalloc()
	{ 
		return UsedMalloc;
	}

	/** 
	 * This function is intended to be called when a fatal error has occurred inside the allocator and
	 * you want to dump the current mprof before crashing, so that it can be used to help debug the error.
	 * IMPORTANT: This function assumes that this thread already has the allocator mutex.. 
	 */
	void PanicDump(EProfilingPayloadType FailedOperation, void* Ptr1, void* Ptr2);

	/**
	 * Constructor, initializing all member variables and potentially loading symbols.
	 *
	 * @param	InMalloc	The allocator wrapped by FMallocProfiler that will actually do the allocs/deallocs.
	 */
	FMallocProfiler(FMalloc* InMalloc);

	/**
	 * Begins profiling operation and opens file.
	 */
	void BeginProfiling();

	/** 
	 * QuantizeSize returns the actual size of allocation request likely to be returned
	 * so for the template containers that use slack, they can more wisely pick
	 * appropriate sizes to grow and shrink to.
	 *
	 * CAUTION: QuantizeSize is a special case and is NOT guarded by a thread lock, so must be intrinsically thread safe!
	 *
	 * @param Size			The size of a hypothetical allocation request
	 * @param Alignment		The alignment of a hypothetical allocation request
	 * @return				Returns the usable size that the allocation request would return. In other words you can ask for this greater amount without using any more actual memory.
	 */
	virtual SIZE_T QuantizeSize( SIZE_T Size, uint32 Alignment ) OVERRIDE
	{
		return UsedMalloc->QuantizeSize(Size,Alignment); 
	}

	/** 
	 * Malloc
	 */
	virtual void* Malloc( SIZE_T Size, uint32 Alignment ) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		void* Ptr = UsedMalloc->Malloc( Size, Alignment );
		TrackMalloc( Ptr, (uint32)Size );
		TrackSpecialMemory();
		return Ptr;
	}

	/** 
	 * Realloc
	 */
	virtual void* Realloc( void* OldPtr, SIZE_T NewSize, uint32 Alignment ) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		void* NewPtr = UsedMalloc->Realloc( OldPtr, NewSize, Alignment );
		TrackRealloc( OldPtr, NewPtr, (uint32)NewSize );
		TrackSpecialMemory();
		return NewPtr;
	}

	/** 
	 * Free
	 */
	virtual void Free( void* Ptr ) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->Free( Ptr );
		TrackFree( Ptr );
		TrackSpecialMemory();
	}

	/**
	 * Returns if the allocator is guaranteed to be thread-safe and therefore
	 * doesn't need a unnecessary thread-safety wrapper around it.
	 */
	virtual bool IsInternallyThreadSafe() const OVERRIDE
	{ 
		return true; 
	}

	/**
	 * Passes request for gathering memory allocations for both virtual and physical allocations
	 * on to used memory manager.
	 *
	 * @param FMemoryAllocationStats_DEPRECATED	[out] structure containing information about the size of allocations
	 */
	virtual void GetAllocationInfo( FMemoryAllocationStats_DEPRECATED& MemStats ) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->GetAllocationInfo( MemStats );
	}

	/**
	 * Dumps details about all allocations to an output device
	 *
	 * @param Ar	[in] Output device
	 */
	virtual void DumpAllocations( class FOutputDevice& Ar ) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->DumpAllocations( Ar );
	}

	/**
	 * Validates the allocator's heap
	 */
	virtual bool ValidateHeap() OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		return( UsedMalloc->ValidateHeap() );
	}

	/**
	* If possible determine the size of the memory allocated at the given address
	*
	* @param Original - Pointer to memory we are checking the size of
	* @param SizeOut - If possible, this value is set to the size of the passed in pointer
	* @return true if succeeded
	*/
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) OVERRIDE
	{
		FScopeLock Lock( &CriticalSection );
		return UsedMalloc->GetAllocationSize(Original,SizeOut);
	}

	
	// Begin Exec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE;
	// End Exec Interface

	/** 
	 * Exec command handlers
	 */
	bool HandleMProfCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleDumpAllocsToFileCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleSnapshotMemoryCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleSnapshotMemoryFrameCommand( const TCHAR* Cmd, FOutputDevice& Ar );

	/** Called every game thread tick */
	virtual void Tick( float DeltaTime ) OVERRIDE
	{ 
		FScopeLock Lock( &CriticalSection );
		UsedMalloc->Tick(DeltaTime);
	}

	virtual const TCHAR * GetDescriptiveName() OVERRIDE
	{ 
		FScopeLock ScopeLock( &SynchronizationObject );
		check(UsedMalloc);
		return UsedMalloc->GetDescriptiveName(); 
	}
};

#endif //USE_MALLOC_PROFILER

