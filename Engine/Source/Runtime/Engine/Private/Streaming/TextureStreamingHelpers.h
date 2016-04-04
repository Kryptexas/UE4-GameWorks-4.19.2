// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureStreamingHelpers.h: Definitions of classes used for texture streaming.
=============================================================================*/

#pragma once

/**
 * Streaming stats
 */
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Num Streaming Textures"),STAT_StreamingTextures,STATGROUP_Streaming, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Pool Memory Size"), STAT_TexturePoolSize, STATGROUP_Streaming, FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Pool Memory Used"), STAT_TexturePoolAllocatedSize, STATGROUP_Streaming, FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Effective Streaming Pool Size"), STAT_EffectiveStreamingPoolSize, STATGROUP_Streaming, FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Streaming Textures"),STAT_StreamingTexturesSize,STATGROUP_Streaming,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Non-streaming Textures"),STAT_NonStreamingTexturesSize,STATGROUP_Streaming,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Textures On Disk"),STAT_StreamingTexturesMaxSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Lightmaps In Memory"),STAT_LightmapMemorySize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Lightmaps On Disk"),STAT_LightmapDiskSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("HLOD Textures In Memory"), STAT_HLODTextureMemorySize, STATGROUP_StreamingDetails, FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("HLOD Textures On Disk"), STAT_HLODTextureDiskSize, STATGROUP_StreamingDetails, FPlatformMemory::MCR_TexturePool, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Requests In Cancelation Phase"),STAT_RequestsInCancelationPhase,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Requests In Update Phase"),STAT_RequestsInUpdatePhase,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Requests in Finalize Phase"),STAT_RequestsInFinalizePhase,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Intermediate Textures"),STAT_IntermediateTextures,STATGROUP_StreamingDetails, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Intermediate Textures Size"),STAT_IntermediateTexturesSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Textures Streamed In, Frame"),STAT_RequestSizeCurrentFrame,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Textures Streamed In, Total"),STAT_RequestSizeTotal,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Lightmaps Streamed In, Total"),STAT_LightmapRequestSizeTotal,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Game Thread Update Time"),STAT_GameThreadUpdateTime,STATGROUP_Streaming, );
DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(TEXT("Streaming Latency, Average (sec)"),STAT_StreamingLatency,STATGROUP_StreamingDetails, );
DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(TEXT("Streaming Bandwidth, Average (MB/s)"),STAT_StreamingBandwidth,STATGROUP_StreamingDetails, );
DECLARE_FLOAT_ACCUMULATOR_STAT_EXTERN(TEXT("Dynamic Streaming Total Time (sec)"),STAT_DynamicStreamingTotal,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Num Visible Textures With Low Resolutions"),STAT_NumVisibleTexturesWithLowResolutions,STATGROUP_StreamingDetails, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Volume Streaming Tick"),STAT_VolumeStreamingTickTime,STATGROUP_StreamingDetails, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Streaming Volumes"),STAT_VolumeStreamingChecks,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Growing Reallocations"),STAT_GrowingReallocations,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Shrinking Reallocations"),STAT_ShrinkingReallocations,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Full Reallocations"),STAT_FullReallocations,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Failed Reallocations"),STAT_FailedReallocations,STATGROUP_StreamingDetails, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Panic Defragmentations"),STAT_PanicDefragmentations,STATGROUP_StreamingDetails, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("AddToWorld Time"),STAT_AddToWorldTime,STATGROUP_StreamingDetails, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("RemoveFromWorld Time"),STAT_RemoveFromWorldTime,STATGROUP_StreamingDetails, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("UpdateLevelStreaming Time"),STAT_UpdateLevelStreamingTime,STATGROUP_StreamingDetails, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Required Streaming Textures"),STAT_OptimalTextureSize,STATGROUP_Streaming,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Over Budget"),STAT_StreamingOverBudget,STATGROUP_Streaming,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Under Budget"),STAT_StreamingUnderBudget,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Num Pending Textures"),STAT_NumWantingTextures,STATGROUP_Streaming, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Levels With PostLoad Rotations"),STAT_LevelsWithPostLoadRotations,STATGROUP_Streaming, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Static Textures In Memory"),STAT_TotalStaticTextureHeuristicSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("LastRenderTime Textures In Memory"),STAT_TotalLastRenderHeuristicSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Dynamic Textures In Memory"),STAT_TotalDynamicHeuristicSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );
DECLARE_MEMORY_STAT_POOL_EXTERN(TEXT("Forced Textures In Memory"),STAT_TotalForcedHeuristicSize,STATGROUP_StreamingDetails,FPlatformMemory::MCR_TexturePool, );

DECLARE_LOG_CATEGORY_EXTERN(LogContentStreaming, Log, All);


extern float GLightmapStreamingFactor;
extern float GShadowmapStreamingFactor;
extern bool GNeverStreamOutTextures;
extern double GStreamingDynamicPrimitivesTime;
extern bool GIsOperatingWithReducedTexturePool;
extern bool GStreamWithTimeFactor;

//@DEBUG:
// Set to 1 to log all dynamic component notifications
#define STREAMING_LOG_DYNAMIC		0
// Set to 1 to log when we change a view
#define STREAMING_LOG_VIEWCHANGES	0
// Set to 1 to log when levels are added/removed
#define STREAMING_LOG_LEVELS		0
// Set to 1 to log textures that are canceled by CancelForcedTextures()
#define STREAMING_LOG_CANCELFORCED	0

#if PLATFORM_SUPPORTS_TEXTURE_STREAMING
extern TAutoConsoleVariable<int32> CVarSetTextureStreaming;
#endif

extern TAutoConsoleVariable<float> CVarStreamingBoost;
extern TAutoConsoleVariable<float> CVarStreamingScreenSizeEffectiveMax;
extern TAutoConsoleVariable<int32> CVarStreamingUseFixedPoolSize;
extern TAutoConsoleVariable<int32> CVarStreamingPoolSize;
extern TAutoConsoleVariable<int32> CVarStreamingMaxTempMemoryAllowed;
extern TAutoConsoleVariable<int32> CVarStreamingShowWantedMips;
extern TAutoConsoleVariable<int32> CVarStreamingHLODStrategy;
extern TAutoConsoleVariable<float> CVarStreamingHiddenPrimitiveScale;
extern TAutoConsoleVariable<int32> CVarStreamingSplitRequestSizeThreshold;
extern TAutoConsoleVariable<float> CVarStreamingMipBias;

typedef TArray<int32, TMemStackAllocator<> > FStreamingRequests;
typedef TArray<const UTexture2D*, TInlineAllocator<12> > FRemovedTextureArray;

#define NUM_BANDWIDTHSAMPLES 512
#define NUM_LATENCYSAMPLES 512

/** Streaming priority: Linear distance factor from 0 to MAX_STREAMINGDISTANCE. */
#define MAX_STREAMINGDISTANCE	10000.0f
#define MAX_MIPDELTA			5.0f
#define MAX_LASTRENDERTIME		90.0f


class UTexture2D;
class UPrimitiveComponent;
class FTextureBoundsVisibility;
class FDynamicComponentTextureManager;
template<typename T>
class FAsyncTask;
class FAsyncTextureStreaming;


struct FStreamingTexture;
struct FStreamingContext;
struct FStreamingHandlerTextureBase;
struct FTexturePriority;

/**
 * Checks whether a UTexture2D is supposed to be streaming.
 * @param Texture	Texture to check
 * @return			true if the UTexture2D is supposed to be streaming
 */
bool IsStreamingTexture( const UTexture2D* Texture2D );


/**
 *	Helper struct that represents a texture and the parameters used for sorting and streaming out high-res mip-levels.
 **/
struct FTextureSortElement
{
	/**
	 *	Constructor.
	 *
	 *	@param InTexture					- The texture to represent
	 *	@param InSize						- Size of the whole texture and all current mip-levels, in bytes
	 *	@param bInIsCharacterTexture		- 1 if this is a character texture, otherwise 0
	 *	@param InTextureDataAddress			- Starting address of the texture data
	 *	@param InNumRequiredResidentMips	- Minimum number of mip-levels required to stay in memory
	 */
	FTextureSortElement( UTexture2D* InTexture, int32 InSize, int32 bInIsCharacterTexture, uint32 InTextureDataAddress, int32 InNumRequiredResidentMips )
	:	Texture( InTexture )
	,	Size( InSize )
	,	bIsCharacterTexture( bInIsCharacterTexture )
	,	TextureDataAddress( InTextureDataAddress )
	,	NumRequiredResidentMips( InNumRequiredResidentMips )
	{
	}
	/** The texture that this element represents */
	UTexture2D*	Texture;
	/** Size of the whole texture and all current mip-levels, in bytes. */
	int32			Size;
	/** 1 if this is a character texture, otherwise 0 */
	int32			bIsCharacterTexture;
	/** Starting address of the texture data. */
	uint32		TextureDataAddress;			
	/** Minimum number of mip-levels required to stay in memory. */
	int32			NumRequiredResidentMips;
};


enum ETextureStreamingType
{
	StreamType_Static,
	StreamType_Dynamic,
	StreamType_Forced,
	StreamType_LastRenderTime,
	StreamType_Orphaned,
	StreamType_Other,
};

static const TCHAR* GStreamTypeNames[] =
{
	TEXT("Static"),
	TEXT("Dynamic"),
	TEXT("Forced"),
	TEXT("LastRenderTime"),
	TEXT("Orphaned"),
	TEXT("Other"),
};

struct FTexturePriority
{
	FTexturePriority( bool InCanDropMips, float InRetentionPriority, float InLoadOrderPriority, int32 InTextureIndex, const UTexture2D* InTexture)
	:	bCanDropMips( InCanDropMips )
	,	RetentionPriority( InRetentionPriority )
	,	LoadOrderPriority( InLoadOrderPriority )
	,	TextureIndex( InTextureIndex )
	,	Texture( InTexture )
	{
	}
	/** True if we allows this texture to drop mips to fit in budget. */
	bool bCanDropMips;
	/** Texture retention priority, higher value means it should be kept in memory. */
	float RetentionPriority;
	/** Texture load order priority, higher value means it should load first. */
	float LoadOrderPriority;
	/** Index into the FStreamingManagerTexture::StreamingTextures array. */
	int32 TextureIndex;
	/** The texture to stream. Only used for validation. */
	const UTexture2D* Texture;
};



#if STATS
struct FTextureStreamingStats
{
	FTextureStreamingStats( UTexture2D* InTexture2D, ETextureStreamingType InType, int32 InResidentMips, int32 InWantedMips, int32 InMostResidentMips, int32 InResidentSize, int32 InWantedSize, int32 InMaxSize, int32 InMostResidentSize, float InBoostFactor, float InPriority, int32 InTextureIndex )
	:	TextureName( InTexture2D->GetFullName() )
	,	SizeX( InTexture2D->GetSizeX() )
	,	SizeY( InTexture2D->GetSizeY() )
	,	NumMips( InTexture2D->GetNumMips() )
	,	LODBias( InTexture2D->GetCachedLODBias() )
	,	LastRenderTime( FMath::Clamp( InTexture2D->Resource ? (FApp::GetCurrentTime() - InTexture2D->Resource->LastRenderTime) : 1000000.0, 0.0, 1000000.0) )
	,	StreamType( InType )
	,	ResidentMips( InResidentMips )
	,	WantedMips( InWantedMips )
	,	MostResidentMips( InMostResidentMips )
	,	ResidentSize( InResidentSize )
	,	WantedSize( InWantedSize )
	,	MaxSize( InMaxSize )
	,	MostResidentSize( InMostResidentSize )
	,	BoostFactor( InBoostFactor )
	,	Priority( InPriority )
	,	TextureIndex( InTextureIndex )
	{
	}
	/** Mirror of UTexture2D::GetName() */
	FString		TextureName;
	/** Mirror of UTexture2D::SizeX */
	int32		SizeX;
	/** Mirror of UTexture2D::SizeY */
	int32		SizeY;
	/** Mirror of UTexture2D::Mips.Num() */
	int32		NumMips;
	/** Mirror of UTexture2D::GetCachedLODBias() */
	int32		LODBias;
	/** How many seconds since it was last rendered: FApp::GetCurrentTime() - UTexture2D::Resource->LastRenderTime */
	float		LastRenderTime;
	/** What streaming heuristics were primarily used for this texture. */
	ETextureStreamingType	StreamType;
	/** Number of resident mip levels. */
	int32		ResidentMips;
	/** Number of wanted mip levels. */
	int32		WantedMips;
	/** Most number of mip-levels this texture has ever had resident in memory. */
	int32		MostResidentMips;
	/** Number of bytes currently in memory. */
	int32		ResidentSize;
	/** Number of bytes we want in memory. */
	int32		WantedSize;
	/** Number of bytes we could potentially stream in. */
	int32		MaxSize;
	/** Most number of bytes this texture has ever had resident in memory. */
	int32		MostResidentSize;
	/** Temporary boost of the streaming distance factor. */
	float		BoostFactor;
	/** Texture priority */
	float		Priority;
	/** Index into the FStreamingManagerTexture::StreamingTextures array. */
	int32		TextureIndex;
};
#endif

/**
 * Helper struct for temporary information for one frame of processing texture streaming.
 */
struct FStreamingContext
{
	FStreamingContext( bool bProcessEverything, UTexture2D* IndividualStreamingTexture, bool bInCollectTextureStats )
	{
		Reset( bProcessEverything, IndividualStreamingTexture, bInCollectTextureStats );
	}

	/**
	 * Initializes all variables for the one frame.
	 * @param bProcessEverything			If true, process all resources with no throttling limits
	 * @param IndividualStreamingTexture	A specific texture to be fully processed this frame, or NULL
	 * @param bInCollectTextureStats		Whether to fill in the TextureStats array this frame
	 */
	void Reset( bool bProcessEverything, UTexture2D* IndividualStreamingTexture, bool bInCollectTextureStats );

	/**
	 * Adds in the stats from another context.
	 *
	 * @param Other		Context to add stats from
	 */
	void AddStats( const FStreamingContext& Other );

	/** Whether the platform RHI supports texture memory stats. */
	bool bRHISupportsMemoryStats;
	/** Currently allocated number of bytes in the texture pool, or INDEX_NONE if bRHISupportsMemoryStats is false. */
	int64 AllocatedMemorySize;
	/** Pending Adjustments to allocated texture memory, due to async reallocations, etc. */
	int64 PendingMemoryAdjustment;
	/** Whether to fill in TextureStats this frame. */
	bool bCollectTextureStats;
#if STATS
	/** Stats for all textures. */
	TArray<FTextureStreamingStats>	TextureStats;
#endif

	// Stats for this frame.
	uint64 ThisFrameTotalRequestSize;
	uint64 ThisFrameTotalLightmapRequestSize;
	uint64 ThisFrameNumStreamingTextures;
	uint64 ThisFrameNumRequestsInCancelationPhase;
	uint64 ThisFrameNumRequestsInUpdatePhase;
	uint64 ThisFrameNumRequestsInFinalizePhase;
	uint64 ThisFrameTotalIntermediateTexturesSize;
	uint64 ThisFrameNumIntermediateTextures;
	uint64 ThisFrameTotalStreamingTexturesSize;
	uint64 ThisFrameTotalStreamingTexturesMaxSize;
	uint64 ThisFrameTotalLightmapMemorySize;
	uint64 ThisFrameTotalLightmapDiskSize;
	uint64 ThisFrameTotalHLODMemorySize;
	uint64 ThisFrameTotalHLODDiskSize;
	uint64 ThisFrameTotalMipCountIncreaseRequestsInFlight;
	uint64 ThisFrameOptimalWantedSize;
	/** Number of bytes using StaticTexture heuristics this frame, currently in memory. */
	uint64 ThisFrameTotalStaticTextureHeuristicSize;
	/** Number of bytes using DynmicTexture heuristics this frame, currently in memory. */
	uint64 ThisFrameTotalDynamicTextureHeuristicSize;
	/** Number of bytes using LastRenderTime heuristics this frame, currently in memory. */
	uint64 ThisFrameTotalLastRenderHeuristicSize;
	/** Number of bytes using ForcedIntoMemory heuristics this frame, currently in memory. */
	uint64 ThisFrameTotalForcedHeuristicSize;

	/** Conservative amount of memory currently available or available after some pending requests are completed. */
	int64 AvailableNow;
	/** Amount of memory expected to be available once all streaming is completed. Depends on WantedMips */
	int64 AvailableLater;
	/** Amount of memory immediatly available temporary streaming data (pending textures). Depends on the delta between RequestedMips and ResidentMips */
	int64 AvailableTempMemory;
};

