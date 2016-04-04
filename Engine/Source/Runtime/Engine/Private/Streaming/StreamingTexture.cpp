// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
StreamingTexture.cpp: Definitions of classes used for texture.
=============================================================================*/

#include "EnginePrivate.h"
#include "StreamingTexture.h"
#include "StreamingManagerTexture.h"
#include "Engine/LightMapTexture2D.h"
#include "Engine/ShadowMapTexture2D.h"

FStreamingTexture::FStreamingTexture( UTexture2D* InTexture )
{
	Texture = InTexture;
	WantedMips = InTexture->ResidentMips;
	MipCount = InTexture->GetNumMips();
	PerfectWantedMips = InTexture->ResidentMips;
	STAT( MostResidentMips = InTexture->ResidentMips );
	LODGroup = (TextureGroup) InTexture->LODGroup;

	bIsHLODTexture = LODGroup == TEXTUREGROUP_HierarchicalLOD;

	// Landscape textures and HLOD textures should not be affected by MipBias
	//@TODO: Long term this should probably be a property of the texture group instead
	bCanBeAffectedByMipBias = true;
	if ((LODGroup == TEXTUREGROUP_Terrain_Heightmap) || (LODGroup == TEXTUREGROUP_Terrain_Weightmap) || bIsHLODTexture)
	{
		bCanBeAffectedByMipBias = false;
	}

	NumNonStreamingMips = InTexture->GetNumNonStreamingMips();
	ForceLoadRefCount = 0;
	bIsStreamingLightmap = IsStreamingLightmap( Texture );
	bIsLightmap = bIsStreamingLightmap || LODGroup == TEXTUREGROUP_Lightmap || LODGroup == TEXTUREGROUP_Shadowmap;
	bUsesStaticHeuristics = false;
	bUsesDynamicHeuristics = false;
	bUsesLastRenderHeuristics = false;
	bUsesForcedHeuristics = false;
	bUsesOrphanedHeuristics = false;
	bHasSplitRequest = false;
	bIsLastSplitRequest = false;
	BoostFactor = 1.0f;
	InstanceRemovedTimestamp = -FLT_MAX;
	LastRenderTimeRefCountTimestamp = -FLT_MAX;
	LastRenderTimeRefCount = 0;

	for ( int32 MipIndex=1; MipIndex <= MAX_TEXTURE_MIP_COUNT; ++MipIndex )
	{
		TextureSizes[MipIndex - 1] = Texture->CalcTextureMemorySize( FMath::Min(MipIndex, MipCount) );
	}

	UpdateCachedInfo();
}

void FStreamingTexture::UpdateCachedInfo( )
{
	check(Texture && Texture->Resource);
	ResidentMips = Texture->ResidentMips;
	RequestedMips = Texture->RequestedMips;
	MinAllowedMips = 1;			//FMath::Min(Texture->ResidentMips, Texture->RequestedMips);
	MaxAllowedMips = MipCount;	//FMath::Max(Texture->ResidentMips, Texture->RequestedMips);
	MaxAllowedOptimalMips = MaxAllowedMips;
	STAT( MostResidentMips = FMath::Max(MostResidentMips, Texture->ResidentMips) );
	float LastRenderTimeForTexture = Texture->GetLastRenderTimeForStreaming();
	LastRenderTime = (FApp::GetCurrentTime() > LastRenderTimeForTexture) ? float( FApp::GetCurrentTime() - LastRenderTimeForTexture ) : 0.0f;
	MinDistance = MAX_STREAMINGDISTANCE;
	bForceFullyLoad = Texture->ShouldMipLevelsBeForcedResident() || (ForceLoadRefCount > 0);
	TextureLODBias = Texture->GetCachedLODBias();
	bInFlight = false;
	bReadyForStreaming = IsStreamingTexture( Texture ) && IsReadyForStreaming( Texture );
	NumCinematicMipLevels = Texture->bUseCinematicMipLevels ? Texture->NumCinematicMipLevels : 0;
}

ETextureStreamingType FStreamingTexture::GetStreamingType() const
{
	if ( bUsesForcedHeuristics || bForceFullyLoad )
	{
		return StreamType_Forced;
	}
	else if ( bUsesDynamicHeuristics )
	{
		return StreamType_Dynamic;
	}
	else if ( bUsesStaticHeuristics )
	{
		return StreamType_Static;
	}
	else if ( bUsesOrphanedHeuristics )
	{
		return StreamType_Orphaned;
	}
	else if ( bUsesLastRenderHeuristics )
	{
		return StreamType_LastRenderTime;
	}
	return StreamType_Other;
}

bool FStreamingTexture::UpdateMipCount(  FStreamingManagerTexture& Manager, FStreamingContext& Context )
{
	if ( !Texture || !bReadyForStreaming )
	{
		return false;
	}

	// If there is a pending load that attempts to load unrequired data, 
	// or if there is a pending unload that attempts to unload required data, try to cancel it.
	if ( bInFlight && 
		(( RequestedMips > ResidentMips && RequestedMips > WantedMips ) || 
		 ( RequestedMips < ResidentMips && RequestedMips < WantedMips )) )
	{
		// Try to cancel, but don't try anything else before next update.
		bReadyForStreaming = false;

		if ( Texture->CancelPendingMipChangeRequest() )
		{
			bInFlight = false;
			RequestedMips = ResidentMips;
			return true;
		}
	}
	// If we should load/unload some mips.
	else if ( !bInFlight && ResidentMips != WantedMips )
	{
		check(RequestedMips == ResidentMips);

		// Try to unload, but don't try anything else before next update.
		bReadyForStreaming = false;

		if ( Texture->PendingMipChangeRequestStatus.GetValue() == TexState_ReadyFor_Requests )
		{
			check(!Texture->bHasCancelationPending);

			bool bHasEnoughTempMemory = GetSize(WantedMips) <= Context.AvailableTempMemory;
			bool bHasEnoughAvailableNow = WantedMips <= ResidentMips || GetSize(WantedMips) <= Context.AvailableNow;

			// Check if there is enough space for temp memory.
			if ( bHasEnoughTempMemory && bHasEnoughAvailableNow )
			{
				FTexture2DResource* Texture2DResource = (FTexture2DResource*)Texture->Resource;

				if (WantedMips > ResidentMips) // If it is a load request.
				{
					// Manually update size as allocations are deferred/ happening in rendering thread.
					int64 LoadRequestSize = GetSize(WantedMips) - GetSize(ResidentMips);
					Context.ThisFrameTotalRequestSize			+= LoadRequestSize;
					Context.ThisFrameTotalLightmapRequestSize	+= bIsStreamingLightmap ? LoadRequestSize : 0;
					Context.AvailableNow						-= LoadRequestSize;
				}

				Context.AvailableTempMemory -= GetSize(WantedMips);

				bInFlight = true;
				RequestedMips = WantedMips;
				Texture->RequestedMips = RequestedMips;

				Texture2DResource->BeginUpdateMipCount( bForceFullyLoad );
				TrackTextureEvent( this, Texture, bForceFullyLoad, &Manager );
				return true;
			}
		}
		else
		{
			// Did UpdateStreamingTextures() miss a texture? Should never happen!
			UE_LOG(LogContentStreaming, Warning, TEXT("BeginUpdateMipCount failure! PendingMipChangeRequestStatus == %d, Resident=%d, Requested=%d, Wanted=%d"), Texture->PendingMipChangeRequestStatus.GetValue(), Texture->ResidentMips, Texture->RequestedMips, WantedMips );
		}
	}
	return false;
}

float FStreamingTexture::CalcRetentionPriority( )
{
	const int32 WantedMipsForPriority = GetWantedMipsForPriority();

	bool bMustKeep = LODGroup == TEXTUREGROUP_Terrain_Heightmap || bForceFullyLoad || WantedMipsForPriority <= MinAllowedMips;
	bool bIsVisible = !GStreamWithTimeFactor || IsVisible();
	bool bAlreadyLowRez = /*(PixelPerTexel > 1.5 && Distance > 1) ||*/ bIsHLODTexture || bIsLightmap;
		
	// Don't consider dropping mips that give nothing. This is because streaming time has a high cost per texture.
	// Dropping many textures for not so much memory would affect the time it takes the streamer to recover.
	// We also don't want to prioritize by size, since bigger wanted mips are expected to have move visual impact.
	bool bGivesOffMemory = GetSize(WantedMipsForPriority) - GetSize(WantedMipsForPriority - 1) >= 256 * 1024; 

	float Priority = 0;
	if (bMustKeep)			Priority += 1024.f;
	if (bAlreadyLowRez)		Priority += 512.f;
	if (!bGivesOffMemory)	Priority += 256.f;
	if (bIsVisible)			Priority += 128.f;

	return Priority;
}

float FStreamingTexture::CalcLoadOrderPriority()
{
	const int32 WantedMipsForPriority = GetWantedMipsForPriority();

	bool bMustLoadFirst = LODGroup == TEXTUREGROUP_Terrain_Heightmap || bForceFullyLoad;
	bool bIsVisible = !GStreamWithTimeFactor || IsVisible();
	bool bFarFromTargetMips = WantedMipsForPriority - ResidentMips > 2;
	bool bBigOnScreen = WantedMipsForPriority > 9.f || bIsHLODTexture;

	float Priority = 0;
	if (bMustLoadFirst)		Priority += 1024.f;
	if (!bIsLastSplitRequest)	Priority += 512.f; 
	if (bIsVisible)				Priority += 256.f;
	if (bFarFromTargetMips)		Priority += 128.f;
	if (bBigOnScreen)			Priority += 64.f;
	return Priority;
}

bool FStreamingTexture::IsReadyForStreaming( UTexture2D* Texture )
{
	return Texture->IsReadyForStreaming();
}

bool FStreamingTexture::IsStreamingLightmap( UTexture2D* Texture )
{
	ULightMapTexture2D* Lightmap = Cast<ULightMapTexture2D>(Texture);
	UShadowMapTexture2D* Shadowmap = Cast<UShadowMapTexture2D>(Texture);

	if ( Lightmap && Lightmap->LightmapFlags & LMF_Streamed )
	{
		return true;
	}
	else if ( Shadowmap && Shadowmap->ShadowmapFlags & SMF_Streamed )
	{
		return true;
	}
	return false;
}
