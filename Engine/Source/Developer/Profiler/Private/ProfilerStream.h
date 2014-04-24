// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*-----------------------------------------------------------------------------
	Basic structures
-----------------------------------------------------------------------------*/

/** Profiler stack node, used to store the whole call stack for one frame. */
struct FProfilerStackNode
{
	FProfilerStackNode* Parent;
	TArray<FProfilerStackNode*> Children;

	/** Short name. */
	FName StatName;
	FName LongName;

	int64 CyclesStart;
	int64 CyclesEnd;

	double TimeMSStart;
	double TimeMSEnd;

	/** Index of this node in the data provider's collection. */
	uint32 SampleIndex;

	/** Initializes thread root node. */
	FProfilerStackNode()
		: Parent( nullptr )
		, StatName( FStatConstants::NAME_ThreadRoot )
		, LongName( FStatConstants::NAME_ThreadRoot )
		, CyclesStart( 0 )
		, CyclesEnd( 0 )
		, TimeMSStart( 0.0f )
		, TimeMSEnd( 0.0f )
		, SampleIndex( 0 )
	{}

	/** Initializes children node. */
	FProfilerStackNode( FProfilerStackNode* InParent, const FStatMessage& StatMessage, uint32 InSampleIndex )
		: Parent( InParent )
		, StatName( StatMessage.NameAndInfo.GetShortName() )
		, LongName( StatMessage.NameAndInfo.GetRawName() )
		, CyclesStart( StatMessage.GetValue_int64() )
		, CyclesEnd( 0 )
		, TimeMSStart( 0.0f )
		, TimeMSEnd( 0.0f )
		, SampleIndex( InSampleIndex )
	{}

	~FProfilerStackNode()
	{
		for( const auto& Child : Children )
		{
			delete Child;
		}
	}

	/** Calculates allocated size by all stacks node, only valid for the thread root node. */
	int64 GetAllocatedSize() const
	{
		int64 AllocatedSize = sizeof(*this) + Children.GetAllocatedSize();
		for( const auto& StackNode : Children )
		{
			AllocatedSize += StackNode->GetAllocatedSize();
		}
		return AllocatedSize;
	}
};

enum
{
	STACKNODE_INVALID = 0,
	STACKNODE_VALID = 1,
};

/** Profiler frame. */
struct FProfilerFrame
{
	/** Root node. */
	FProfilerStackNode* Root;

	/** Thread times in milliseconds for this frame. */
	TMap<uint32, float> ThreadTimesMS;

	// @TODO yrx 2014-04-22 Non-frame stats like accumulator, counters, memory etc?

	/**
	*	Indicates whether this node is in the memory, move to thread root node later.
	*	Valid only for the root stack node.
	*	This set by one thread and accessed by another thread, there is no thread contention.
	*/
	FThreadSafeCounter AccessLock;

	FProfilerFrame()
		: Root( new FProfilerStackNode() )
		, AccessLock( STACKNODE_INVALID )
	{}

	~FProfilerFrame()
	{
		FreeMemory();
	}

	void MarkAsValid()
	{
		const int32 OldLock = AccessLock.Set( STACKNODE_VALID );
		check( OldLock == STACKNODE_INVALID );
	}

	void MarkAsInvalid()
	{
		const int32 OldLock = AccessLock.Set( STACKNODE_INVALID );
		check( OldLock == STACKNODE_VALID );
	}

	bool IsValid() const
	{
		return AccessLock.GetValue() == STACKNODE_VALID;
	}

	int64 GetAllocatedSize() const
	{
		return ThreadTimesMS.GetAllocatedSize() + Root ? Root->GetAllocatedSize() : 0;
	}

	void FreeMemory()
	{
		delete Root;
		Root = nullptr;

		ThreadTimesMS.Empty();
	}
};

/** Contains all processed profiler's frames. */
// @TODO yrx 2014-04-22 Array frames to real frames conversion.
// @TODO yrx 2014-04-22 Can be done fully lock free and thread safe, but later.
struct FProfilerStream
{
protected:

	/** History frames collected so far or read from the file. */
	TChunkedArray<FProfilerFrame*> Frames;

	/** Critical section. */
	mutable FCriticalSection CriticalSection;


public:
	void AddProfilerFrame( FProfilerFrame* ProfilerFrame )
	{
		FScopeLock Lock( &CriticalSection );
		Frames.AddElement( ProfilerFrame );
	}

	/**
	* @return a pointer to the profiler frame, once obtained can be used until end of the profiler session.
	*/
	FProfilerFrame* GetProfilerFrame( int32 FrameIndex ) const
	{
		FScopeLock Lock( &CriticalSection );
		return Frames[FrameIndex];
	}

	int64 GetAllocatedSize() const
	{
		FScopeLock Lock( &CriticalSection );
		int64 AllocatedSize = 0;

		for( int32 FrameIndex = 0; FrameIndex < Frames.Num(); FrameIndex++ )
		{
			const FProfilerFrame* ProfilerFrame = Frames[FrameIndex];
			if( ProfilerFrame->IsValid() )
			{
				AllocatedSize += ProfilerFrame->GetAllocatedSize();
			}
		}

		return AllocatedSize;
	}
};

/**
*	Profiler UI stack node.
*	Similar to the profiler stack node, but contains data prepared and optimized for the UI
*/
struct FProfilerUIStackNode
{
	/** Frames used to generate this data, at least 1 frame. */
	TArray<int64> Frames;

	FProfilerUIStackNode* Parent;
	TArray<FProfilerUIStackNode*> Children;

	/** Short name. */
	FName StatName;
	FName LongName;

	double TimeMSStart;
	double TimeMSEnd;
};


struct FProfilerUIStream
{
	TArray<FProfilerFrame*> Frames;
};

