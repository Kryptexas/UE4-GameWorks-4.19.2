// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
AsyncTextureStreaming.h: Definitions of classes used for texture streaming async task.
=============================================================================*/

#pragma once

#include "StreamingManagerTexture.h"

/*-----------------------------------------------------------------------------
	Asynchronous texture streaming.
-----------------------------------------------------------------------------*/

/** Async work class for calculating priorities for all textures. */
// this could implement a better abandon, but give how it is used, it does that anyway via the abort mechanism
class FAsyncTextureStreaming : public FNonAbandonableTask
{
public:
	FAsyncTextureStreaming( FStreamingManagerTexture* InStreamingManager )
	:	StreamingManager( *InStreamingManager )
	,	ThreadContext( false, NULL, false )
	,	bAbort( false )
	{
		Reset(false);
	}

	/** Resets the state to start a new async job. */
	void Reset( bool bCollectTextureStats )
	{
		bAbort = false;
		ThreadContext.Reset( false, NULL, bCollectTextureStats );
		ThreadStats.Reset();
		PrioritizedTextures.Empty( StreamingManager.StreamingTextures.Num() );
		PrioTexIndicesSortedByLoadOrder.Empty( StreamingManager.StreamingTextures.Num() );
	}

	/** Notifies the async work that it should abort the thread ASAP. */
	void Abort()
	{
		bAbort = true;
	}

	/** Whether the async work is being aborted. Can be used in conjunction with IsDone() to see if it has finished. */
	bool IsAborted() const
	{
		return bAbort;
	}

	/** Statistics for the async work. */
	struct FAsyncStats
	{
		FAsyncStats()
		{
			Reset();
		}
		/** Resets the statistics to zero. */
		void Reset()
		{
			TotalResidentSize = 0;
			TotalPossibleResidentSize = 0;
			TotalWantedMipsSize = 0;
			TotalTempMemorySize = 0;
			TotalRequiredSize = 0;
			PendingStreamInSize = 0;
			PendingStreamOutSize = 0;
			WantedInSize = 0;
			WantedOutSize = 0;
			NumWantingTextures = 0;
			NumVisibleTexturesWithLowResolutions = 0;
		}
		/** Total number of bytes currently in memory */
		int64 TotalResidentSize;
		/** Total number of bytes that could possibly be used according to the worst case between resident size (current state) and requested size (pending requests) */
		int64 TotalPossibleResidentSize;
		/** Total number of bytes required to satisfy wanted mips.  Not taking into account split requests. */
		int64 TotalWantedMipsSize;
		/** Total number of bytes used for updating textures (conservative value since it is platform specific */
		int64 TotalTempMemorySize;
		/** Total number of bytes required, using PerfectWantedMips */
		int64 TotalRequiredSize;
		/** Currently being streamed in, in bytes. */
		SIZE_T PendingStreamInSize;
		/** Currently being streamed out, in bytes. */
		SIZE_T PendingStreamOutSize;
		/** How much we want to stream in, in bytes. */
		int64 WantedInSize;
		/** How much we want to stream out, in bytes.  */
		int64 WantedOutSize;
		/** Number of textures that want more mips. */
		int64 NumWantingTextures;
		/** Number of textures that are visibles and still low rez. */
		int64 NumVisibleTexturesWithLowResolutions;
	};

	void ClearRemovedTextureReferences();

	/** Returns the resulting priorities, matching the FStreamingManagerTexture::StreamingTextures array. */
	const TArray<FTexturePriority>& GetPrioritizedTextures() const
	{
		return PrioritizedTextures;
	}

	const TArray<int32>& GetPrioTexIndicesSortedByLoadOrder() const
	{
		return PrioTexIndicesSortedByLoadOrder;
	}

	/** Returns the context (temporary info) used for the async work. */
	const FStreamingContext& GetContext() const
	{
		return ThreadContext;
	}

	/** Returns the thread statistics. */
	const FAsyncStats& GetStats() const
	{
		return ThreadStats;
	}

private:
	friend class FAsyncTask<FAsyncTextureStreaming>;
	/** Performs the async work. */
	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncTextureStreaming, STATGROUP_ThreadPoolAsyncTasks);
	}

	/** Reference to the owning streaming manager, for accessing the thread-safe data. */
	FStreamingManagerTexture&	StreamingManager;
	/** Resulting priorities for keeping textures in memory and controlling load order, matching the FStreamingManagerTexture::StreamingTextures array. */
	TArray<FTexturePriority>	PrioritizedTextures;
	/** Indices for PrioritizedTextures, sorted by load order. */
	TArray<int32>	PrioTexIndicesSortedByLoadOrder;

	/** Context (temporary info) used for the async work. */
	FStreamingContext			ThreadContext;
	/** Thread statistics. */
	FAsyncStats				ThreadStats;
	/** Whether the async work should abort its processing. */
	volatile bool				bAbort;
};
