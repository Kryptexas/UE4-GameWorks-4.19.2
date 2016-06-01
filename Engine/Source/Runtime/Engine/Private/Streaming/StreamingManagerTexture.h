// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureStreamingManager.h: Definitions of classes used for texture streaming.
=============================================================================*/

#pragma once

#include "StreamingTexture.h"
#include "TextureInstanceManager.h"

/*-----------------------------------------------------------------------------
	Texture streaming.
-----------------------------------------------------------------------------*/


/**
 * Structure containing all information needed for determining the screen space
 * size of an object/ texture instance.
 */
struct FStreamableTextureInstance4
{
	FStreamableTextureInstance4()
	:	BoundsOriginX( MAX_FLT, MAX_FLT, MAX_FLT, MAX_FLT )
	,	BoundsOriginY( 0, 0, 0, 0 )
	,	BoundsOriginZ( 0, 0, 0, 0 )
	,	BoxExtentSizeX( 0, 0, 0, 0 )
	,	BoxExtentSizeY( 0, 0, 0, 0 )
	,	BoxExtentSizeZ( 0, 0, 0, 0 )
	,	BoundingSphereRadius( 0, 0, 0, 0 )
	,	MinDistanceSq( 0, 0, 0, 0 )
	,	MaxDistanceSq( MAX_FLT, MAX_FLT, MAX_FLT, MAX_FLT )
	,	TexelFactor( 0, 0, 0, 0 )
	{
	}

	/** X coordinates for the bounds origin of 4 texture instances */
	FVector4 BoundsOriginX;
	/** Y coordinates for the bounds origin of 4 texture instances */
	FVector4 BoundsOriginY;
	/** Z coordinates for the bounds origin of 4 texture instances */
	FVector4 BoundsOriginZ;

	/** X size of the bounds box extent of 4 texture instances */
	FVector4 BoxExtentSizeX;
	/** Y size of the bounds box extent of 4 texture instances */
	FVector4 BoxExtentSizeY;
	/** Z size of the bounds box extent of 4 texture instances */
	FVector4 BoxExtentSizeZ;

	/** Sphere radii for the bounding sphere of 4 texture instances */
	FVector4 BoundingSphereRadius;
	/** Minimal distance ^2 (between the bounding sphere origin and the view origin) for which this entry is valid */
	FVector4 MinDistanceSq;
	/** Maximal distance ^2 (between the bounding sphere origin and the view origin) for which this entry is valid */
	FVector4 MaxDistanceSq;

	/** Texel scale factors for 4 texture instances */
	FVector4 TexelFactor;
};


// To compute the maximum mip size required by any heuristic
// float to avoid float->int conversion, unclamped until the end
class FFloatMipLevel
{
public:
	// constructor
	FFloatMipLevel();

	/**
	 * Helper function to map screen texels to a mip level, includes the global bias
	 * @param StreamingTexture can be 0 (if texture is getting destroyed)
	 * @apram MipBias from cvar
	 * @param bOptimal only needed for stats
	 */
	int32 ComputeMip(const FStreamingTexture* StreamingTexture, float MipBias, bool bOptimal) const;
	//
	bool operator>=(const FFloatMipLevel& rhs) const;
	//
	static FFloatMipLevel FromScreenSizeInTexels(float ScreenSizeInTexels);
	
	static FFloatMipLevel FromMipLevel(int32 InMipLevel);

	void Invalidate();

	// @return true: means this handle has a mip level that needs to be considered
	bool IsHandled() const
	{
		return MipLevel > 0.0f;
	}

private:
	// -1: not set yet
	float MipLevel;
};

/**
 * Streaming manager dealing with textures.
 */
struct FStreamingManagerTexture : public ITextureStreamingManager
{
	/** Constructor, initializing all members */
	FStreamingManagerTexture();

	virtual ~FStreamingManagerTexture();

	/**
	 * Updates streaming, taking into account all current view infos. Can be called multiple times per frame.
	 *
	 * @param DeltaTime				Time since last call in seconds
	 * @param bProcessEverything	[opt] If true, process all resources with no throttling limits
	 */
	virtual void UpdateResourceStreaming( float DeltaTime, bool bProcessEverything=false ) override;

	/**
	 * Updates streaming for an individual texture, taking into account all view infos.
	 *
	 * @param Texture	Texture to update
	 */
	virtual void UpdateIndividualTexture( UTexture2D* Texture ) override;

	/**
	 * Blocks till all pending requests are fulfilled.
	 *
	 * @param TimeLimit		Optional time limit for processing, in seconds. Specifying 0 means infinite time limit.
	 * @param bLogResults	Whether to dump the results to the log.
	 * @return				Number of streaming requests still in flight, if the time limit was reached before they were finished.
	 */
	virtual int32 BlockTillAllRequestsFinished( float TimeLimit = 0.0f, bool bLogResults = false ) override;

	/**
	 * Cancels the timed Forced resources (i.e used the Kismet action "Stream In Textures").
	 */
	virtual void CancelForcedResources() override;

	/**
	 * Notifies manager of "level" change so it can prioritize character textures for a few frames.
	 */
	virtual void NotifyLevelChange() override;

	/**
	 * Adds a textures streaming handler to the array of handlers used to determine which
	 * miplevels need to be streamed in/ out.
	 *
	 * @param TextureStreamingHandler	Handler to add
	 */
	void AddTextureStreamingHandler( FStreamingHandlerTextureBase* TextureStreamingHandler );

	/**
	 * Removes a textures streaming handler from the array of handlers used to determine which
	 * miplevels need to be streamed in/ out.
	 *
	 * @param TextureStreamingHandler	Handler to remove
	 */
	void RemoveTextureStreamingHandler( FStreamingHandlerTextureBase* TextureStreamingHandler );

	/** Don't stream world resources for the next NumFrames. */
	virtual void SetDisregardWorldResourcesForFrames( int32 NumFrames ) override;

	/**
	 *	Try to stream out texture mip-levels to free up more memory.
	 *	@param RequiredMemorySize	- Required minimum available texture memory
	 *	@return						- Whether it succeeded or not
	 **/
	virtual bool StreamOutTextureData( int64 RequiredMemorySize ) override;

	virtual int64 GetMemoryOverBudget() const override { return MemoryOverBudget; }

	virtual int64 GetMaxEverRequired() const override { return MaxEverRequired; }

	virtual void ResetMaxEverRequired() override { MaxEverRequired = 0; }

	/**
	 * Allows the streaming manager to process exec commands.
	 *
	 * @param InWorld World context
	 * @param Cmd	Exec command
	 * @param Ar	Output device for feedback
	 * @return		true if the command was handled
	 */
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) override;

	/**
	 * Exec command handlers
	 */
#if STATS_FAST
	bool HandleDumpTextureStreamingStatsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
#endif // STATS_FAST
#if STATS
	bool HandleListStreamingTexturesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListStreamingTexturesCollectCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListStreamingTexturesReportReadyCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListStreamingTexturesReportCommand( const TCHAR* Cmd, FOutputDevice& Ar );
#endif // STATS
#if !UE_BUILD_SHIPPING
	bool HandleResetMaxEverRequiredTexturesCommand(const TCHAR* Cmd, FOutputDevice& Ar);
	bool HandleLightmapStreamingFactorCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCancelTextureStreamingCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleShadowmapStreamingFactorCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNumStreamedMipsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleTrackTextureCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListTrackedTexturesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleDebugTrackedTexturesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleUntrackTextureCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleStreamOutCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandlePauseTextureStreamingCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleStreamingManagerMemoryCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld );
	bool HandleTextureGroupsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleInvestigateTextureCommand( const TCHAR* Cmd, FOutputDevice& Ar );
#endif // !UE_BUILD_SHIPPING
	/** Adds a new texture to the streaming manager. */
	virtual void AddStreamingTexture( UTexture2D* Texture ) override;

	/** Removes a texture from the streaming manager. */
	virtual void RemoveStreamingTexture( UTexture2D* Texture ) override;

	/** Adds a ULevel to the streaming manager. */
	virtual void AddPreparedLevel( class ULevel* Level ) override;

	/** Removes a ULevel from the streaming manager. */
	virtual void RemoveLevel( class ULevel* Level ) override;

	/** Called when an actor is spawned. */
	virtual void NotifyActorSpawned( AActor* Actor ) override;

	/** Called when a spawned actor is destroyed. */
	virtual void NotifyActorDestroyed( AActor* Actor ) override;

	/**
	 * Called when a primitive is attached to an actor or another component.
	 * Replaces previous info, if the primitive was already attached.
	 *
	 * @param InPrimitive	Newly attached dynamic/spawned primitive
	 */
	virtual void NotifyPrimitiveAttached( const UPrimitiveComponent* Primitive, EDynamicPrimitiveType DynamicType ) override;

	/** Called when a primitive is detached from an actor or another component. */
	virtual void NotifyPrimitiveDetached( const UPrimitiveComponent* Primitive ) override;

	/**
	 * Called when a LastRenderTime primitive is attached to an actor or another component.
	 * Modifies the LastRenderTimeRefCount for the textures used, so that those textures can
	 * use both distance-based and LastRenderTime heuristics.
	 *
	 * @param Primitive	Newly attached dynamic/spawned primitive
	 */
	virtual void NotifyTimedPrimitiveAttached( const UPrimitiveComponent* Primitive, EDynamicPrimitiveType DynamicType ) override;

	/**
	 * Called when a LastRenderTime primitive is detached from an actor or another component.
	 * Modifies the LastRenderTimeRefCount for the textures used, so that those textures can
	 * use both distance-based and LastRenderTime heuristics.
	 *
	 * @param Primitive	Newly detached dynamic/spawned primitive
	 */
	virtual void NotifyTimedPrimitiveDetached( const UPrimitiveComponent* Primitive ) override;

	/**
	 * Called when a primitive has had its textured changed.
	 * Only affects primitives that were already attached.
	 * Replaces previous info.
	 */
	virtual void NotifyPrimitiveUpdated( const UPrimitiveComponent* Primitive ) override;

	/** Returns the corresponding FStreamingTexture for a UTexture2D. */
	FStreamingTexture& GetStreamingTexture( const UTexture2D* Texture2D );

	/** Returns true if this is a streaming texture that is managed by the streaming manager. */
	virtual bool IsManagedStreamingTexture( const UTexture2D* Texture2D ) override;

	/** Updates the I/O state of a texture (allowing it to progress to the next stage) and some stats. */
	void UpdateTextureStatus( FStreamingTexture& StreamingTexture, FStreamingContext& Context );

	/**
	 * Cancels the current streaming request for the specified texture.
	 *
	 * @param StreamingTexture		Texture to cancel streaming for
	 * @return						true if a streaming request was canceled
	 */
	bool CancelStreamingRequest( FStreamingTexture& StreamingTexture );

	/** Resets the streaming statistics to zero. */
	void ResetStreamingStats();

	/**
	 * Updates the streaming statistics with current frame's worth of stats.
	 *
	 * @param Context					Context for the current frame
	 * @param bAllTexturesProcessed		Whether all processing is complete
	 */
	void UpdateStreamingStats( const FStreamingContext& Context, bool bAllTexturesProcessed );

	/** Returns the number of cached view infos (thread-safe). */
	int32 ThreadNumViews()
	{
		return ThreadSettings.ThreadViewInfos.Num();
	}

	/** Returns a cached view info for the specified index (thread-safe). */
	const FStreamingViewInfo& ThreadGetView( int32 ViewIndex )
	{
		return ThreadSettings.ThreadViewInfos[ ViewIndex ];
	}

#if STATS
	/** Ringbuffer of bandwidth samples for streaming in mip-levels (MB/sec). */
	static float BandwidthSamples[NUM_BANDWIDTHSAMPLES];
	/** Number of bandwidth samples that have been filled in. Will stop counting when it reaches NUM_BANDWIDTHSAMPLES. */
	static int32 NumBandwidthSamples;
	/** Current sample index in the ring buffer. */
	static int32 BandwidthSampleIndex;
	/** Average of all bandwidth samples in the ring buffer, in MB/sec. */
	static float BandwidthAverage;
	/** Maximum bandwidth measured since the start of the game.  */
	static float BandwidthMaximum;
	/** Minimum bandwidth measured since the start of the game.  */
	static float BandwidthMinimum;
#endif

	/** Set current pause state for texture streaming */
	virtual void PauseTextureStreaming(bool bInShouldPause) override
	{
		bPauseTextureStreaming = bInShouldPause;
	}

protected:
//BEGIN: Thread-safe functions and data
		friend class FAsyncTextureStreaming;
		friend struct FStreamingHandlerTextureStatic;
		friend struct FStreamingHandlerTextureDynamic;

		/** Calculates the minimum and maximum number of mip-levels for a streaming texture. */
		void CalcMinMaxMips( FStreamingTexture& StreamingTexture );

		/** Calculates the number of mip-levels we would like to have in memory for a texture. */
		void CalcWantedMips( FStreamingTexture& StreamingTexture );

		/**
		 * Fallback handler to catch textures that have been orphaned recently.
		 * This handler prevents massive spike in texture memory usage.
		 * Orphaned textures were previously streamed based on distance but those instance locations have been removed -
		 * for instance because a ULevel is being unloaded. These textures are still active and in memory until they are garbage collected,
		 * so we must ensure that they do not start using the LastRenderTime fallback handler and suddenly stream in all their mips -
		 * just to be destroyed shortly after by a garbage collect.
		 */
		FFloatMipLevel GetWantedMipsForOrphanedTexture( FStreamingTexture& StreamingTexture, float& Distance );

		/** Updates this frame's STATs by one texture. */
		void UpdateFrameStats( FStreamingContext& Context, FStreamingTexture& StreamingTexture, int32 TextureIndex );

		/**
		 * Not thread-safe: Updates a portion (as indicated by 'StageIndex') of all streaming textures,
		 * allowing their streaming state to progress.
		 *
		 * @param Context			Context for the current stage (frame)
		 * @param StageIndex		Current stage index
		 * @param NumUpdateStages	Number of texture update stages
		 */
		void UpdateStreamingTextures( FStreamingContext& Context, int32 StageIndex, int32 NumStages );

		/** Adds new textures and level data on the gamethread (while the worker thread isn't active). */
		void UpdateThreadData();

		/** Checks for updates in the user settings (CVars, etc). */
		void CheckUserSettings();

		/**
		 * Temporarily boosts the streaming distance factor by the specified number.
		 * This factor is automatically reset to 1.0 after it's been used for mip-calculations.
		 */
		void BoostTextures( AActor* Actor, float BoostFactor ) override;

		/**
		 * Stream textures in/out, based on the priorities calculated by the async work.
		 *
		 * @param bProcessEverything	Whether we're processing all textures in one go
		 */
		void StreamTextures( bool bProcessEverything );

		/**
		 * Keep as many unnecessary mips as possible.
		 *
		 * @param Context				Context for the current stage
		 */
		void KeepUnwantedMips( FStreamingContext& Context );

		/**
		 * Drop as many mips required to fit in budget.
		 *
		 * @param Context				Context for the current stage
		 */
		void DropWantedMips( FStreamingContext& Context );

		/**
		 * Drop as many forced required to fit in budget.
		 *
		 * @param Context				Context for the current stage
		 */
		void DropForcedMips( FStreamingContext& Context );

		/** Thread-safe helper struct for per-level information. */
		struct FThreadLevelData
		{
			FThreadLevelData()
			:	bRemove( false )
			{
			}

			/** Whether this level has been removed. */
			bool bRemove;

			/** Texture instances used in this level, stored in SIMD-friendly format. */
			TMap<const UTexture2D*,TArray<FStreamableTextureInstance4> > ThreadTextureInstances;
		};

		typedef TKeyValuePair< class ULevel*, FThreadLevelData >	FLevelData;

		/** Thread-safe helper struct for streaming information. */
		struct FThreadSettings
		{
			FThreadSettings() : TextureBoundsVisibility(nullptr) {}
			
			/** Cached from the system settings. */
			int32 NumStreamedMips[TEXTUREGROUP_MAX];

			/** Cached from each ULevel. */
			TArray< FLevelData > LevelData;

			/** Cached from FStreamingManagerBase. */
			TArray<FStreamingViewInfo> ThreadViewInfos;

			FTextureBoundsVisibility* TextureBoundsVisibility;

			/** from cvar, >=0 */
			float MipBias;
		};


		/** Thread-safe helper data for streaming information. */
		FThreadSettings	ThreadSettings;

		/** All streaming UTexture2D objects. */
		TArray<FStreamingTexture> StreamingTextures;

		/** Index of the StreamingTexture that will be updated next by UpdateStreamingTextures(). */
		int32 CurrentUpdateStreamingTextureIndex;
//END: Thread-safe functions and data

	/**
	 * Mark the textures with a timestamp. They're about to lose their location-based heuristic and we don't want them to
	 * start using LastRenderTime heuristic for a few seconds until they are garbage collected!
	 *
	 * @param RemovedTextures	List of removed textures.
	 */
	void	SetTexturesRemovedTimestamp(const FRemovedTextureArray& RemovedTextures);

	void	DumpTextureGroupStats( bool bDetailedStats );

	/**
	 * Prints out detailed information about streaming textures that has a name that contains the given string.
	 * Triggered by the InvestigateTexture exec command.
	 *
	 * @param InvestigateTextureName	Partial name to match textures against
	 */
	void	InvestigateTexture( const FString& InvestigateTextureName );

	void	DumpTextureInstances( const UPrimitiveComponent* Primitive, UTexture2D* Texture2D );

	/** Next sync, dump texture group stats. */
	bool	bTriggerDumpTextureGroupStats;

	/** Whether to the dumped texture group stats should contain extra information. */
	bool	bDetailedDumpTextureGroupStats;

	/** Next sync, dump all information we have about a certain texture. */
	bool	bTriggerInvestigateTexture;

	/** Name of a texture to investigate. Can be partial name. */
	FString	InvestigateTextureName;

	/** Async work for calculating priorities for all textures. */
	FAsyncTask<FAsyncTextureStreaming>*	AsyncWork;

	/** Textures from dynamic primitives. */
	FDynamicComponentTextureManager* DynamicComponentTextureManager;

	/** New textures, before they've been added to the thread-safe container. */
	TArray<UTexture2D*>		PendingStreamingTextures;

	/** New levels, before they've been added to the thread-safe container. */
	TArray<class ULevel*>	PendingLevels;

	/** Stages [0,N-2] is non-threaded data collection, Stage N-1 is wait-for-AsyncWork-and-finalize. */
	int32					ProcessingStage;

	/** Total number of processing stages (N). */
	int32					NumTextureProcessingStages;

	/** Whether to support texture instance streaming for dynamic (movable/spawned) objects. */
	bool					bUseDynamicStreaming;

	float					BoostPlayerTextures;

	/** Extra distance added to the range test, to start streaming the texture before they are actually used. */
	float					RangePrefetchDistance;

	/** Array of texture streaming objects to use during update. */
	TArray<FStreamingHandlerTextureBase*> TextureStreamingHandlers;

	/** Amount of memory to leave free in the texture pool. */
	int64					MemoryMargin;

	/** Minimum number of bytes to evict when we need to stream out textures because of a failed allocation. */
	int64					MinEvictSize;

	/** The actual memory pool size available to stream textures, excludes non-streaming texture, temp memory (for streaming mips), memory margin (allocator overhead). */
	int64					EffectiveStreamingPoolSize;

	/** If set, UpdateResourceStreaming() will only process this texture. */
	UTexture2D*				IndividualStreamingTexture;

	// Stats we need to keep across frames as we only iterate over a subset of textures.

	/** Number of streaming textures */
	uint32 NumStreamingTextures;
	/** Number of requests in cancelation phase. */
	uint32 NumRequestsInCancelationPhase;
	/** Number of requests in mip update phase. */
	uint32 NumRequestsInUpdatePhase;
	/** Number of requests in mip finalization phase. */
	uint32 NumRequestsInFinalizePhase;
	/** Size ot all intermerdiate textures in flight. */
	uint32 TotalIntermediateTexturesSize;
	/** Number of intermediate textures in flight. */
	uint32 NumIntermediateTextures;
	/** Size of all streaming testures. */
	uint64 TotalStreamingTexturesSize;
	/** Maximum size of all streaming textures. */
	uint64 TotalStreamingTexturesMaxSize;
	/** Total number of bytes in memory, for all streaming lightmap textures. */
	uint64 TotalLightmapMemorySize;
	/** Total number of bytes on disk, for all streaming lightmap textures. */
	uint64 TotalLightmapDiskSize;
	/** Total number of bytes in memory, for all HLOD textures. */
	uint64 TotalHLODMemorySize;
	/** Total number of bytes on disk, for all HLOD textures. */
	uint64 TotalHLODDiskSize;
	/** Number of mip count increase requests in flight. */
	uint32 TotalMipCountIncreaseRequestsInFlight;
	/** Total number of bytes in memory, if all textures were streamed perfectly with a 1.0 fudge factor. */
	uint64 TotalOptimalWantedSize;
	/** Total number of bytes using StaticTexture heuristics, currently in memory. */
	uint64 TotalStaticTextureHeuristicSize;
	/** Total number of bytes using DynmicTexture heuristics, currently in memory. */
	uint64 TotalDynamicTextureHeuristicSize;
	/** Total number of bytes using LastRenderTime heuristics, currently in memory. */
	uint64 TotalLastRenderHeuristicSize;
	/** Total number of bytes using ForcedIntoMemory heuristics, currently in memory. */
	uint64 TotalForcedHeuristicSize;
	int64 MemoryOverBudget;
	int64 MaxEverRequired;

	/** Unmodified texture pool size, in bytes, as specified in the .ini file. */
	int64 OriginalTexturePoolSize;
	/** Timestamp when we last shrunk the pool size because of memory usage. */
	double PreviousPoolSizeTimestamp;
	/** PoolSize CVar setting the last time we adjusted the pool size. */
	int32 PreviousPoolSizeSetting;

	/** Whether to collect, and optionally report, texture stats for the next run. */
	bool   bCollectTextureStats;
	bool   bReportTextureStats;
	TArray<FString> TextureStatsReport;

	/** Optional string to match against the texture names when collecting stats. */
	FString CollectTextureStatsName;

	/** Whether texture streaming is paused or not. When paused, it won't stream any textures in or out. */
	bool bPauseTextureStreaming;

#if STATS
	/**
	 * Ring buffer containing all latency samples we keep track of, as measured in seconds from the time the streaming system
	 * detects that a change is required until the data has been streamed in and the new texture is ready to be used.
	 */
	float LatencySamples[NUM_LATENCYSAMPLES];
	/** Number of latency samples that have been filled in. Will stop counting when it reaches NUM_LATENCYSAMPLES. */
	int32 NumLatencySamples;
	/** Current sample index in the ring buffer. */
	int32 LatencySampleIndex;
	/** Average of all latency samples in the ring buffer, in seconds. */
	float LatencyAverage;
	/** Maximum latency measured since the start of the game.  */
	float LatencyMaximum;

	/**
	 * Updates the streaming latency STAT for a texture.
	 *
	 * @param Texture		Texture to update for
	 * @param WantedMips	Number of desired mip-levels for the texture
	 * @param bInFlight		Whether the texture is currently being streamed
	 */
	void UpdateLatencyStats( UTexture2D* Texture, int32 WantedMips, bool bInFlight );

	/** Total time taken for each processing stage. */
	TArray<double> StreamingTimes;
#endif

#if STATS_FAST
	uint64 MaxStreamingTexturesSize;
	uint64 MaxOptimalTextureSize;
	int64 MaxStreamingOverBudget;
	uint64 MaxTexturePoolAllocatedSize;
	uint64 MinLargestHoleSize;
	uint32 MaxNumWantingTextures;
#endif
	
	friend bool TrackTextureEvent( FStreamingTexture* StreamingTexture, UTexture2D* Texture, bool bForceMipLevelsToBeResident, const FStreamingManagerTexture* Manager);
};




/*-----------------------------------------------------------------------------
	Texture streaming handler.
-----------------------------------------------------------------------------*/

/**
 * Base of texture streaming handler functionality.
 */
struct FStreamingHandlerTextureBase
{
	/** Friendly name identifying the handler for debug purposes. */
	const TCHAR* HandlerName;

	/** Default constructor. */
	FStreamingHandlerTextureBase()
		: HandlerName(TEXT("Base"))
	{
	}

	/**
	 * Returns mip count wanted by this handler for the passed in texture. 
	 * 
	 * @param	StreamingManager	Streaming manager
	 * @param	Streaming Texture	Texture to determine wanted mip count for
	 * @param	MinDistance			[out] Distance to the closest instance of this texture, in world space units.
	 * @return	Number of miplevels that should be streamed in or INDEX_NONE if texture isn't handled by this handler.
	 */
	virtual FFloatMipLevel GetWantedMips( FStreamingManagerTexture& StreamingManager, FStreamingTexture& StreamingTexture, float& MinDistance ) = 0;
};

/**
 * Static texture streaming handler. Used to stream textures on static level geometry.
 */
struct FStreamingHandlerTextureStatic : public FStreamingHandlerTextureBase
{
	/** Default constructor. */
	FStreamingHandlerTextureStatic()
	{
		HandlerName = TEXT("Static");
	}

	/**
	 * Returns mip count wanted by this handler for the passed in texture. 
	 * 
	 * @param	StreamingManager	Streaming manager
	 * @param	Streaming Texture	Texture to determine wanted mip count for
	 * @param	MinDistance			[out] Distance to the closest instance of this texture, in world space units.
	 * @return	Number of miplevels that should be streamed in or INDEX_NONE if texture isn't handled by this handler.
	 */
	virtual FFloatMipLevel GetWantedMips( FStreamingManagerTexture& StreamingManager, FStreamingTexture& StreamingTexture, float& MinDistance );
};


/**
 * Dynamic texture streaming handler. Used to stream textures for dynamic primitives.
 */
struct FStreamingHandlerTextureDynamic : public FStreamingHandlerTextureBase
{
	/** Default constructor. */
	FStreamingHandlerTextureDynamic()
	{
		HandlerName = TEXT("Dynamic");
	}

	/**
	 * Returns mip count wanted by this handler for the passed in texture. 
	 * 
	 * @param	StreamingManager	Streaming manager
	 * @param	Streaming Texture	Texture to determine wanted mip count for
	 * @param	MinDistance			[out] Distance to the closest instance of this texture, in world space units.
	 * @return	Number of miplevels that should be streamed in or INDEX_NONE if texture isn't handled by this handler.
	 */
	virtual FFloatMipLevel GetWantedMips( FStreamingManagerTexture& StreamingManager, FStreamingTexture& StreamingTexture, float& MinDistance );
};

/**
 * Streaming handler that bases its decision on the last render time.
 */
struct FStreamingHandlerTextureLastRender : public FStreamingHandlerTextureBase
{
	/** Default constructor. */
	FStreamingHandlerTextureLastRender()
	{
		HandlerName = TEXT("LastRender");
	}

	/**
	 * Returns mip count wanted by this handler for the passed in texture. 
	 * 
	 * @param	StreamingManager	Streaming manager
	 * @param	Streaming Texture	Texture to determine wanted mip count for
	 * @param	MinDistance			[out] Distance to the closest instance of this texture, in world space units.
	 * @return	Number of miplevels that should be streamed in or INDEX_NONE if texture isn't handled by this handler.
	 */
	virtual FFloatMipLevel GetWantedMips( FStreamingManagerTexture& StreamingManager, FStreamingTexture& StreamingTexture, float& MinDistance );
};

/**
 *	Streaming handler that bases its decision on the bForceMipStreaming flag in PrimitiveComponent.
 */
struct FStreamingHandlerTextureLevelForced : public FStreamingHandlerTextureBase
{
	/** Default constructor. */
	FStreamingHandlerTextureLevelForced()
	{
		HandlerName = TEXT("LevelForced");
	}

	/**
	 * Returns mip count wanted by this handler for the passed in texture. 
	 * 
	 * @param	StreamingManager	Streaming manager
	 * @param	Streaming Texture	Texture to determine wanted mip count for
	 * @param	MinDistance			[out] Distance to the closest instance of this texture, in world space units.
	 * @return	Number of miplevels that should be streamed in or INDEX_NONE if texture isn't handled by this handler.
	 */
	virtual FFloatMipLevel GetWantedMips( FStreamingManagerTexture& StreamingManager, FStreamingTexture& StreamingTexture, float& MinDistance );
};

/*-----------------------------------------------------------------------------
	Texture streaming helper structs.
-----------------------------------------------------------------------------*/

struct FSpawnedTextureInstance
{
	FSpawnedTextureInstance( UTexture2D* InTexture2D, float InTexelFactor, float InOriginalRadius )
	:	Texture2D( InTexture2D )
	,	TexelFactor( InTexelFactor )
	,	InvOriginalRadius( (InOriginalRadius > 0.0f) ? (1.0f/InOriginalRadius) : 1.0f )
	{
	}
	/** Texture that is used by a dynamic UPrimitiveComponent. */
	UTexture2D*		Texture2D;
	/** Texture specific texel scale factor  */
	float			TexelFactor;
	/** 1.0f / OriginalBoundingSphereRadius, at the time the TexelFactor was calculated originally. */
	float			InvOriginalRadius;
};