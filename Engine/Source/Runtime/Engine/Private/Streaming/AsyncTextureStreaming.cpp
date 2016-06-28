// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
AsyncTextureStreaming.cpp: Definitions of classes used for texture streaming async task.
=============================================================================*/

#include "EnginePrivate.h"
#include "AsyncTextureStreaming.h"

void FAsyncTextureStreamingData::Init(TArray<FStreamingViewInfo> InViewInfos, float InLastUpdateTime, TArray<FLevelTextureManager>& LevelTextureManagers, FDynamicComponentTextureManager& DynamicComponentManager)
{
	ViewInfos = InViewInfos;
	LastUpdateTime = InLastUpdateTime;

	DynamicInstancesView = DynamicComponentManager.GetAsyncView();

	StaticInstancesViews.Reset();
	for (FLevelTextureManager& LevelManager : LevelTextureManagers)
	{
		StaticInstancesViews.Push(LevelManager.GetAsyncView());
	}
}

void FAsyncTextureStreamingData::UpdateBoundSizes_Async()
{
	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnAnyThread() > 0;
	const float MaxEffectiveScreenSize = CVarStreamingScreenSizeEffectiveMax.GetValueOnAnyThread();

	for (FTextureInstanceAsyncView& StaticInstancesView : StaticInstancesViews)
	{
		StaticInstancesView.UpdateBoundSizes_Async(ViewInfos, LastUpdateTime, bUseNewMetrics, MaxEffectiveScreenSize);
	}
	DynamicInstancesView.UpdateBoundSizes_Async(ViewInfos, LastUpdateTime, bUseNewMetrics, MaxEffectiveScreenSize);
}


void FAsyncTextureStreamingData::UpdatePerfectWantedMips_Async(FStreamingTexture& StreamingTexture, float MipBias, bool bOutputToLog) const
{
#if UE_BUILD_SHIPPING
	bOutputToLog = false;
#endif

	// Cache Texture on the stack as it could be nullified on the gamethread.
	const UTexture2D* Texture = StreamingTexture.Texture;
	if (!Texture) return;

	float MaxSize = 0;
	float MaxSize_VisibleOnly = 0;
	bool bLooksLowRes = false;

	const float MaxAllowedSize = StreamingTexture.GetMaxAllowedSize();

	if (StreamingTexture.MinAllowedMips == StreamingTexture.MaxAllowedMips)
	{
		MaxSize_VisibleOnly = MaxSize = MaxAllowedSize;
	}
	else if (StreamingTexture.bForceFullyLoad)
	{
		if (bOutputToLog) UE_LOG(LogContentStreaming, Log,  TEXT("  Forced FullyLoad"));

		MaxSize_VisibleOnly = FLT_MAX;
	}
	else
	{
		DynamicInstancesView.GetTexelSize(Texture, MaxSize, MaxSize_VisibleOnly, bOutputToLog ? TEXT("Dynamic") : nullptr);

		for (const FTextureInstanceAsyncView& StaticInstancesView : StaticInstancesViews)
		{
			// No need to iterate more if texture is already at maximum resolution.
			if (MaxSize_VisibleOnly >= MAX_TEXTURE_SIZE && !bOutputToLog)
			{
				break;
			}

			StaticInstancesView.GetTexelSize(Texture, MaxSize, MaxSize_VisibleOnly, bOutputToLog ? TEXT("Static") : nullptr);
		}

		// Don't apply to FLT_MAX since it is used as forced streaming. BoostFactor as only meaning for texture instances since the other heuristics are based on max resolution.
		if ((MaxSize > 0 || MaxSize_VisibleOnly > 0) && MaxSize != FLT_MAX && MaxSize_VisibleOnly != FLT_MAX)
		{
			const float CumBoostFactor = StreamingTexture.BoostFactor * StreamingTexture.DynamicBoostFactor;

			// If there is not enough resolution in the texture to fix the required quality, save this information to prevent degrading this texture before other ones.
			bLooksLowRes = FMath::Max3<int32>(MaxSize_VisibleOnly, MaxSize, MaxAllowedSize) / MaxAllowedSize >= CumBoostFactor * 2.f;

			MaxSize *=  CumBoostFactor;
			MaxSize_VisibleOnly *= CumBoostFactor;
		}
 
		/**
		 * Fallback handler to catch textures that have been orphaned recently.
		 * This handler prevents massive spike in texture memory usage.
		 * Orphaned textures were previously streamed based on distance but those instance locations have been removed -
		 * for instance because a ULevel is being unloaded. These textures are still active and in memory until they are garbage collected,
		 * so we must ensure that they do not start using the LastRenderTime fallback handler and suddenly stream in all their mips -
		 * just to be destroyed shortly after by a garbage collect.
		 */
		const float TimeSinceRemoved = (float)(FApp::GetCurrentTime() - StreamingTexture.InstanceRemovedTimestamp);
		if (MaxSize == 0 && MaxSize_VisibleOnly == 0 && TimeSinceRemoved < 91.f && StreamingTexture.LastRenderTime > TimeSinceRemoved - 5.f)
		{
			if (bOutputToLog) UE_LOG(LogContentStreaming, Log,  TEXT("  Orphaned"));

			// Keep the current mip, unless the space is required.
			MaxSize = (float)(0x1 << (StreamingTexture.ResidentMips - 1)); // Keep all mips.

			if (StreamingTexture.LastRenderTime < 5.0f)
			{
				MaxSize_VisibleOnly = MaxSize;
			}
		}

		const bool bUseLastRenderTime = (StreamingTexture.LastRenderTimeRefCount > 0 || FApp::GetCurrentTime() - StreamingTexture.LastRenderTimeRefCountTimestamp < 91.0);
		if (((MaxSize == 0 && MaxSize_VisibleOnly == 0) || bUseLastRenderTime) && StreamingTexture.LastRenderTime < 90.0f)
		{
			if (bOutputToLog) UE_LOG(LogContentStreaming, Log,  TEXT("  Using Last RenderTime"));

			// Get all mips. Using MaxSize=MaxTextureSize, and not FLT_MAX, will drop one mip if the texture is not used.
			MaxSize = FMath::Max<int32>(MaxSize, MaxAllowedSize);

			if (StreamingTexture.LastRenderTime < 5.0f)
			{
				MaxSize_VisibleOnly = FMath::Max<int32>(MaxSize_VisibleOnly, MaxAllowedSize);
			}
		}
	}

	StreamingTexture.SetPerfectWantedMips_Async(MaxSize, MaxSize_VisibleOnly, MipBias, bLooksLowRes);
}

void FAsyncTextureStreamingTask::UpdateBudgetedMips_Async(int64& MemoryUsed, int64& TempMemoryUsed)
{
	//*************************************
	// Update Budget
	//*************************************

	TArray<FStreamingTexture>& StreamingTextures = StreamingManager.StreamingTextures;
	TArray<int32> PrioritizedTextures;

	int64 MemoryBudgeted = 0;
	MemoryUsed = 0;
	TempMemoryUsed = 0;

	for (FStreamingTexture& StreamingTexture : StreamingTextures)
	{
		if (IsAborted()) break;

		MemoryBudgeted += StreamingTexture.UpdateRetentionPriority_Async();
		MemoryUsed += StreamingTexture.GetSize(StreamingTexture.ResidentMips);

		if (StreamingTexture.ResidentMips != StreamingTexture.RequestedMips)
		{
			TempMemoryUsed += StreamingTexture.GetSize(StreamingTexture.RequestedMips);
		}
	}

	//*************************************
	// Update Effective Budget
	//*************************************

	// Update EffectiveStreamingPoolSize, trying to stabilize it independently of temp memory, allocator overhead and non-streaming resources normal variation.
	// It's hard to know how much temp memory and allocator overhead is actually in AllocatedMemorySize as it is platform specific.
	// We handle it by not using all memory available. If temp memory and memory margin values are effectively bigger than the actual used values, the pool will stabilize.
	const int64 AvailableMemoryForStreaming =  PoolSize - (AllocatedMemory - MemoryUsed);

	if (AvailableMemoryForStreaming < MemoryBudget)
	{
		// Reduce size immediately to avoid taking more memory.
		MemoryBudget = AvailableMemoryForStreaming;
	}
	else if (AvailableMemoryForStreaming - MemoryBudget > TempMemoryBudget + MemoryMargin)
	{
		// Increase size considering that the variation does not come from temp memory or allocator overhead (or other recurring cause).
		// It's unclear how much temp memory is actually in there, but the value will decrease if temp memory increases.
		MemoryBudget = AvailableMemoryForStreaming;
	}

	//*************************************
	// Drop Mips
	//*************************************

	// If the budget is taking too much, drop some mips.
	if (MemoryBudgeted > MemoryBudget && !IsAborted())
	{
		PrioritizedTextures.Empty(StreamingTextures.Num());

		for (int32 TextureIndex = 0; TextureIndex < StreamingTextures.Num() && !IsAborted(); ++TextureIndex)
		{
			FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];
			// Only consider non deleted textures (can change any time).
			if (!StreamingTexture.Texture) continue;

			// In editor, forced stream in and terrain should never have reduced mips as they can be edited.
			if (GIsEditor && (StreamingTexture.bIsTerrainTexture || StreamingTexture.bForceFullyLoadHeuristic)) continue;

			// Ignore texture that can't drop any mips
			if (StreamingTexture.BudgetedMips > StreamingTexture.MinAllowedMips)
			{
				PrioritizedTextures.Add(TextureIndex);
			}
		}

		// Sort texture, having those that should be dropped first.
		PrioritizedTextures.Sort(FCompareTextureByRetentionPriority(StreamingTextures));

		bool bBudgetIsChanging = true;
		int32 NumDroppedMips = 0;
		while (MemoryBudgeted > MemoryBudget && bBudgetIsChanging && !IsAborted())
		{
			bBudgetIsChanging = false;
			++NumDroppedMips;

			// Drop from the lowest priority first (starting with last elements)
			for (int32 PriorityIndex = PrioritizedTextures.Num() - 1; PriorityIndex >= 0 && MemoryBudgeted > MemoryBudget && !IsAborted(); --PriorityIndex)
			{
				int32 TextureIndex = PrioritizedTextures[PriorityIndex];
				if (TextureIndex == INDEX_NONE) continue;

				FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];

				// If this texture has already missing mips for its normal quality, don't drop more than required..
				if (StreamingTexture.NumMissingMips >= NumDroppedMips)
				{
					bBudgetIsChanging = true; // Needed to prevent aborting loop.
					continue;
				}

				int64 GivenMemory = StreamingTexture.DropOneMip_Async();

				if (GivenMemory > 0)
				{
					MemoryBudgeted -= GivenMemory;
					bBudgetIsChanging = true;
				}
				else // Nothing to drop
				{
					PrioritizedTextures[PriorityIndex] = INDEX_NONE; // Don't try this one again.
				}
			}
		}
	}

	//*************************************
	// Keep Mips
	//*************************************

	// If there is some room left, try to keep as much as long as it won't bust budget.
	// This will run even after sacrificing to fit in budget since some small unwanted mips could still be kept.
	if (MemoryBudgeted < MemoryBudget && !IsAborted())
	{
		PrioritizedTextures.Empty(StreamingTextures.Num());
		const int64 MaxMipSize = MemoryBudget - MemoryBudgeted;
		for (int32 TextureIndex = 0; TextureIndex < StreamingTextures.Num() && !IsAborted(); ++TextureIndex)
		{
			FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];
			// Only consider non deleted textures (can change any time).
			if (!StreamingTexture.Texture) continue;

			// Ignore texture that can't drop any mips
			if (StreamingTexture.BudgetedMips < StreamingTexture.ResidentMips && 
				StreamingTexture.GetSize(StreamingTexture.BudgetedMips + 1) - StreamingTexture.GetSize(StreamingTexture.BudgetedMips) <= MaxMipSize)
			{
				PrioritizedTextures.Add(TextureIndex);
			}
		}

		// Sort texture, having those that should be dropped first.
		PrioritizedTextures.Sort(FCompareTextureByRetentionPriority(StreamingTextures));

		bool bBudgetIsChanging = true;
		while (MemoryBudgeted < MemoryBudget && bBudgetIsChanging && !IsAborted())
		{
			bBudgetIsChanging = false;

			// Keep from highest priority first.
			for (int32 PriorityIndex = 0; PriorityIndex < PrioritizedTextures.Num() && MemoryBudgeted < MemoryBudget && !IsAborted(); ++PriorityIndex)
			{
				int32 TextureIndex = PrioritizedTextures[PriorityIndex];
				if (TextureIndex == INDEX_NONE) continue;

				FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];
				int64 TakenMemory = StreamingTexture.KeepOneMip_Async();

				if (TakenMemory > 0)
				{
					if (MemoryBudgeted + TakenMemory <= MemoryBudget)
					{
						MemoryBudgeted += TakenMemory;
						bBudgetIsChanging = true;
					}
					else // Cancel keeping this mip
					{
						StreamingTexture.DropOneMip_Async();
						PrioritizedTextures[PriorityIndex] = INDEX_NONE; // Don't try this one again.
					}
				}
				else // No other mips to keep.
				{
					PrioritizedTextures[PriorityIndex] = INDEX_NONE; // Don't try this one again.
				}
			}
		}
	}

	//*************************************
	// Handle drop mips debug option
	//*************************************
#if !UE_BUILD_SHIPPING
	int32 DropMipsSetting = CVarStreamingDropMips.GetValueOnAnyThread();
	if (DropMipsSetting > 0)
	{
		for (FStreamingTexture& StreamingTexture : StreamingTextures)
		{
			if (IsAborted()) break;

			if (DropMipsSetting == 1)
			{
				StreamingTexture.BudgetedMips = FMath::Min<int32>(StreamingTexture.BudgetedMips, StreamingTexture.GetPerfectWantedMips());
			}
			else
			{
				StreamingTexture.BudgetedMips = FMath::Min<int32>(StreamingTexture.BudgetedMips, StreamingTexture.VisibleWantedMips);
			}
		}
	}
#endif
}

void FAsyncTextureStreamingTask::UpdateLoadAndCancelationRequests_Async(int64 MemoryUsed, int64 TempMemoryUsed)
{
	TArray<FStreamingTexture>& StreamingTextures = StreamingManager.StreamingTextures;

	TArray<int32> PrioritizedTextures;
	PrioritizedTextures.Empty(StreamingTextures.Num());
	for (int32 TextureIndex = 0; TextureIndex < StreamingTextures.Num() && !IsAborted(); ++TextureIndex)
	{

		FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];
		if (StreamingTexture.UpdateLoadOrderPriority_Async())
		{
			PrioritizedTextures.Add(TextureIndex);
		}
	}
	PrioritizedTextures.Sort(FCompareTextureByLoadOrderPriority(StreamingTextures));

	LoadRequests.Empty();
	CancelationRequests.Empty();

	// Now fill in the LoadRequest and CancelationRequests
	for (int32 PriorityIndex = 0; PriorityIndex < PrioritizedTextures.Num() && !IsAborted(); ++PriorityIndex)
	{
		int32 TextureIndex = PrioritizedTextures[PriorityIndex];
		FStreamingTexture& StreamingTexture = StreamingTextures[TextureIndex];

		if (StreamingTexture.bInFlight)
		{
			// If there is a pending load that attempts to load unrequired data, 
			// or if there is a pending unload that attempts to unload required data, try to cancel it.
			if (StreamingTexture.RequestedMips > FMath::Max<int32>(StreamingTexture.ResidentMips, StreamingTexture.WantedMips ) || 
				StreamingTexture.RequestedMips < FMath::Min<int32>(StreamingTexture.ResidentMips, StreamingTexture.WantedMips ))
			{
				CancelationRequests.Add(TextureIndex);
				StreamingTexture.WantedMips = StreamingTexture.ResidentMips;
			}
			else // Reset wanted mips for stats.
			{
				StreamingTexture.WantedMips = StreamingTexture.RequestedMips;
			}
		}
		else if (StreamingTexture.WantedMips < StreamingTexture.ResidentMips && TempMemoryUsed < TempMemoryBudget)
		{
			const int64 TempMemoryRequired = StreamingTexture.GetSize(StreamingTexture.WantedMips);
			const int64 UsedMemoryRequired = StreamingTexture.GetSize(StreamingTexture.WantedMips) - StreamingTexture.GetSize(StreamingTexture.ResidentMips);

			if (TempMemoryUsed + TempMemoryRequired <= TempMemoryBudget)
			{
				LoadRequests.Add(TextureIndex);
	
				MemoryUsed -= UsedMemoryRequired;
				TempMemoryUsed += TempMemoryRequired;
			}
			else // Reset wanted mips for stats.
			{
				StreamingTexture.WantedMips = StreamingTexture.ResidentMips;
			}
		}
		else if (StreamingTexture.WantedMips > StreamingTexture.ResidentMips && MemoryUsed < MemoryBudget && TempMemoryUsed < TempMemoryBudget)
		{
			const int64 UsedMemoryRequired = StreamingTexture.GetSize(StreamingTexture.WantedMips) - StreamingTexture.GetSize(StreamingTexture.ResidentMips);
			const int64 TempMemoryRequired = StreamingTexture.GetSize(StreamingTexture.WantedMips);

			if (MemoryUsed + UsedMemoryRequired <= MemoryBudget && TempMemoryUsed + TempMemoryRequired <= TempMemoryBudget)
			{
				LoadRequests.Add(TextureIndex);
	
				MemoryUsed += UsedMemoryRequired;
				TempMemoryUsed += TempMemoryRequired;
			}
			else // Reset wanted mips for stats.
			{
				StreamingTexture.WantedMips = StreamingTexture.ResidentMips;
			}
		}
		else // Reset wanted mips for stats.
		{
			StreamingTexture.WantedMips = StreamingTexture.ResidentMips;
		}
	}
}

void FAsyncTextureStreamingTask::DoWork()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FAsyncTextureStreamingTask::DoWork"), STAT_AsyncTextureStreaming_DoWork, STATGROUP_StreamingDetails);

	// While the async task is runnning, the StreamingTextures are guarantied not to be reallocated.
	// 2 things can happen : a texture can be removed, in which case the texture will be set to null
	// or some members can be updated following calls to UpdateDynamicData().
	TArray<FStreamingTexture>& StreamingTextures = StreamingManager.StreamingTextures;

	// Update the distance and size for each bounds.
	StreamingData.UpdateBoundSizes_Async();
	
	for (FStreamingTexture& StreamingTexture : StreamingTextures)
	{
		if (IsAborted()) break;

		StreamingData.UpdatePerfectWantedMips_Async(StreamingTexture, StreamingManager.MipBias);
		StreamingTexture.DynamicBoostFactor = 1.f; // Reset after every computation.
	}

	int64 MemoryUsed, TempMemoryUsed;
	// According to budget, make relevant sacrifices and keep possible unwanted mips
	UpdateBudgetedMips_Async(MemoryUsed, TempMemoryUsed);

	// Update load requests.
	UpdateLoadAndCancelationRequests_Async(MemoryUsed, TempMemoryUsed);

	STAT(UpdateStats_Async());
}

void FAsyncTextureStreamingTask::UpdateStats_Async()
{
#if STATS
	FTextureStreamingStats& Stats = StreamingManager.GatheredStats;
	FTextureStreamingStats& PrevStats = StreamingManager.DisplayedStats;
	TArray<FStreamingTexture>& StreamingTextures = StreamingManager.StreamingTextures;

	Stats.TexturePool = PoolSize;
	// Stats.StreamingPool = MemoryBudget;
	Stats.UsedStreamingPool = 0;

	Stats.SafetyPool = MemoryMargin; 
	Stats.TemporaryPool = TempMemoryBudget;
	Stats.StreamingPool = MemoryBudget;
	Stats.NonStreamingMips = AllocatedMemory;

	Stats.RequiredPool = 0;
	Stats.VisibleMips = 0;
	Stats.HiddenMips = 0;
	Stats.ForcedMips = 0;
	Stats.CachedMips = 0;

	Stats.WantedMips = 0;
	Stats.PendingRequests = 0;

	for (FStreamingTexture& StreamingTexture : StreamingTextures)
	{
		if (IsAborted()) break;
		if (!StreamingTexture.Texture) continue;

		const int64 ResidentSize = StreamingTexture.GetSize(StreamingTexture.ResidentMips);
		const int64 RequiredSize = StreamingTexture.GetSize(StreamingTexture.GetPerfectWantedMips());
		const int64 BudgetedSize = StreamingTexture.GetSize(StreamingTexture.BudgetedMips);
		const int64 VisibleWantedSize = StreamingTexture.GetSize(StreamingTexture.VisibleWantedMips);

		// How much the streamer would use if there was no limit.
		Stats.RequiredPool += RequiredSize;

		// How much the streamer actually use.
		Stats.UsedStreamingPool += FMath::Min<int64>(RequiredSize, BudgetedSize);

		// Remove from the non streaming budget what is actually taken by streaming.
		Stats.NonStreamingMips -= ResidentSize;

		const int64 UsedSize = FMath::Min3<int64>(RequiredSize, BudgetedSize, ResidentSize);

		Stats.WantedMips += UsedSize;
		Stats.CachedMips += FMath::Max<int64>(ResidentSize - UsedSize, 0);

		if (StreamingTexture.bForceFullyLoadHeuristic)
		{
			Stats.ForcedMips += UsedSize;
		}
		else if (VisibleWantedSize >= UsedSize)
		{
			Stats.VisibleMips += UsedSize;
		}
		else // VisibleWantedSize < UsedSize
		{
			Stats.VisibleMips += VisibleWantedSize;
			Stats.HiddenMips += UsedSize - VisibleWantedSize;
		}

		if (StreamingTexture.RequestedMips > StreamingTexture.ResidentMips)
		{
			Stats.PendingRequests += StreamingTexture.GetSize(StreamingTexture.RequestedMips) - ResidentSize;
		}
	}
	Stats.Timestamp = FPlatformTime::Seconds();
#endif
}
