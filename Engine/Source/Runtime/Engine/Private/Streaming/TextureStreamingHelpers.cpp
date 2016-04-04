// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureStreamingHelpers.cpp: Definitions of classes used for texture streaming.
=============================================================================*/

#include "EnginePrivate.h"
#include "TextureStreamingHelpers.h"
#include "Engine/Texture2D.h"
#include "GenericPlatformMemoryPoolStats.h"

/** Streaming stats */

DEFINE_STAT(STAT_GameThreadUpdateTime);
DEFINE_STAT(STAT_TexturePoolSize);
DEFINE_STAT(STAT_TexturePoolAllocatedSize);
DEFINE_STAT(STAT_OptimalTextureSize);
DEFINE_STAT(STAT_EffectiveStreamingPoolSize);
DEFINE_STAT(STAT_StreamingTexturesSize);
DEFINE_STAT(STAT_NonStreamingTexturesSize);
DEFINE_STAT(STAT_StreamingTexturesMaxSize);
DEFINE_STAT(STAT_StreamingOverBudget);
DEFINE_STAT(STAT_StreamingUnderBudget);
DEFINE_STAT(STAT_NumWantingTextures);

DEFINE_STAT(STAT_StreamingTextures);

DEFINE_STAT(STAT_TotalStaticTextureHeuristicSize);
DEFINE_STAT(STAT_TotalDynamicHeuristicSize);
DEFINE_STAT(STAT_TotalLastRenderHeuristicSize);
DEFINE_STAT(STAT_TotalForcedHeuristicSize);

DEFINE_STAT(STAT_LightmapMemorySize);
DEFINE_STAT(STAT_LightmapDiskSize);
DEFINE_STAT(STAT_HLODTextureMemorySize);
DEFINE_STAT(STAT_HLODTextureDiskSize);
DEFINE_STAT(STAT_IntermediateTexturesSize);
DEFINE_STAT(STAT_RequestSizeCurrentFrame);
DEFINE_STAT(STAT_RequestSizeTotal);
DEFINE_STAT(STAT_LightmapRequestSizeTotal);

DEFINE_STAT(STAT_IntermediateTextures);
DEFINE_STAT(STAT_RequestsInCancelationPhase);
DEFINE_STAT(STAT_RequestsInUpdatePhase);
DEFINE_STAT(STAT_RequestsInFinalizePhase);
DEFINE_STAT(STAT_StreamingLatency);
DEFINE_STAT(STAT_StreamingBandwidth);
DEFINE_STAT(STAT_GrowingReallocations);
DEFINE_STAT(STAT_ShrinkingReallocations);
DEFINE_STAT(STAT_FullReallocations);
DEFINE_STAT(STAT_FailedReallocations);
DEFINE_STAT(STAT_PanicDefragmentations);
DEFINE_STAT(STAT_DynamicStreamingTotal);
DEFINE_STAT(STAT_NumVisibleTexturesWithLowResolutions);
DEFINE_STAT(STAT_LevelsWithPostLoadRotations);

DEFINE_LOG_CATEGORY(LogContentStreaming);

ENGINE_API TAutoConsoleVariable<int32> CVarStreamingUseNewMetrics(
	TEXT("r.Streaming.UseNewMetrics"),
	0,
	TEXT("If non-zero, will use tight AABB bounds and improved texture factors."),
	ECVF_Default);


/** Whether the texture pool size has been artificially lowered for testing. */
bool GIsOperatingWithReducedTexturePool = false;

TAutoConsoleVariable<float> CVarStreamingBoost(
	TEXT("r.Streaming.Boost"),
	1.0f,
	TEXT("=1.0: normal\n")
	TEXT("<1.0: decrease wanted mip levels\n")
	TEXT(">1.0: increase wanted mip levels"),
	ECVF_Default
	);

TAutoConsoleVariable<float> CVarStreamingScreenSizeEffectiveMax(
	TEXT("r.Streaming.MaxEffectiveScreenSize"),
	0,
	TEXT("0: Use current actual vertical screen size\n")	
	TEXT("> 0: Clamp wanted mip size calculation to this value for the vertical screen size component."),
	ECVF_Scalability
	);

#if PLATFORM_SUPPORTS_TEXTURE_STREAMING
TAutoConsoleVariable<int32> CVarSetTextureStreaming(
	TEXT("r.TextureStreaming"),
	1,
	TEXT("Allows to define if texture streaming is enabled, can be changed at run time.\n")
	TEXT("0: off\n")
	TEXT("1: on (default)"),
	ECVF_Default | ECVF_RenderThreadSafe);
#endif

TAutoConsoleVariable<int32> CVarStreamingUseFixedPoolSize(
	TEXT("r.Streaming.UseFixedPoolSize"),
	0,
	TEXT("If non-zero, do not allow the pool size to change at run time."),
	ECVF_ReadOnly);

TAutoConsoleVariable<int32> CVarStreamingPoolSize(
	TEXT("r.Streaming.PoolSize"),
	-1,
	TEXT("-1: Default texture pool size, otherwise the size in MB"),
	ECVF_Scalability);

TAutoConsoleVariable<int32> CVarStreamingMaxTempMemoryAllowed(
	TEXT("r.Streaming.MaxTempMemoryAllowed"),
	50,
	TEXT("Maximum temporary memory used when streaming in or out texture mips.\n")
	TEXT("This memory contains mips used for the new updated texture.\n")
	TEXT("The value must be high enough to not be a limiting streaming speed factor.\n"),
	ECVF_Default);

TAutoConsoleVariable<int32> CVarStreamingShowWantedMips(
	TEXT("r.Streaming.ShowWantedMips"),
	0,
	TEXT("If non-zero, will limit resolution to wanted mip."),
	ECVF_Cheat);

TAutoConsoleVariable<int32> CVarStreamingHLODStrategy(
	TEXT("r.Streaming.HLODStrategy"),
	0,
	TEXT("Define the HLOD streaming strategy.\n")
	TEXT("0: stream\n")
	TEXT("1: stream only mip 0\n")
	TEXT("2: disable streaming"),
	ECVF_Default);

TAutoConsoleVariable<float> CVarStreamingHiddenPrimitiveScale(
	TEXT("r.Streaming.HiddenPrimitiveScale"),
	0.5,
	TEXT("Define the resolution scale to apply when not in range.\n")
	TEXT(".5: drop one mip\n")
	TEXT("1: ignore visiblity"),
	ECVF_Default
	);

TAutoConsoleVariable<int32> CVarStreamingSplitRequestSizeThreshold(
	TEXT("r.Streaming.SplitRequestSizeThreshold"),
	0, // Good value : 2 * 1024 * 1024,
	TEXT("Define how many IO pending request the streamer can generate.\n"),
	ECVF_Default
	);

// Used for scalability (GPU memory, streaming stalls)
TAutoConsoleVariable<float> CVarStreamingMipBias(
	TEXT("r.Streaming.MipBias"),
	0.0f,
	TEXT("0..x reduce texture quality for streaming by a floating point number.\n")
	TEXT("0: use full resolution (default)\n")
	TEXT("1: drop one mip\n")
	TEXT("2: drop two mips"),
	ECVF_Scalability
	);

/** For debugging purposes: Whether to consider the time factor when streaming. Turning it off is easier for debugging. */
bool GStreamWithTimeFactor		= true;

extern ENGINE_API UPrimitiveComponent* GDebugSelectedComponent;

/** the float table {-1.0f,1.0f} **/
float ENGINE_API GNegativeOneOneTable[2] = {-1.0f,1.0f};


/** Smaller value will stream out lightmaps more aggressively. */
float GLightmapStreamingFactor = 1.0f;

/** Smaller value will stream out shadowmaps more aggressively. */
float GShadowmapStreamingFactor = 0.09f;

/** For testing, finding useless textures or special demo purposes. If true, textures will never be streamed out (but they can be GC'd). 
* Caution: this only applies to unlimited texture pools (i.e. not consoles)
*/
bool GNeverStreamOutTextures = false;

/** Accumulated total time spent on dynamic primitives, in seconds. */
double GStreamingDynamicPrimitivesTime = 0.0;


/**
 * Checks whether a UTexture2D is supposed to be streaming.
 * @param Texture	Texture to check
 * @return			true if the UTexture2D is supposed to be streaming
 */
bool IsStreamingTexture( const UTexture2D* Texture2D )
{
	return Texture2D && Texture2D->bIsStreamable && Texture2D->NeverStream == false && Texture2D->GetNumMips() > UTexture2D::GetMinTextureResidentMipCount();
}

void FStreamingContext::Reset( bool bProcessEverything, UTexture2D* IndividualStreamingTexture, bool bInCollectTextureStats )
{
	bCollectTextureStats							= bInCollectTextureStats;
	ThisFrameTotalRequestSize						= 0;
	ThisFrameTotalLightmapRequestSize				= 0;
	ThisFrameNumStreamingTextures					= 0;
	ThisFrameNumRequestsInCancelationPhase			= 0;
	ThisFrameNumRequestsInUpdatePhase				= 0;
	ThisFrameNumRequestsInFinalizePhase				= 0;
	ThisFrameTotalIntermediateTexturesSize			= 0;
	ThisFrameNumIntermediateTextures				= 0;
	ThisFrameTotalStreamingTexturesSize				= 0;
	ThisFrameTotalStreamingTexturesMaxSize			= 0;
	ThisFrameTotalLightmapMemorySize				= 0;
	ThisFrameTotalLightmapDiskSize					= 0;
	ThisFrameTotalHLODMemorySize					= 0;
	ThisFrameTotalHLODDiskSize						= 0;
	ThisFrameTotalMipCountIncreaseRequestsInFlight	= 0;
	ThisFrameOptimalWantedSize						= 0;
	ThisFrameTotalStaticTextureHeuristicSize		= 0;
	ThisFrameTotalDynamicTextureHeuristicSize		= 0;
	ThisFrameTotalLastRenderHeuristicSize			= 0;
	ThisFrameTotalForcedHeuristicSize				= 0;

	AvailableNow = 0;
	AvailableLater = 0;
	AvailableTempMemory = 0;

	STAT( TextureStats.Empty() );

	AllocatedMemorySize	= INDEX_NONE;
	PendingMemoryAdjustment = INDEX_NONE;
		
	// Available texture memory, if supported by RHI. This stat is async as the renderer allocates the memory in its own thread so we
	// only query once and roughly adjust the values as needed.
	FTextureMemoryStats Stats;
	RHIGetTextureMemoryStats(Stats);
		
	bRHISupportsMemoryStats = Stats.IsUsingLimitedPoolSize();

	// Update stats if supported.
	if( bRHISupportsMemoryStats )
	{
		AllocatedMemorySize	= Stats.AllocatedMemorySize;
		PendingMemoryAdjustment = Stats.PendingMemoryAdjustment;
	
		// set total size for the pool (used to available)
		SET_MEMORY_STAT(STAT_TexturePoolAllocatedSize, AllocatedMemorySize);
		SET_MEMORY_STAT(STAT_TexturePoolSize, Stats.TexturePoolSize);
		SET_MEMORY_STAT(MCR_TexturePool, Stats.TexturePoolSize);
	}
	else
	{
		SET_MEMORY_STAT(STAT_TexturePoolAllocatedSize,0);
		SET_MEMORY_STAT(STAT_TexturePoolSize, 0);
		SET_MEMORY_STAT(MCR_TexturePool, 0);
	}
}

void FStreamingContext::AddStats( const FStreamingContext& Other )
{
	ThisFrameTotalRequestSize						+= Other.ThisFrameTotalRequestSize;						
	ThisFrameTotalLightmapRequestSize				+= Other.ThisFrameTotalLightmapRequestSize;
	ThisFrameNumStreamingTextures					+= Other.ThisFrameNumStreamingTextures;
	ThisFrameNumRequestsInCancelationPhase			+= Other.ThisFrameNumRequestsInCancelationPhase;
	ThisFrameNumRequestsInUpdatePhase				+= Other.ThisFrameNumRequestsInUpdatePhase;
	ThisFrameNumRequestsInFinalizePhase				+= Other.ThisFrameNumRequestsInFinalizePhase;
	ThisFrameTotalIntermediateTexturesSize			+= Other.ThisFrameTotalIntermediateTexturesSize;
	ThisFrameNumIntermediateTextures				+= Other.ThisFrameNumIntermediateTextures;
	ThisFrameTotalStreamingTexturesSize				+= Other.ThisFrameTotalStreamingTexturesSize;
	ThisFrameTotalStreamingTexturesMaxSize			+= Other.ThisFrameTotalStreamingTexturesMaxSize;
	ThisFrameTotalLightmapMemorySize				+= Other.ThisFrameTotalLightmapMemorySize;
	ThisFrameTotalLightmapDiskSize					+= Other.ThisFrameTotalLightmapDiskSize;
	ThisFrameTotalHLODMemorySize					+= Other.ThisFrameTotalHLODMemorySize;
	ThisFrameTotalHLODDiskSize						+= Other.ThisFrameTotalHLODDiskSize;
	ThisFrameTotalMipCountIncreaseRequestsInFlight	+= Other.ThisFrameTotalMipCountIncreaseRequestsInFlight;
	ThisFrameOptimalWantedSize						+= Other.ThisFrameOptimalWantedSize;
	ThisFrameTotalStaticTextureHeuristicSize		+= Other.ThisFrameTotalStaticTextureHeuristicSize;
	ThisFrameTotalDynamicTextureHeuristicSize		+= Other.ThisFrameTotalDynamicTextureHeuristicSize;
	ThisFrameTotalLastRenderHeuristicSize			+= Other.ThisFrameTotalLastRenderHeuristicSize;
	ThisFrameTotalForcedHeuristicSize				+= Other.ThisFrameTotalForcedHeuristicSize;
	bCollectTextureStats							= bCollectTextureStats || Other.bCollectTextureStats;
	STAT( TextureStats.Append( Other.TextureStats ) );
}
