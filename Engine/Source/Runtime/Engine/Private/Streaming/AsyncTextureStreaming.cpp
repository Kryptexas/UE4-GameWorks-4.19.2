// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
AsyncTextureStreaming.cpp: Definitions of classes used for texture streaming async task.
=============================================================================*/

#include "EnginePrivate.h"
#include "AsyncTextureStreaming.h"

void FAsyncTextureStreaming::ClearRemovedTextureReferences()
{
	// CleanUp Prioritized Textures, account for removed textures.
	for (int32 TexPrioIndex = 0; TexPrioIndex < PrioritizedTextures.Num(); ++TexPrioIndex)
	{
		FTexturePriority& TexturePriority = PrioritizedTextures[ TexPrioIndex ];

		// Because deleted textures can shrink StreamingTextures, we need to check that the index is in range.
		// Even if it is, because of the RemoveSwap logic, it could refer to the wrong texture now.
		if (!StreamingManager.StreamingTextures.IsValidIndex(TexturePriority.TextureIndex) || TexturePriority.Texture != StreamingManager.StreamingTextures[TexturePriority.TextureIndex].Texture)
		{
			TexturePriority.TextureIndex = INDEX_NONE;
			TexturePriority.Texture = nullptr;
		}
	}
}

void FAsyncTextureStreaming::DoWork()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FAsyncTextureStreaming::DoWork"), STAT_AsyncTextureStreaming_DoWork, STATGROUP_StreamingDetails);
	PrioritizedTextures.Empty( StreamingManager.StreamingTextures.Num() );
	PrioTexIndicesSortedByLoadOrder.Empty( StreamingManager.StreamingTextures.Num() );

	const int32 SplitRequestSizeThreshold = CVarStreamingSplitRequestSizeThreshold.GetValueOnAnyThread();

	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnAnyThread() > 0;
	const float MaxEffectiveScreenSize = CVarStreamingScreenSizeEffectiveMax.GetValueOnAnyThread();
	StreamingManager.ThreadSettings.TextureBoundsVisibility->ComputeBoundsViewInfos(StreamingManager.ThreadSettings.ThreadViewInfos, bUseNewMetrics, MaxEffectiveScreenSize);

	// Number of textures that want more mips.
	ThreadStats.NumWantingTextures = 0;
	ThreadStats.NumVisibleTexturesWithLowResolutions = 0;

	for ( int32 Index=0; Index < StreamingManager.StreamingTextures.Num() && !IsAborted(); ++Index )
	{
		FStreamingTexture& StreamingTexture = StreamingManager.StreamingTextures[ Index ];

		StreamingTexture.bUsesStaticHeuristics = false;
		StreamingTexture.bUsesDynamicHeuristics = false;
		StreamingTexture.bUsesLastRenderHeuristics = false;
		StreamingTexture.bUsesForcedHeuristics = false;
		StreamingTexture.bUsesOrphanedHeuristics = false;
		StreamingTexture.bHasSplitRequest  = false;
		StreamingTexture.bIsLastSplitRequest = false;

			// Figure out max number of miplevels allowed by LOD code.
			StreamingManager.CalcMinMaxMips( StreamingTexture );

			// Determine how many mips this texture should have in memory.
			StreamingManager.CalcWantedMips( StreamingTexture );

		ThreadStats.TotalResidentSize += StreamingTexture.GetSize( StreamingTexture.ResidentMips );
		ThreadStats.TotalPossibleResidentSize += StreamingTexture.GetSize( FMath::Max<int32>(StreamingTexture.ResidentMips, StreamingTexture.RequestedMips) );
		ThreadStats.TotalWantedMipsSize += StreamingTexture.GetSize( StreamingTexture.WantedMips );
		if (StreamingTexture.RequestedMips != StreamingTexture.ResidentMips)
		{
			// UpdateMips load texture mip changes in temporary memory.
			ThreadStats.TotalTempMemorySize += StreamingTexture.GetSize( StreamingTexture.RequestedMips );
		}


		// Split the request in two if the last mip to load is too big. Helps putprioriHelps converge the image quality.
		if (SplitRequestSizeThreshold != 0 && 
			StreamingTexture.LODGroup != TEXTUREGROUP_Terrain_Heightmap &&
			StreamingTexture.WantedMips > StreamingTexture.ResidentMips && 
			StreamingTexture.GetSize(StreamingTexture.WantedMips) - StreamingTexture.GetSize(StreamingTexture.WantedMips - 1) >= SplitRequestSizeThreshold) 
		{
			// The texture fits for a 2 request stream strategy, though it is not garantied that there will be 2 requests (since it could be only one mip aways)
			StreamingTexture.bHasSplitRequest = true;
			if (StreamingTexture.WantedMips == StreamingTexture.ResidentMips + 1)
			{
				StreamingTexture.bIsLastSplitRequest = true;
			}
			else
			{
				// Don't stream the last mips now.
				--StreamingTexture.WantedMips;
			}
		}
		if ( StreamingTexture.WantedMips > StreamingTexture.ResidentMips )
		{
			ThreadStats.NumWantingTextures++;
		}

		bool bTrackedTexture = TrackTextureEvent( &StreamingTexture, StreamingTexture.Texture, StreamingTexture.bForceFullyLoad, &StreamingManager );

		// Add to sort list, if it wants to stream in or could potentially stream out.
		if ( StreamingTexture.MaxAllowedMips > StreamingTexture.NumNonStreamingMips)
		{
			const UTexture2D* Texture = StreamingTexture.Texture; // This needs to go on the stack in case the main thread updates it at the same time (for the condition consistency).
			if (Texture)
			{
				new (PrioritizedTextures) FTexturePriority( StreamingTexture.CanDropMips(), StreamingTexture.CalcRetentionPriority(), StreamingTexture.CalcLoadOrderPriority(), Index, Texture );
			}
		}

		if (StreamingTexture.IsVisible() && StreamingTexture.GetWantedMipsForPriority() - StreamingTexture.ResidentMips > 1)
		{
			++ThreadStats.NumVisibleTexturesWithLowResolutions;
		}

			// Accumulate streaming numbers.
		int64 ResidentTextureSize = StreamingTexture.GetSize( StreamingTexture.ResidentMips );
		int64 WantedTextureSize = StreamingTexture.GetSize( StreamingTexture.WantedMips );
		if ( StreamingTexture.bInFlight )
		{
			int64 RequestedTextureSize = StreamingTexture.GetSize( StreamingTexture.RequestedMips );
			if ( StreamingTexture.RequestedMips > StreamingTexture.ResidentMips )
			{
				ThreadStats.PendingStreamInSize += FMath::Abs(RequestedTextureSize - ResidentTextureSize);
			}
			else
			{
				ThreadStats.PendingStreamOutSize += FMath::Abs(RequestedTextureSize - ResidentTextureSize);
			}
		}
		else
		{
			if ( StreamingTexture.WantedMips > StreamingTexture.ResidentMips )
			{
				ThreadStats.WantedInSize += FMath::Abs(WantedTextureSize - ResidentTextureSize);
			}
			else
			{
				// Counting on shrinking reallocation.
				ThreadStats.WantedOutSize += FMath::Abs(WantedTextureSize - ResidentTextureSize);
			}
		}

		const int32 PerfectWantedTextureSize = StreamingTexture.GetSize( StreamingTexture.PerfectWantedMips );
		ThreadStats.TotalRequiredSize += PerfectWantedTextureSize;
		StreamingManager.UpdateFrameStats( ThreadContext, StreamingTexture, Index );

		// Reset the boost factor
		StreamingTexture.BoostFactor = 1.0f;
	}

	// Sort the candidates.
	struct FCompareTexturePriority // Smaller retention priority first because we drop them first.
	{
		FORCEINLINE bool operator()( const FTexturePriority& A, const FTexturePriority& B ) const
		{
			if ( A.RetentionPriority < B.RetentionPriority )
			{
				return true;
			}
			else if ( A.RetentionPriority == B.RetentionPriority )
			{
				return ( A.TextureIndex < B.TextureIndex );
			}
			return false;
		}
	};
	// Texture are sorted by retention priority since the ASYNC loading will resort request by load priority
	PrioritizedTextures.Sort( FCompareTexturePriority() );


	// Sort the candidates.
	struct FCompareTexturePriorityByLoadOrder // Bigger load order priority first because we load them first.
	{
		FCompareTexturePriorityByLoadOrder(const TArray<FTexturePriority>& InPrioritizedTextures) : PrioritizedTextures(InPrioritizedTextures) {}
		const TArray<FTexturePriority>& PrioritizedTextures;

		FORCEINLINE bool operator()( int32 IndexA, int32 IndexB ) const
		{
			const FTexturePriority& A = PrioritizedTextures[IndexA];
			const FTexturePriority& B = PrioritizedTextures[IndexB];

			if ( A.LoadOrderPriority > B.LoadOrderPriority )
			{
				return true;
			}
			else if ( A.LoadOrderPriority == B.LoadOrderPriority )
			{
				return ( A.TextureIndex < B.TextureIndex );
			}
			return false;
		}
	};

	PrioTexIndicesSortedByLoadOrder.AddUninitialized(PrioritizedTextures.Num());
	for (int32 Index = 0; Index < PrioTexIndicesSortedByLoadOrder.Num(); ++Index)
	{
		PrioTexIndicesSortedByLoadOrder[Index] = Index;
	}
	PrioTexIndicesSortedByLoadOrder.Sort(FCompareTexturePriorityByLoadOrder(PrioritizedTextures));
}