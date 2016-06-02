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

	NumNonStreamingMips = InTexture->GetNumNonStreamingMips();
	bIsStreamingLightmap = IsStreamingLightmap( Texture );
	bIsLightmap = bIsStreamingLightmap || LODGroup == TEXTUREGROUP_Lightmap || LODGroup == TEXTUREGROUP_Shadowmap;
	bHasSplitRequest = false;
	bIsLastSplitRequest = false;
	bIsVisibleWantedMips = false;
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
	static auto CVarOnlyStreamInTextures = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.OnlyStreamInTextures"));
	const bool bOnlyStreamIn = CVarOnlyStreamInTextures->GetValueOnAnyThread() != 0;
	const int32 HLODStategy = CVarStreamingHLODStrategy.GetValueOnAnyThread();

	check(Texture && Texture->Resource);
	ResidentMips = Texture->ResidentMips;
	RequestedMips = Texture->RequestedMips;
	MinAllowedMips = 1;			//FMath::Min(Texture->ResidentMips, Texture->RequestedMips);
	MaxAllowedMips = MipCount;	//FMath::Max(Texture->ResidentMips, Texture->RequestedMips);
	STAT( MostResidentMips = FMath::Max(MostResidentMips, Texture->ResidentMips) );
	float LastRenderTimeForTexture = Texture->GetLastRenderTimeForStreaming();
	LastRenderTime = (FApp::GetCurrentTime() > LastRenderTimeForTexture) ? float( FApp::GetCurrentTime() - LastRenderTimeForTexture ) : 0.0f;

	bForceFullyLoad = Texture->ShouldMipLevelsBeForcedResident() || (bOnlyStreamIn && LastRenderTime < 300)
		|| LODGroup == TEXTUREGROUP_Skybox || (LODGroup == TEXTUREGROUP_HierarchicalLOD && HLODStategy == 2);

	TextureLODBias = Texture->GetCachedLODBias();
	bInFlight = false;
	bReadyForStreaming = IsStreamingTexture( Texture ) && IsReadyForStreaming( Texture );
	NumCinematicMipLevels = Texture->bUseCinematicMipLevels ? Texture->NumCinematicMipLevels : 0;
}

float FStreamingTexture::GetStreamingScale(float GlobalBias) const
{
	if (LODGroup == TEXTUREGROUP_Terrain_Heightmap || LODGroup == TEXTUREGROUP_Terrain_Weightmap) return BoostFactor;
	if (LODGroup == TEXTUREGROUP_Lightmap)	return BoostFactor * GLightmapStreamingFactor;
	if (LODGroup == TEXTUREGROUP_Shadowmap)	return BoostFactor * GShadowmapStreamingFactor;

	// When using the new metrics, the bounds are conservative and we can relax the required resolution.
	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnAnyThread() != 0;
	const float GlobalScale = FMath::Exp2(-GlobalBias) * (bUseNewMetrics ? 0.875f : 1.f);

	return BoostFactor * GlobalScale;
}

int32 FStreamingTexture::GetWantedMipsFromSize(float Size) const
{
	float WantedMipsFloat = 1.0f + FMath::Log2(FMath::Max(1.f, Size));
	// Using the new metrics has very conservative distance computation. Rounding to mip here compensates.
	int32 WantedMipsInt = FMath::CeilToInt(WantedMipsFloat);
	return FMath::Clamp<int32>(WantedMipsInt, MinAllowedMips, MaxAllowedMips);
}

/** Set the wanted mips from the async task data */
void FStreamingTexture::SetPerfectWantedMips(float MaxSize, float MaxSize_VisibleOnly, float MipBias, bool bIgnoreStreamingScale)
{
	const float HiddenScale = CVarStreamingHiddenPrimitiveScale.GetValueOnAnyThread();
	const float ExtraScale = bIgnoreStreamingScale ? 1.f : GetStreamingScale(MipBias);

	// Do this now before clamping the resolutions.
	bAsNeverStream = (MaxSize == FLT_MAX || MaxSize_VisibleOnly == FLT_MAX);

	int32 WantedMips_VisibleOnly = GetWantedMipsFromSize(MaxSize_VisibleOnly * ExtraScale);
	int32 WantedMips_Any = GetWantedMipsFromSize(MaxSize * HiddenScale * ExtraScale);

	// Load visible mips first, then non visible mips.
	if (WantedMips_Any > WantedMips_VisibleOnly && WantedMips_VisibleOnly <= ResidentMips)
	{
		PerfectWantedMips = WantedMips = WantedMips_Any;
		bIsVisibleWantedMips = false;
	}
	else
	{
		PerfectWantedMips = WantedMips = WantedMips_VisibleOnly;
		bIsVisibleWantedMips = true;
	}
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

				Texture2DResource->BeginUpdateMipCount( bAsNeverStream );
				TrackTextureEvent( this, Texture, bAsNeverStream, &Manager );
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

int32 FStreamingTexture::CalcRetentionPriority( )
{
	const int32 WantedMipsForPriority = GetWantedMipsForPriority();

	bool bMustKeep = bAsNeverStream;
	bool bIsVisible = !GStreamWithTimeFactor || IsVisible();
	bool bAlreadyLowRez = /*(PixelPerTexel > 1.5 && Distance > 1) ||*/ LODGroup == TEXTUREGROUP_HierarchicalLOD || bIsLightmap;
		
	// Don't consider dropping mips that give nothing. This is because streaming time has a high cost per texture.
	// Dropping many textures for not so much memory would affect the time it takes the streamer to recover.
	// We also don't want to prioritize by size, since bigger wanted mips are expected to have move visual impact.
	bool bGivesOffMemory = GetSize(WantedMipsForPriority) - GetSize(WantedMipsForPriority - 1) >= 256 * 1024; 

	int32 Priority = 0;
	if (bMustKeep)			Priority += 1024;
	if (bIsVisible)			Priority += 512;
	if (!bGivesOffMemory)	Priority += 256;
	if (bAlreadyLowRez)		Priority += 128;
	return Priority;
}

int32 FStreamingTexture::CalcLoadOrderPriority()
{
	const int32 WantedMipsForPriority = GetWantedMipsForPriority();

	bool bMustLoadFirst = bAsNeverStream || LODGroup == TEXTUREGROUP_Terrain_Heightmap;
	bool bIsVisible = !GStreamWithTimeFactor || IsVisible();
	bool bFarFromTargetMips = WantedMipsForPriority - ResidentMips > 2;
	bool bBigOnScreen = WantedMipsForPriority > 9.f || LODGroup == TEXTUREGROUP_HierarchicalLOD;

	int32 Priority = 0;
	if (bIsVisible)				Priority += 1024;
	if (!bIsLastSplitRequest)	Priority += 512; 
	if (bFarFromTargetMips)		Priority += 256;
	if (bMustLoadFirst)			Priority += 128;
	if (bBigOnScreen)			Priority += 64;
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
