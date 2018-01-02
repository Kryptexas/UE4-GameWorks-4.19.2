// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StereoRendering.h: Abstract stereoscopic rendering interface
=============================================================================*/

#pragma once

#include "CoreMinimal.h"


/** Render thread proxy that hold's the heuristic for dynamic resolution. */
class ENGINE_API FDynamicResolutionHeuristicProxy
{
public:
	static constexpr uint64 kInvalidEntryId = ~uint64(0);


	FDynamicResolutionHeuristicProxy();
	~FDynamicResolutionHeuristicProxy();


	/** Resets the proxy. */
	void Reset_RenderThread();

	/** Create a new previous frame and feeds its timings and returns history's unique id. */
	uint64 CreateNewPreviousFrameTimings_RenderThread(float GameThreadTimeMs, float RenderThreadTimeMs);

	/** Commit GPU busy times
	 *
	 *	@param	HistoryFrameId The history's unique id for the frame returned by CreateNewPreviousFrameTimings_RenderThread()
	 *	@param	TotalFrameGPUBusyTimeMs Total GPU busy time for the frame.
	 *	@param	DynamicResolutionGPUBusyTimeMs GPU time for sub parts of the frame that does dynamic resolution.
	 */
	void CommitPreviousFrameGPUTimings_RenderThread(
		uint64 HistoryFrameId, float TotalFrameGPUBusyTimeMs, float DynamicResolutionGPUBusyTimeMs, bool bGPUTimingsHaveCPUBubbles);

	/** Refresh resolution fraction's from history. */
	void RefreshCurentFrameResolutionFraction_RenderThread();

	/** Returns the view fraction upper bound. */
	float GetResolutionFractionUpperBound() const;

	/** Returns the view fraction that should be used for current frame. */
	FORCEINLINE float QueryCurentFrameResolutionFraction_RenderThread() const
	{
		return FMath::Min(CurrentFrameResolutionFraction, GetResolutionFractionUpperBound());
	}

	/** Returns a non thread safe approximation of the current resolution fraction applied on render thread. */
	FORCEINLINE float GetResolutionFractionApproximation_GameThread() const
	{
		return FMath::Min(CurrentFrameResolutionFraction, GetResolutionFractionUpperBound());
	}


	/** Creates a default dynamic resolution state using this proxy that queries GPU timing from the RHI. */
	static TSharedPtr< class IDynamicResolutionState > CreateDefaultState();


private:
	struct FrameHistoryEntry
	{
		// The resolution fraction the frame was rendered with.
		float ResolutionFraction;

		// Thread timings in milliseconds.
		float GameThreadTimeMs;
		float RenderThreadTimeMs;

		// Total GPU busy time for the entire frame in milliseconds.
		float TotalFrameGPUBusyTimeMs;

		// Total GPU busy time for the render thread commands that does dynamic resolutions.
		float GlobalDynamicResolutionTimeMs;

		// Whether GPU timings have GPU bubbles.
		bool bGPUTimingsHaveCPUBubbles;


		inline FrameHistoryEntry()
			: ResolutionFraction(1.0f)
			, GameThreadTimeMs(-1.0f)
			, RenderThreadTimeMs(-1.0f)
			, TotalFrameGPUBusyTimeMs(-1.0f)
			, GlobalDynamicResolutionTimeMs(-1.0f)
			, bGPUTimingsHaveCPUBubbles(true)
		{ }


		// Returns whether GPU timings have landed.
		inline bool HasGPUTimings() const
		{
			return TotalFrameGPUBusyTimeMs >= 0.0f;
		}
	};

	// Circular buffer of the history.
	// We don't use TCircularBuffer because does not support resizes.
	TArray<FrameHistoryEntry> History;
	int32 PreviousFrameIndex;
	int32 HistorySize;

	// Counts the number of frame since the last screen percentage change.
	int32 NumberOfFramesSinceScreenPercentageChange;

	// Number of frame remaining to ignore.
	int32 IgnoreFrameRemainingCount;

	// Current frame's view fraction.
	float CurrentFrameResolutionFraction;

	// Frame counter to allocate unique ID for CommitPreviousFrameGPUTimings_RenderThread().
	uint64 FrameCounter;


	inline const FrameHistoryEntry& GetPreviousFrameEntry(int32 BrowsingFrameId) const
	{
		if (BrowsingFrameId < 0 || BrowsingFrameId >= HistorySize)
		{
			static const FrameHistoryEntry InvalidEntry;
			return InvalidEntry;
		}
		return History[(History.Num() + PreviousFrameIndex - BrowsingFrameId) % History.Num()];
	}

	void ResetInternal();

	void ResizeHistoryIfNeeded();
};

