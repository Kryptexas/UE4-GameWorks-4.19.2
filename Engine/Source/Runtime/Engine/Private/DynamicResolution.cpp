// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
Renderer.cpp: Renderer module implementation.
=============================================================================*/

#include "DynamicResolutionProxy.h"
#include "DynamicResolutionState.h"

#include "Engine/Engine.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "RenderCore.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "DynamicRHI.h"
#include "UnrealEngine.h"


static TAutoConsoleVariable<float> CVarDynamicResMinSP(
	TEXT("r.DynamicRes.MinScreenPercentage"),
	50,
	TEXT("Minimal screen percentage."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarDynamicResMaxSP(
	TEXT("r.DynamicRes.MaxScreenPercentage"),
	100,
	TEXT("Maximal screen percentage."),
	ECVF_Default);

// TODO: Seriously need a centralized engine perf manager.
static TAutoConsoleVariable<float> CVarFrameTimeBudget(
	TEXT("r.DynamicRes.FrameTimeBudget"),
	33.3f,
	TEXT("Frame's time budget in milliseconds."),
	ECVF_RenderThreadSafe | ECVF_Default);



static TAutoConsoleVariable<float> CVarTargetedGPUHeadRoomPercentage(
	TEXT("r.DynamicRes.TargetedGPUHeadRoomPercentage"),
	10.0f,
	TEXT("Targeted GPU headroom (in percent from r.DynamicRes.FrameTimeBudget)."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarCPUTimeHeadRoom(
	TEXT("r.DynamicRes.CPUTimeHeadRoom"),
	1,
	TEXT("Head room for the threads compared GPU time to avoid keep getting resolution fraction ")
	TEXT("shrinking down when CPU bound (in milliseconds)."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<int32> CVarHistorySize(
	TEXT("r.DynamicRes.HistorySize"),
	16,
	TEXT("Number of frames keept in the history."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarOutlierThreshold(
	TEXT("r.DynamicRes.OutlierThreshold"),
	0.0f,
	TEXT("Ignore frame timing that have Game thread or render thread X time more than frame budget."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarFrameWeightExponent(
	TEXT("r.DynamicRes.FrameWeightExponent"),
	0.9f,
	TEXT("Recursive weight of frame N-1 against frame N."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<int32> CVarFrameChangePeriod(
	TEXT("r.DynamicRes.MinResolutionChangePeriod"),
	8,
	TEXT("Minimal number of frames between resolution changes, important to avoid input ")
	TEXT("sample position interferences in TAA upsample."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarIncreaseAmortizationFactor(
	TEXT("r.DynamicRes.IncreaseAmortizationBlendFactor"),
	0.9f,
	TEXT("Amortization blend factor when scale resolution back up to reduce resolution fraction oscillations."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarChangeThreshold(
	TEXT("r.DynamicRes.ChangePercentageThreshold"),
	2.0f,
	TEXT("Minimal increase percentage threshold to alow when changing resolution."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<int32> CVarMaxConsecutiveOverbudgetGPUFrameCount(
	TEXT("r.DynamicRes.MaxConsecutiveOverbudgetGPUFrameCount"),
	2,
	TEXT("Maximum number of consecutive frame tolerated over GPU budget."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<int32> CVarTimingMeasureModel(
	TEXT("r.DynamicRes.GPUTimingMeasureMethod"),
	0,
	TEXT("Selects the method to use to measure GPU timings.\n")
	TEXT(" 0: Same as stat unit (default);\n 1: Timestamp queries."),
	ECVF_RenderThreadSafe | ECVF_Default);

static TAutoConsoleVariable<float> CVarCPUBoundScreenPercentage(
	TEXT("r.DynamicRes.CPUBoundScreenPercentage"),
	100,
	TEXT("Screen percentage to converge to when CPU bound. This can be used when GPU and CPU share same memory."),
	ECVF_RenderThreadSafe | ECVF_Default);


FDynamicResolutionHeuristicProxy::FDynamicResolutionHeuristicProxy()
{
	check(IsInGameThread());

	FrameCounter = 0;

	ResetInternal();
}

FDynamicResolutionHeuristicProxy::~FDynamicResolutionHeuristicProxy()
{
	check(IsInRenderingThread());
}

void FDynamicResolutionHeuristicProxy::Reset_RenderThread()
{
	check(IsInRenderingThread());

	ResetInternal();
}

void FDynamicResolutionHeuristicProxy::ResetInternal()
{
	PreviousFrameIndex = -1;
	HistorySize = 0;
	History.Reset();

	NumberOfFramesSinceScreenPercentageChange = 0;
	CurrentFrameResolutionFraction = 1.0f;

	// Ignore previous frame timings.
	IgnoreFrameRemainingCount = 1;
}

uint64 FDynamicResolutionHeuristicProxy::CreateNewPreviousFrameTimings_RenderThread(
	float GameThreadTimeMs, float RenderThreadTimeMs)
{
	check(IsInRenderingThread());

	// Early return if want to ignore frames.
	if (IgnoreFrameRemainingCount > 0)
	{
		IgnoreFrameRemainingCount--;
		return FDynamicResolutionHeuristicProxy::kInvalidEntryId;
	}

	ResizeHistoryIfNeeded();

	// Update history state.
	{
		int32 NewHistoryEntryIndex = (PreviousFrameIndex + 1) % History.Num();
		History[NewHistoryEntryIndex] = FrameHistoryEntry();
		History[NewHistoryEntryIndex].ResolutionFraction = CurrentFrameResolutionFraction;
		History[NewHistoryEntryIndex].GameThreadTimeMs = GameThreadTimeMs;
		History[NewHistoryEntryIndex].RenderThreadTimeMs = RenderThreadTimeMs;
		PreviousFrameIndex = NewHistoryEntryIndex;
		HistorySize = FMath::Min(HistorySize + 1, History.Num());
	}

	return ++FrameCounter;
}

void FDynamicResolutionHeuristicProxy::CommitPreviousFrameGPUTimings_RenderThread(
	uint64 HistoryFrameId, float TotalFrameGPUBusyTimeMs, float DynamicResolutionGPUBusyTimeMs, bool bGPUTimingsHaveCPUBubbles)
{
	check(TotalFrameGPUBusyTimeMs >= 0.0f);
	check(DynamicResolutionGPUBusyTimeMs >= 0.0f);

	// Conveniently early return if invalid history frame id.
	if (HistoryFrameId == FDynamicResolutionHeuristicProxy::kInvalidEntryId ||
		HistoryFrameId <= (FrameCounter - HistorySize))
	{
		return;
	}

	int32 EntryId = (History.Num() + PreviousFrameIndex - int32(FrameCounter - HistoryFrameId)) % History.Num();

	// Make sure not overwriting.
	check(History[EntryId].TotalFrameGPUBusyTimeMs == -1.0f);

	History[EntryId].TotalFrameGPUBusyTimeMs = TotalFrameGPUBusyTimeMs;
	History[EntryId].GlobalDynamicResolutionTimeMs = DynamicResolutionGPUBusyTimeMs;
	History[EntryId].bGPUTimingsHaveCPUBubbles = bGPUTimingsHaveCPUBubbles;
}

void FDynamicResolutionHeuristicProxy::RefreshCurentFrameResolutionFraction_RenderThread()
{
	// GPU time budget per frame=.
	const float FrameTimeBudgetMs = CVarFrameTimeBudget.GetValueOnRenderThread();

	// Targeted GPU time, lower than budget to diggest noise.
	const float TargetedGPUBusyTimeMs = FrameTimeBudgetMs * (1.0f - CVarTargetedGPUHeadRoomPercentage.GetValueOnRenderThread() / 100.0f);

	// Resolution fraction to downscale to when CPU bound.
	const float CPUBoundResolutionFraction = CVarCPUBoundScreenPercentage.GetValueOnRenderThread() / 100.0f;

	// New ResolutionFraction to use for this frame.
	float NewFrameResolutionFraction = CurrentFrameResolutionFraction;

	// Whether there is a GPU over budget panic.
	bool bGPUOverbugetPanic = false;

	// Compute NewFrameResolutionFraction if have an history to work with.
	if (HistorySize > 0)
	{
		const float FrameWeightExponent = CVarFrameWeightExponent.GetValueOnRenderThread();
		const float RenderThreadHeadRoomFromGPUMs = CVarCPUTimeHeadRoom.GetValueOnRenderThread();
		const int32 MaxConsecutiveOverbudgetGPUFrameCount = FMath::Max(CVarMaxConsecutiveOverbudgetGPUFrameCount.GetValueOnRenderThread(), 2);
		
		const float OutLierTimeThreshold = FrameTimeBudgetMs * CVarOutlierThreshold.GetValueOnRenderThread();

		NewFrameResolutionFraction = 0.0f;

		// Total weight of NewFrameResolutionFraction.
		float TotalWeight = 0.0f;
		
		// Frame weight.
		float Weight = 1.0f;

		// Number of consecutive frames that have over budget GPU.
		int32 ConsecutiveOverbudgetGPUFramCount = 0;

		// Whether should continue browsing the frame history.
		bool bCaryOnBrowsingFrameHistory = true;

		// Number of frame browsed.
		int32 FrameCount = 0;
		int32 BrowsingFrameId = 0;
		for (; BrowsingFrameId < HistorySize && bCaryOnBrowsingFrameHistory; BrowsingFrameId++)
		{
			const FrameHistoryEntry& NextFrameEntry = GetPreviousFrameEntry(BrowsingFrameId - 1);
			const FrameHistoryEntry& FrameEntry = GetPreviousFrameEntry(BrowsingFrameId + 0);
			const FrameHistoryEntry& PreviousFrameEntry = GetPreviousFrameEntry(BrowsingFrameId + 1);

			// Ignores frames that does not have any GPU timing yet. 
			if (FrameEntry.TotalFrameGPUBusyTimeMs < 0)
			{
				continue;
			}

			// Compute the maximum thread time between game and render thread for this <i>th frame.
			// We add a head room for render thread to detect when the GPU could be starving.
			const float MaxThreadTimeMs = FMath::Max(
				FrameEntry.RenderThreadTimeMs + (FrameEntry.bGPUTimingsHaveCPUBubbles ? RenderThreadHeadRoomFromGPUMs : 0),
				FrameEntry.GameThreadTimeMs);

			// Whether bound by game thread.
			const bool bIsGameThreadBound = FrameEntry.GameThreadTimeMs > FrameTimeBudgetMs;

			// Whether bound by render thread.
			const bool bIsRenderThreadBound = (FrameEntry.RenderThreadTimeMs + (FrameEntry.bGPUTimingsHaveCPUBubbles ? RenderThreadHeadRoomFromGPUMs : 0)) > TargetedGPUBusyTimeMs;

			// Whether the frame is CPU bound.
			const bool bIsCPUBound = bIsGameThreadBound || bIsRenderThreadBound;
			
			// Whether the render thread might create bubbles in GPU timings.
			const bool bMayHaveGPUBubbles = FrameEntry.bGPUTimingsHaveCPUBubbles && bIsRenderThreadBound;

			// Whether GPU is over budget, when not CPU bound.
			const bool bHasOverbudgetGPU = !bIsCPUBound && FrameEntry.TotalFrameGPUBusyTimeMs > FrameTimeBudgetMs;

			// Whether this is possibly an outlier compared to previous and next frame.
			// Normally should look for out lier compared to previous and next frames timings, however
			// only interested in detected them if they are over budget, when cruising at targeted
			// GPU busy time.
			const bool bIsGPUOutlier =
				bHasOverbudgetGPU &&
				ConsecutiveOverbudgetGPUFramCount == 0 &&
				OutLierTimeThreshold > 0.0f &&
				FrameEntry.TotalFrameGPUBusyTimeMs > OutLierTimeThreshold &&
				(!NextFrameEntry.HasGPUTimings() || (
					NextFrameEntry.TotalFrameGPUBusyTimeMs < OutLierTimeThreshold &&
					NextFrameEntry.GameThreadTimeMs < OutLierTimeThreshold &&
					NextFrameEntry.RenderThreadTimeMs < OutLierTimeThreshold)) &&
				(!PreviousFrameEntry.HasGPUTimings() || (
					PreviousFrameEntry.TotalFrameGPUBusyTimeMs < OutLierTimeThreshold &&
					PreviousFrameEntry.GameThreadTimeMs < OutLierTimeThreshold &&
					PreviousFrameEntry.RenderThreadTimeMs < OutLierTimeThreshold));

			// Look if this is multiple consecutive GPU over budget frames.
			if (bHasOverbudgetGPU)
			{
				ConsecutiveOverbudgetGPUFramCount++;
				check(ConsecutiveOverbudgetGPUFramCount <= MaxConsecutiveOverbudgetGPUFrameCount);

				// Max number of over budget frames where reached.
				if (ConsecutiveOverbudgetGPUFramCount == MaxConsecutiveOverbudgetGPUFrameCount)
				{
					// We ignore frames in history that happen before consecutive GPU overbudget frames.
					bCaryOnBrowsingFrameHistory = false;

					ensureMsgf(!bIsGPUOutlier, TEXT("Does not make any sens to have ConsecutiveOverbudgetGPUFramCount >= 2 if this frame is detected as being an outlier."));
				}
			}
			else
			{
				ConsecutiveOverbudgetGPUFramCount = 0;
			}

			// Ignore if an outlier.
			if (bIsGPUOutlier)
			{
				continue;
			}

			float SuggestedResolutionFraction = 1.0f;

			// If have reliable GPU times, or guess there is not GPU bubbles -> estimate the suggested resolution fraction that could have been used.
			if (!FrameEntry.bGPUTimingsHaveCPUBubbles ||
				(FrameEntry.bGPUTimingsHaveCPUBubbles && !bIsCPUBound && !bMayHaveGPUBubbles))
			{
				// This assumes GPU busy time is directly proportional to ResolutionFraction^2, but in practice
				// this is more A * ResolutionFraction^2 + B with B >= 0 non constant unknown cost such as unscaled
				// post processing, vertex fetching & processing, occlusion queries, shadow maps rendering...
				//
				// This assumption means we may drop ResolutionFraction lower than needed, or be slower to increase
				// resolution.
				//
				// TODO: If we have RHI guarantee of frame timing association, we could make an estimation of B
				// At resolution change that happen every N frames, amortized over time and scaled down as the
				// standard variation of the GPU timing over non resolution changing frames increases.
				SuggestedResolutionFraction = (
					FMath::Sqrt(TargetedGPUBusyTimeMs / FrameEntry.TotalFrameGPUBusyTimeMs)
					* FrameEntry.ResolutionFraction);
			}
			else if (FrameEntry.ResolutionFraction > CPUBoundResolutionFraction)
			{
				// When CPU bound, we no longer want to downscale, or drop resolution if CPU and GPU are sharing same memory.
				SuggestedResolutionFraction = CPUBoundResolutionFraction;
			}
			else
			{
				// Unable to know reliably the GPU time so don't try to make any prediction to avoid issues such
				// as resolution keep shrinking because GPU time is slightly more than render thread time, or
				// increasing because GPU time slightly lower than frame time.
				SuggestedResolutionFraction = FrameEntry.ResolutionFraction;
			}

			NewFrameResolutionFraction += SuggestedResolutionFraction * Weight;
			TotalWeight += Weight;
			FrameCount++;

			Weight *= FrameWeightExponent;
		}

		NewFrameResolutionFraction /= TotalWeight;

		// If immediate previous frames where over budget, react immediately.
		bGPUOverbugetPanic = FrameCount > 0 && ConsecutiveOverbudgetGPUFramCount == FrameCount;

		// If over budget, reset history size to 0 so that this frame really behave as a first frame after an history reset.
		if (bGPUOverbugetPanic)
		{
			HistorySize = 0;
		}
		// If not immediately over budget, refine the new resolution fraction.
		else
		{
			// If scaling the resolution, look if this is above a threshold compared to current res.
			if (CVarChangeThreshold.GetValueOnRenderThread())
			{
				float IncreaseResolutionFractionThreshold = CurrentFrameResolutionFraction * (1.0f + CVarChangeThreshold.GetValueOnRenderThread() / 100.0f);

				float DecreaseResolutionFractionThreshold = CurrentFrameResolutionFraction * (1.0f - CVarChangeThreshold.GetValueOnRenderThread() / 100.0f);

				if (NewFrameResolutionFraction > CurrentFrameResolutionFraction && NewFrameResolutionFraction < IncreaseResolutionFractionThreshold)
				{
					NewFrameResolutionFraction = CurrentFrameResolutionFraction;
				}
			}

			// If scaling the resolution up, amortize to avoid oscillations.
			if (NewFrameResolutionFraction > CurrentFrameResolutionFraction)
			{
				NewFrameResolutionFraction = FMath::Lerp(
					CurrentFrameResolutionFraction,
					NewFrameResolutionFraction,
					CVarIncreaseAmortizationFactor.GetValueOnRenderThread());
			}
		}

		// Clamp resolution fraction.
		NewFrameResolutionFraction = FMath::Clamp(
			NewFrameResolutionFraction,
			CVarDynamicResMinSP.GetValueOnRenderThread() / 100.0f,
			GetResolutionFractionUpperBound());
	}

	// Update the current frame's resolution fraction.
	{
		// CVarChangeThreshold avoids very small changes.
		bool bWouldBeWorthChangingRes = CurrentFrameResolutionFraction != NewFrameResolutionFraction;

		// We do not change resolution too often to avoid interferences with temporal sub pixel in TAA upsample.
		if ((bWouldBeWorthChangingRes && 
			NumberOfFramesSinceScreenPercentageChange >= CVarFrameChangePeriod.GetValueOnRenderThread()) ||
			bGPUOverbugetPanic)
		{
			NumberOfFramesSinceScreenPercentageChange = 0;
			CurrentFrameResolutionFraction = NewFrameResolutionFraction;
		}
		else
		{
			NumberOfFramesSinceScreenPercentageChange++;
		}
	}
}

float FDynamicResolutionHeuristicProxy::GetResolutionFractionUpperBound() const
{
	return CVarDynamicResMaxSP.GetValueOnAnyThread() / 100.0f;
}

void FDynamicResolutionHeuristicProxy::ResizeHistoryIfNeeded()
{
	uint32 DesiredHistorySize = FMath::Max(1, CVarHistorySize.GetValueOnRenderThread());

	if (History.Num() == DesiredHistorySize)
	{
		return;
	}

	TArray<FrameHistoryEntry> NewHistory;
	NewHistory.SetNum(DesiredHistorySize);

	int32 NewHistorySize = FMath::Min(HistorySize, NewHistory.Num());
	int32 NewPreviousFrameIndex = NewHistorySize - 1;

	for (int32 i = 0; i < NewHistorySize; i++)
	{
		NewHistory[NewPreviousFrameIndex - i] = History[(PreviousFrameIndex - i) % History.Num()];
	}

	History = NewHistory;
	HistorySize = NewHistorySize;
	PreviousFrameIndex = NewPreviousFrameIndex;
}


/**
 * Engine's default dynamic resolution driver for view families.
 */
class FDefaultDynamicResolutionDriver : public ISceneViewFamilyScreenPercentage
{
public:

	FDefaultDynamicResolutionDriver(const FDynamicResolutionHeuristicProxy* InProxy, const FSceneViewFamily& InViewFamily)
		: Proxy(InProxy)
		, ViewFamily(InViewFamily)
	{
		check(IsInGameThread());
	}

	virtual float GetPrimaryResolutionFractionUpperBound() const override
	{
		if (!ViewFamily.EngineShowFlags.ScreenPercentage)
		{
			return 1.0f;
		}

		return Proxy->GetResolutionFractionUpperBound();
	}

	virtual ISceneViewFamilyScreenPercentage* Fork_GameThread(const class FSceneViewFamily& ForkedViewFamily) const override
	{
		check(IsInGameThread());

		return new FDefaultDynamicResolutionDriver(Proxy, ForkedViewFamily);
	}

	virtual void ComputePrimaryResolutionFractions_RenderThread(TArray<FSceneViewScreenPercentageConfig>& OutViewScreenPercentageConfigs) const override
	{
		check(IsInRenderingThread());

		if (!ViewFamily.EngineShowFlags.ScreenPercentage)
		{
			return;
		}

		float GlobalResolutionFraction = Proxy->QueryCurentFrameResolutionFraction_RenderThread();

		for (int32 i = 0; i < OutViewScreenPercentageConfigs.Num(); i++)
		{
			OutViewScreenPercentageConfigs[i].PrimaryResolutionFraction = GlobalResolutionFraction;
		}
	}

private:
	// Dynamic resolution proxy to use.
	const FDynamicResolutionHeuristicProxy* Proxy;

	// View family to take care of.
	const FSceneViewFamily& ViewFamily;

};


/**
 * Render thread proxy for engine's dynamic resolution state.
 */
class FDefaultDynamicResolutionStateProxy
{
public:
	FDefaultDynamicResolutionStateProxy()
	{
		check(IsInGameThread());
		InFlightFrames.SetNum(4);
		CurrentFrameInFlightIndex = -1;
		bUseTimeQueriesThisFrame = false;
	}

	void Reset()
	{
		check(IsInRenderingThread());

		// Reset heuristic.
		Heuristic.Reset_RenderThread();

		// Set invalid heuristic's entry id on all inflight frames.
		for (auto& InFlightFrame : InFlightFrames)
		{
			InFlightFrame.HeuristicHistoryEntry = FDynamicResolutionHeuristicProxy::kInvalidEntryId;
		}
	}

	void BeginFrame(FRHICommandList& RHICmdList, float PrevGameThreadTimeMs)
	{
		check(IsInRenderingThread());

		// Query render thread time Ms.
		float PrevRenderThreadTimeMs = FPlatformTime::ToMilliseconds(GRenderThreadTime);

		bUseTimeQueriesThisFrame = GSupportsTimestampRenderQueries && CVarTimingMeasureModel.GetValueOnRenderThread() == 1;

		if (bUseTimeQueriesThisFrame)
		{
			// Process to the in-flight frames that had their queries fully landed.
			HandLandedQueriesToHeuristic(/* bWait = */ false);

			// Finds a new CurrentFrameInFlightIndex that does not have any pending queries.
			FindNewInFlightIndex();

			InFlightFrameQueries& InFlightFrame = InFlightFrames[CurrentFrameInFlightIndex];

			// Feed the thread timings to the heuristic.
			InFlightFrame.HeuristicHistoryEntry = Heuristic.CreateNewPreviousFrameTimings_RenderThread(
				PrevGameThreadTimeMs, PrevRenderThreadTimeMs);

			{
				InFlightFrame.BeginFrameQuery = RHICreateRenderQuery(RQT_AbsoluteTime);
				RHICmdList.EndRenderQuery(InFlightFrame.BeginFrameQuery);
			}
		}
		else
		{
			// If RHI does not support GPU busy time queries, fall back to what stat unit does.
			float PrevFrameGPUTimeMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());

			uint64 HistoryEntryId = Heuristic.CreateNewPreviousFrameTimings_RenderThread(
				PrevGameThreadTimeMs, PrevRenderThreadTimeMs);

			Heuristic.CommitPreviousFrameGPUTimings_RenderThread(HistoryEntryId,
				/* TotalFrameGPUBusyTimeMs = */ PrevFrameGPUTimeMs,
				/* DynamicResolutionGPUBusyTimeMs = */ PrevFrameGPUTimeMs,
				/* bGPUTimingsHaveCPUBubbles = */ !GRHISupportsFrameCyclesBubblesRemoval);

			Heuristic.RefreshCurentFrameResolutionFraction_RenderThread();

			// Set a non insane value for internal checks to pass as if GRHISupportsGPUBusyTimeQueries == true.
			CurrentFrameInFlightIndex = 0;
		}
	}

	void ProcessEvent(FRHICommandList& RHICmdList, EDynamicResolutionStateEvent Event)
	{
		check(IsInRenderingThread());

		if (bUseTimeQueriesThisFrame)
		{
			InFlightFrameQueries& InFlightFrame = InFlightFrames[CurrentFrameInFlightIndex];

			FRenderQueryRHIRef* QueryPtr = nullptr;
			switch (Event)
			{
			case EDynamicResolutionStateEvent::BeginDynamicResolutionRendering:
				QueryPtr = &InFlightFrame.BeginDynamicResolutionQuery; break;
			case EDynamicResolutionStateEvent::EndDynamicResolutionRendering:
				QueryPtr = &InFlightFrame.EndDynamicResolutionQuery; break;
			case EDynamicResolutionStateEvent::EndFrame:
				QueryPtr = &InFlightFrame.EndFrameQuery; break;
			default: check(0);
			}

			*QueryPtr = RHICreateRenderQuery(RQT_AbsoluteTime);
			RHICmdList.EndRenderQuery(*QueryPtr);
		}

		// Clobber CurrentFrameInFlightIndex for internal checks.
		if (Event == EDynamicResolutionStateEvent::EndFrame)
		{
			CurrentFrameInFlightIndex = -1;
			bUseTimeQueriesThisFrame = false;
		}
	}

	void Finish()
	{
		check(IsInRenderingThread());

		// Wait for all queries to land.
		HandLandedQueriesToHeuristic(/* bWait = */ true);
	}


	// Heuristic's proxy.
	FDynamicResolutionHeuristicProxy Heuristic;


private:
	struct InFlightFrameQueries
	{
		// GPU queries.
		FRenderQueryRHIRef BeginFrameQuery;
		FRenderQueryRHIRef BeginDynamicResolutionQuery;
		FRenderQueryRHIRef EndDynamicResolutionQuery;
		FRenderQueryRHIRef EndFrameQuery;

		// Heuristic's history 
		uint64 HeuristicHistoryEntry;

		InFlightFrameQueries()
			: HeuristicHistoryEntry(FDynamicResolutionHeuristicProxy::kInvalidEntryId)
		{ }
	};

	// List of frame queries in flight.
	TArray<InFlightFrameQueries> InFlightFrames;

	// Current frame's in flight index.
	int32 CurrentFrameInFlightIndex;

	// Uses GPU busy time queries.
	bool bUseTimeQueriesThisFrame;


	void HandLandedQueriesToHeuristic(bool bWait)
	{
		check(IsInRenderingThread());

		bool ShouldRefreshHeuristic = false;

		for (int32 i = 0; i < InFlightFrames.Num(); i++)
		{
			// If current in flight frame queries, ignore them since have not called EndRenderQuery().
			if (i == CurrentFrameInFlightIndex)
			{
				continue;
			}

			InFlightFrameQueries& InFlightFrame = InFlightFrames[i];

			// Results in microseconds.
			uint64 BeginFrameResult = 0;
			uint64 BeginDynamicResolutionResult = 0;
			uint64 EndDynamicResolutionResult = 0;
			uint64 EndFrameResult = 0;

			int32 LandingCount = 0;
			int32 QueryCount = 0;
			if (InFlightFrame.BeginFrameQuery.IsValid())
			{
				LandingCount += RHIGetRenderQueryResult(
					InFlightFrame.BeginFrameQuery, BeginFrameResult, bWait) ? 1 : 0;
				QueryCount += 1;
			}

			if (InFlightFrame.BeginDynamicResolutionQuery.IsValid())
			{
				LandingCount += RHIGetRenderQueryResult(
					InFlightFrame.BeginDynamicResolutionQuery, BeginDynamicResolutionResult, bWait) ? 1 : 0;
				QueryCount += 1;
			}

			if (InFlightFrame.EndDynamicResolutionQuery.IsValid())
			{
				LandingCount += RHIGetRenderQueryResult(
					InFlightFrame.EndDynamicResolutionQuery, EndDynamicResolutionResult, bWait) ? 1 : 0;
				QueryCount += 1;
			}

			if (InFlightFrame.EndFrameQuery.IsValid())
			{
				LandingCount += RHIGetRenderQueryResult(
					InFlightFrame.EndFrameQuery, EndFrameResult, bWait) ? 1 : 0;
				QueryCount += 1;
			}

			check(QueryCount == 0 || QueryCount == 4);

			// If all queries have landed, then hand the results to the heuristic.
			if (LandingCount == 4)
			{
				Heuristic.CommitPreviousFrameGPUTimings_RenderThread(
					InFlightFrame.HeuristicHistoryEntry,
					/* TotalFrameGPUBusyTimeMs = */ float(EndFrameResult - BeginFrameResult) / 1000.0f,
					/* DynamicResolutionGPUBusyTimeMs = */ float(EndDynamicResolutionResult - BeginDynamicResolutionResult) / 1000.0f,
					/* bGPUTimingsHaveCPUBubbles = */ !GRHISupportsGPUTimestampBubblesRemoval);

				// Reset this in-flight frame queries to be reused.
				InFlightFrame = InFlightFrameQueries();

				ShouldRefreshHeuristic = true;
			}
		}

		// Refresh the heuristic.
		if (ShouldRefreshHeuristic)
		{
			Heuristic.RefreshCurentFrameResolutionFraction_RenderThread();
		}
	}

	void FindNewInFlightIndex()
	{
		check(IsInRenderingThread());
		check(CurrentFrameInFlightIndex == -1);

		for (int32 i = 0; i < InFlightFrames.Num(); i++)
		{
			auto& InFlightFrame = InFlightFrames[i];
			if (!InFlightFrame.BeginFrameQuery.IsValid())
			{
				CurrentFrameInFlightIndex = i;
				break;
			}
		}

		// Allocate a new in-flight frame in the unlikely event.
		if (CurrentFrameInFlightIndex == -1)
		{
			CurrentFrameInFlightIndex = InFlightFrames.Num();
			InFlightFrames.Add(InFlightFrameQueries());
		}
	}
};


/**
 * Engine's default dynamic resolution state.
 */
class FDefaultDynamicResolutionState : public IDynamicResolutionState
{
public:
	FDefaultDynamicResolutionState()
		: Proxy(new FDefaultDynamicResolutionStateProxy())
	{
		check(IsInGameThread());
		bIsEnabled = false;
		bRecordThisFrame = false;
	}

	~FDefaultDynamicResolutionState() override
	{
		check(IsInGameThread());

		// Deletes the proxy on the rendering thread to make sure we don't delete before a recommand using it has finished.
		FDefaultDynamicResolutionStateProxy* P = Proxy;
		ENQUEUE_RENDER_COMMAND(DeleteDynamicResolutionProxy)(
			[P](class FRHICommandList&)
			{
				P->Finish();
				delete P;
			});
	}


	// Implements IDynamicResolutionState

	virtual bool IsSupported() const override
	{
		// No VR platforms are officially supporting dynamic resolution with Engine default's dynamic resolution state.
		const bool bIsStereo = GEngine->StereoRenderingDevice.IsValid() ? GEngine->StereoRenderingDevice->IsStereoEnabled() : false;
		if (bIsStereo)
		{
			return false;
		}
		return GRHISupportsDynamicResolution;
	}

	virtual void ResetHistory() override
	{
		check(IsInGameThread());
		FDefaultDynamicResolutionStateProxy* P = Proxy;
		ENQUEUE_RENDER_COMMAND(DynamicResolutionResetHistory)(
			[P](class FRHICommandList&)
			{
				P->Reset();
			});

	}

	virtual void SetEnabled(bool bEnable) override
	{
		check(IsInGameThread());
		bIsEnabled = bEnable;
	}

	virtual bool IsEnabled() const override
	{
		check(IsInGameThread());
		return bIsEnabled;
	}

	virtual float GetResolutionFractionApproximation() const override
	{
		check(IsInGameThread());
		return Proxy->Heuristic.GetResolutionFractionApproximation_GameThread();
	}

	virtual float GetResolutionFractionUpperBound() const override
	{
		check(IsInGameThread());
		return Proxy->Heuristic.GetResolutionFractionUpperBound();
	}

	virtual void ProcessEvent(EDynamicResolutionStateEvent Event) override
	{
		check(IsInGameThread());

		if (Event == EDynamicResolutionStateEvent::BeginFrame)
		{
			check(bRecordThisFrame == false);
			bRecordThisFrame = bIsEnabled;
		}

		// Early return if not recording this frame.
		if (!bRecordThisFrame)
		{
			return;
		}

		if (Event == EDynamicResolutionStateEvent::BeginFrame)
		{
			// Query game thread time in milliseconds.
			float PrevGameThreadTimeMs = FPlatformTime::ToMilliseconds(GGameThreadTime);

			FDefaultDynamicResolutionStateProxy* P = Proxy;
			ENQUEUE_RENDER_COMMAND(DynamicResolutionBeginFrame)(
				[PrevGameThreadTimeMs, P](class FRHICommandList& RHICmdList)
			{
				P->BeginFrame(RHICmdList, PrevGameThreadTimeMs);
			});
		}
		else
		{
			// Forward event to render thread.
			FDefaultDynamicResolutionStateProxy* P = Proxy;
			ENQUEUE_RENDER_COMMAND(DynamicResolutionBeginFrame)(
				[P, Event](class FRHICommandList& RHICmdList)
			{
				P->ProcessEvent(RHICmdList, Event);
			});

			if (Event == EDynamicResolutionStateEvent::EndFrame)
			{
				// Only record frames that have a BeginFrame event.
				bRecordThisFrame = false;
			}
		}
	}

	virtual void SetupMainViewFamily(class FSceneViewFamily& ViewFamily) override
	{
		check(IsInGameThread());

		if (bIsEnabled)
		{
			ViewFamily.SetScreenPercentageInterface(new FDefaultDynamicResolutionDriver(&Proxy->Heuristic, ViewFamily));
		}
	}

private:
	// Owned render thread proxy.
	FDefaultDynamicResolutionStateProxy* const Proxy;

	// Whether dynamic resolution is enabled.
	bool bIsEnabled;

	// Whether dynamic resolution is recording this frame.
	bool bRecordThisFrame;
};


//static
TSharedPtr< class IDynamicResolutionState > FDynamicResolutionHeuristicProxy::CreateDefaultState()
{
	return MakeShareable(new FDefaultDynamicResolutionState());
}
