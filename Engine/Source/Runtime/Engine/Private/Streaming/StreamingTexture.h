// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
StreamingTexture.h: Definitions of classes used for texture streaming.
=============================================================================*/

#pragma once

#include "TextureStreamingHelpers.h"

/*-----------------------------------------------------------------------------
	FStreamingTexture, the streaming system's version of UTexture2D.
-----------------------------------------------------------------------------*/

/** Self-contained structure to manage a streaming texture, possibly on a separate thread. */
struct FStreamingTexture
{
	FStreamingTexture( UTexture2D* InTexture );

	/** Reinitializes the cached variables from UTexture2D. */
	void UpdateCachedInfo( );

	ETextureStreamingType GetStreamingType() const;

	/**
	 * Checks whether a UTexture2D is ready for streaming.
	 *
	 * @param Texture	Texture to check
	 * @return			true if the UTexture2D is ready to be streamed in or out
	 */
	static bool IsReadyForStreaming( UTexture2D* Texture );

	/**
	 * Checks whether a UTexture2D is a streaming lightmap or shadowmap.
	 *
	 * @param Texture	Texture to check
	 * @return			true if the UTexture2D is a streaming lightmap or shadowmap
	 */
	static bool IsStreamingLightmap( UTexture2D* Texture );

	/**
	 * Returns the amount of memory used by the texture given a specified number of mip-maps, in bytes.
	 *
	 * @param MipCount	Number of mip-maps to account for
	 * @return			Total amount of memory used for the specified mip-maps, in bytes
	 */
	int32 GetSize( int32 InMipCount ) const
	{
		check(InMipCount >= 0 && InMipCount <= MAX_TEXTURE_MIP_COUNT);
		return TextureSizes[ InMipCount - 1 ];
	}

	/**
	 * Returns whether this primitive has been used rencently. Conservative.
	 *
	 * @return			true if visible
	 */
	FORCEINLINE bool IsVisible() const
	{
		return LastRenderTime < 5.f && LastRenderTime != MAX_FLT;
	}
	
	/**
	 * Return whether we allow to not stream the mips to fit in memory.
	 *
	 * @return			true if it is ok to drop the mips
	 */
	FORCEINLINE bool CanDropMips() const
	{
		return (LODGroup != TEXTUREGROUP_Terrain_Heightmap) && WantedMips > MinAllowedMips && !bForceFullyLoad;
	}


	/**
	 * Return the wanted mips to use for priority computates
	 *
	 * @return		the WantedMips
	 */
	FORCEINLINE int32 GetWantedMipsForPriority() const
	{
		if (bHasSplitRequest && !bIsLastSplitRequest)
		{
			return WantedMips + 1;
		}
		else
		{
			return WantedMips;
		}
	}

	/**
	 * Calculates a retention priority value for the textures. Higher value means more important.
	 * Retention is used determine which texture to drop when out of texture pool memory.
	 * The retention value must be stable, meaning it must not depend on the current state of the streamer.
	 * Not doing so could make the streamer go into a loop where is never stops dropping and loading different textures when out of budget.
	 * @return		Priority value
	 */
	float CalcRetentionPriority( );

	/**
	 * Calculates a load order priority value for the texture. Higher value means more important.
	 * Load Order can depend on the current state of resident mips, because it will not affect the streamer stability.
	 * @return		Priority value
	 */
	float CalcLoadOrderPriority();

		
	/**
	 * Not thread-safe: Sets the streaming index in the corresponding UTexture2D.
	 * @param ArrayIndex	Index into the FStreamingManagerTexture::StreamingTextures array
	 */
	void SetStreamingIndex( int32 ArrayIndex )
	{
		Texture->StreamingIndex = ArrayIndex;
	}

	/** Update texture streaming. Returns whether it did anything */
	bool UpdateMipCount(  FStreamingManagerTexture& Manager, FStreamingContext& Context );

	/** Texture to manage. */
	UTexture2D*		Texture;

	/** Cached number of mip-maps in the UTexture2D mip array (including the base mip) */
	int32			MipCount;
	/** Cached number of mip-maps in memory (including the base mip) */
	int32			ResidentMips;
	/** Cached number of mip-maps requested by the streaming system. */
	int32			RequestedMips;
	/** Cached number of mip-maps we would like to have in memory. */
	int32			WantedMips;
	/** Legacy: Same as WantedMips, but not clamped by fudge factor. */
	int32			PerfectWantedMips;
	/** Same as MaxAllowedMips, but not clamped by the -reducepoolsize command line option. */
	int32			MaxAllowedOptimalMips;
#if STATS
	/** Most number of mip-levels this texture has ever had resident in memory. */
	int32			MostResidentMips;
#endif
	/** Minimum number of mip-maps that this texture must keep in memory. */
	int32			MinAllowedMips;
	/** Maximum number of mip-maps that this texture can have in memory. */
	int32			MaxAllowedMips;
	/** Cached memory sizes for each possible mipcount. */
	int32			TextureSizes[MAX_TEXTURE_MIP_COUNT + 1];
	/** Ref-count for how many ULevels want this texture fully-loaded. */
	int32			ForceLoadRefCount;

	/** Cached texture group. */
	TextureGroup	LODGroup;
	/** Cached LOD bias. */
	int32			TextureLODBias;
	/** Cached number of mipmaps that are not allowed to stream. */
	int32			NumNonStreamingMips;
	/** Cached number of cinematic (high-resolution) mip-maps. Normally not streamed in, unless the texture is forcibly fully loaded. */
	int32			NumCinematicMipLevels;

	/** Last time this texture was rendered, 0-based from GStartTime. */
	float			LastRenderTime;
	/** Distance to the closest instance of this texture, in world space units. Defaults to a specific value for non-staticmesh textures. */
	float			MinDistance;
	/** If non-zero, the most recent time an instance location was removed for this texture. */
	double			InstanceRemovedTimestamp;

	/** Set to FApp::GetCurrentTime() every time LastRenderTimeRefCount is modified. */
	double			LastRenderTimeRefCountTimestamp;
	/** Current number of instances that need LRT heuristics for this texture. */
	int32			LastRenderTimeRefCount;

	/**
	 * Temporary boost of the streaming distance factor.
	 * This factor is automatically reset to 1.0 after it's been used for mip-calculations.
	 */
	float			BoostFactor;

	/** Whether the texture should be forcibly fully loaded. */
	uint32			bForceFullyLoad : 1;
	/** Whether the texture is ready to be streamed in/out (cached from IsReadyForStreaming()). */
	uint32			bReadyForStreaming : 1;
	/** Whether the texture is currently being streamed in/out. */
	uint32			bInFlight : 1;
	/** Whether this is a streaming lightmap or shadowmap (cached from IsStreamingLightmap()). */
	uint32			bIsStreamingLightmap : 1;
	/** Whether this is a lightmap or shadowmap. */
	uint32			bIsLightmap : 1;
	/** Whether this texture uses StaticTexture heuristics. */
	uint32			bUsesStaticHeuristics : 1;
	/** Whether this texture uses DynamicTexture heuristics. */
	uint32			bUsesDynamicHeuristics : 1;
	/** Whether this texture uses LastRenderTime heuristics. */
	uint32			bUsesLastRenderHeuristics : 1;
	/** Whether this texture uses ForcedIntoMemory heuristics. */
	uint32			bUsesForcedHeuristics : 1;
	/** Whether this texture uses the OrphanedTexture heuristics. */
	uint32			bUsesOrphanedHeuristics : 1;
	/** Whether this texture has a split request strategy. */
	uint32			bHasSplitRequest : 1;
	/** Whether this is the second request to converge to the wanted mip. */
	uint32			bIsLastSplitRequest : 1;
	/** Can this texture be affected by mip bias? */
	uint32			bCanBeAffectedByMipBias : 1;
	/** Is this texture a HLOD texture? */
	//@TODO: Temporary: Done because there is currently no texture group for HLOD textures and adding one in a branch isn't ideal; this should be removed / replaced once there is a real HLOD texture group
	uint32			bIsHLODTexture : 1;
};
