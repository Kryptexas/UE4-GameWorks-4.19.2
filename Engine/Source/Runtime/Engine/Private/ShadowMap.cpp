// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "TextureLayout.h"
#include "TargetPlatform.h"

#if WITH_EDITOR
	// NOTE: We're only counting the top-level mip-map for the following variables.
	/** Total number of texels allocated for all shadowmap textures. */
	ENGINE_API uint64 GNumShadowmapTotalTexels = 0;
	/** Number of shadowmap textures generated. */
	ENGINE_API int32 GNumShadowmapTextures = 0;
	/** Total number of mapped texels. */
	ENGINE_API uint64 GNumShadowmapMappedTexels = 0;
	/** Total number of unmapped texels. */
	ENGINE_API uint64 GNumShadowmapUnmappedTexels = 0;
	/** Whether to allow cropping of unmapped borders in lightmaps and shadowmaps. Controlled by BaseEngine.ini setting. */
	extern ENGINE_API bool GAllowLightmapCropping;
	/** Total shadowmap texture memory size (in bytes), including GShadowmapTotalStreamingSize. */
	ENGINE_API uint64 GShadowmapTotalSize = 0;
	/** Total texture memory size for streaming shadowmaps. */
	ENGINE_API uint64 GShadowmapTotalStreamingSize = 0;

	/** Whether to allow lighting builds to generate streaming lightmaps. */
	extern ENGINE_API bool GAllowStreamingLightmaps;
#endif

UShadowMapTexture2D::UShadowMapTexture2D(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	LODGroup = TEXTUREGROUP_Shadowmap;
}

void FShadowMap::Serialize(FArchive& Ar)
{
	Ar << LightGuids;
}

void FShadowMap::Cleanup()
{
	BeginCleanup(this);
}

void FShadowMap::FinishCleanup()
{
	delete this;
}

#if WITH_EDITOR

struct FShadowMapAllocation
{
	FShadowMap2D*				ShadowMap;
	UObject*					TextureOuter;
	/** Upper-left X-coordinate in the texture atlas. */
	int32						OffsetX;
	/** Upper-left Y-coordinate in the texture atlas. */
	int32						OffsetY;
	/** Total number of texels along the X-axis. */
	int32						TotalSizeX;
	/** Total number of texels along the Y-axis. */
	int32						TotalSizeY;
	/** The rectangle of mapped texels within this mapping that is placed in the texture atlas. */
	FIntRect					MappedRect;
	ELightMapPaddingType		PaddingType;

	TMap<ULightComponent*, TArray<FQuantizedSignedDistanceFieldShadowSample> > ShadowMapData;

	/** Bounds of the primitive that the mapping is applied to. */
	FBoxSphereBounds	Bounds;
	/** Bit-field with shadowmap flags. */
	EShadowMapFlags		ShadowmapFlags;

	FShadowMapAllocation()
	{
		PaddingType = GAllowLightmapPadding ? LMPT_NormalPadding : LMPT_NoPadding;
		MappedRect.Min.X = 0;
		MappedRect.Min.Y = 0;
		MappedRect.Max.X = 0;
		MappedRect.Max.Y = 0;
	}
};

/** Largest boundingsphere radius to use when packing shadowmaps into a texture atlas. */
float GMaxShadowmapRadius = 2000.0f;

/** Whether to try to pack procbuilding lightmaps/shadowmaps into the same texture. */
extern bool GGroupComponentLightmaps;

struct FShadowMapPendingTexture : FTextureLayout
{
	TArray<FShadowMapAllocation*> Allocations;

	UObject* Outer;

	/** Bounds for all shadowmaps in this texture. */
	FBoxSphereBounds	Bounds;
	/** Bit-field with shadowmap flags that are shared among all shadowmaps in this texture. */
	EShadowMapFlags		ShadowmapFlags;

	/**
	 * Minimal initialization constructor.
	 */
	FShadowMapPendingTexture(uint32 InSizeX,uint32 InSizeY):
		FTextureLayout(1,1,InSizeX,InSizeY,true)
	{}

	bool AddElement(FShadowMapAllocation& Allocation, const bool bForceIntoThisTexture = false);

	/*
	 * Begin encoding the textures
	 *
	 * @param	InWorld	World in which the textures exist
	 */
	void StartEncoding(UWorld* InWorld);
};

bool FShadowMapPendingTexture::AddElement(FShadowMapAllocation& Allocation, const bool bForceIntoThisTexture)
{
	if( !bForceIntoThisTexture )
	{
		// Must be in the same package
		if ( Outer != Allocation.TextureOuter )
		{
			return false;
		}
	}

	const FBoxSphereBounds NewBounds = Bounds + Allocation.Bounds;
	const bool bEmptyTexture = Allocations.Num() == 0;
	if ( !bEmptyTexture && !bForceIntoThisTexture )
	{
		// Don't mix streaming lightmaps with non-streaming lightmaps.
		if ( (ShadowmapFlags & SMF_Streamed) != (Allocation.ShadowmapFlags & SMF_Streamed) )
		{
			return false;
		}

		// If this is a streaming shadowmap?
		if ( ShadowmapFlags & SMF_Streamed )
		{
			bool bPerformDistanceCheck = true;

			// Don't pack together shadowmaps that are too far apart.
			if ( bPerformDistanceCheck && NewBounds.SphereRadius > GMaxShadowmapRadius && NewBounds.SphereRadius > (Bounds.SphereRadius + SMALL_NUMBER) )
			{
				return false;
			}
		}
	}

	// 4-byte alignment, no more padding
	uint32 PaddingBaseX = 0;
	uint32 PaddingBaseY = 0;

	// See if the new one will fit in the existing texture
	if ( !FTextureLayout::AddElement(PaddingBaseX,PaddingBaseY,Allocation.MappedRect.Width(),Allocation.MappedRect.Height()) )
	{
		return false;
	}

	// Position the shadow-maps in the middle of their padded space.
	Allocation.OffsetX = PaddingBaseX;
	Allocation.OffsetY = PaddingBaseY;
	Bounds = bEmptyTexture ? Allocation.Bounds : NewBounds;

	// Add the shadow-map to the list of shadow-maps allocated space in this texture.
	Allocations.Add(&Allocation);

	return true;
}

void FShadowMapPendingTexture::StartEncoding(UWorld* InWorld)
{
	// Create the shadow-map texture.
	UShadowMapTexture2D* Texture = new(Outer) UShadowMapTexture2D(FPostConstructInitializeProperties());
	Texture->Filter			= GUseBilinearLightmaps ? TF_Linear : TF_Nearest;
	// Signed distance field textures get stored in linear space, since they need more precision near .5.
	Texture->SRGB			= false;
	Texture->LODGroup		= TEXTUREGROUP_Shadowmap;
	Texture->ShadowmapFlags	= ShadowmapFlags;
	Texture->Source.Init2DWithMipChain(GetSizeX(), GetSizeY(), TSF_BGRA8);
	Texture->MipGenSettings = TMGS_LeaveExistingMips;
	Texture->CompressionNone = true;

	int32 TextureSizeX = Texture->Source.GetSizeX();
	int32 TextureSizeY = Texture->Source.GetSizeY();

	{
		// Create the uncompressed top mip-level.
		TArray< TArray<FFourDistanceFieldSamples> > MipData;
		FShadowMap2D::EncodeSingleTexture(*this, Texture, MipData);

		// Copy the mip-map data into the UShadowMapTexture2D's mip-map array.
		for(int32 MipIndex = 0;MipIndex < MipData.Num();MipIndex++)
		{
			FColor* DestMipData = (FColor*)Texture->Source.LockMip(MipIndex);
			uint32 MipSizeX = FMath::Max(1,TextureSizeX >> MipIndex);
			uint32 MipSizeY = FMath::Max(1,TextureSizeY >> MipIndex);

			for(uint32 Y = 0;Y < MipSizeY;Y++)
			{
				for(uint32 X = 0;X < MipSizeX;X++)
				{
					const FFourDistanceFieldSamples& SourceSample = MipData[MipIndex][Y * MipSizeX + X];
					DestMipData[ Y * MipSizeX + X ] = FColor(SourceSample.Samples[0].Distance, SourceSample.Samples[1].Distance, SourceSample.Samples[2].Distance, SourceSample.Samples[3].Distance);
				}
			}

			Texture->Source.UnlockMip(MipIndex);
		}
	}

	// Update stats.
	int32 TextureSize			= Texture->CalcTextureMemorySizeEnum( TMC_AllMips );
	GNumShadowmapTotalTexels	+= TextureSizeX * TextureSizeY;
	GNumShadowmapTextures++;
	GShadowmapTotalSize			+= TextureSize;
	GShadowmapTotalStreamingSize += (ShadowmapFlags & SMF_Streamed) ? TextureSize : 0;
	UPackage* TexturePackage = Texture->GetOutermost();

	for ( int32 LevelIndex=0; TexturePackage && LevelIndex < InWorld->GetNumLevels(); LevelIndex++ )
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		UPackage* LevelPackage = Level->GetOutermost();
		if ( TexturePackage == LevelPackage )
		{
			Level->ShadowmapTotalSize += float(TextureSize) / 1024.0f;
			break;
		}
	}

	// Update the texture resource.
	Texture->BeginCachePlatformData();
	Texture->FinishCachePlatformData();
	Texture->UpdateResource();
}

static TIndirectArray<FShadowMapAllocation> PendingShadowMaps;
static uint32 PendingShadowMapSize;
/** If true, update the status when encoding light maps */
bool FShadowMap2D::bUpdateStatus = true;

#endif 

FShadowMap2D* FShadowMap2D::AllocateShadowMap(
	UObject* Outer, 
	const TMap<ULightComponent*,FShadowMapData2D*>& ShadowMapData,
	const FBoxSphereBounds& Bounds, 
	ELightMapPaddingType InPaddingType,
	EShadowMapFlags InShadowmapFlags)
{
#if WITH_EDITOR

	check(ShadowMapData.Num() > 0);

	// Create a new shadow-map.
	FShadowMap2D* ShadowMap = new FShadowMap2D(ShadowMapData);

	int32 SizeX = -1;
	int32 SizeY = -1;
	int32 LightIndex = 0;

	for (TMap<ULightComponent*, FShadowMapData2D*>::TConstIterator It(ShadowMapData); It; ++It)
	{
		if (LightIndex == 0)
		{
			SizeX = It.Value()->GetSizeX();
			SizeY = It.Value()->GetSizeY();
		}
		else
		{
			check(SizeX == It.Value()->GetSizeX() && SizeY == It.Value()->GetSizeY());
		}

		LightIndex++;
	}

	check(SizeX != -1 && SizeY != -1);

	// Add a pending allocation for this shadow-map.
	FShadowMapAllocation* Allocation = new(PendingShadowMaps) FShadowMapAllocation;
	Allocation->ShadowMap		= ShadowMap;
	Allocation->TextureOuter	= Outer->GetOutermost();
	Allocation->TotalSizeX		= SizeX;
	Allocation->TotalSizeY		= SizeY;
	Allocation->MappedRect		= FIntRect( 0, 0, SizeX, SizeY );
	Allocation->PaddingType		= InPaddingType;
	Allocation->Bounds			= Bounds;
	Allocation->ShadowmapFlags	= InShadowmapFlags;

	if ( !GAllowStreamingLightmaps )
	{
		Allocation->ShadowmapFlags = EShadowMapFlags( Allocation->ShadowmapFlags & ~SMF_Streamed );
	}

	for (TMap<ULightComponent*, FShadowMapData2D*>::TConstIterator It(ShadowMapData); It; ++It)
	{
		const FShadowMapData2D* RawData = It.Value();
		TArray<FQuantizedSignedDistanceFieldShadowSample>& DistanceFieldShadowData = Allocation->ShadowMapData.Add(It.Key(), TArray<FQuantizedSignedDistanceFieldShadowSample>());

		switch (RawData->GetType())
		{
		case FShadowMapData2D::SHADOW_SIGNED_DISTANCE_FIELD_DATA:
		case FShadowMapData2D::SHADOW_SIGNED_DISTANCE_FIELD_DATA_QUANTIZED:
			// If the data is already quantized, this will just copy the data
			RawData->Quantize(DistanceFieldShadowData);
			break;
		default:
			check(0);
		}

		delete RawData;

		// Track the size of pending light-maps.
		PendingShadowMapSize += Allocation->TotalSizeX * Allocation->TotalSizeY;
	}

	return ShadowMap;
#else
	return NULL;
#endif 
}

FShadowMap2D::FShadowMap2D() :
	Texture(NULL),
	CoordinateScale(FVector2D(0, 0)),
	CoordinateBias(FVector2D(0, 0))
{
	for (int Channel = 0; Channel < ARRAY_COUNT(bChannelValid); Channel++)
	{
		bChannelValid[Channel] = false;
	}
}

FShadowMap2D::FShadowMap2D(const TMap<ULightComponent*,FShadowMapData2D*>& ShadowMapData) :
	Texture(NULL),
	CoordinateScale(FVector2D(0, 0)),
	CoordinateBias(FVector2D(0, 0))
{
	for (int Channel = 0; Channel < ARRAY_COUNT(bChannelValid); Channel++)
	{
		bChannelValid[Channel] = false;
	}

	for (TMap<ULightComponent*, FShadowMapData2D*>::TConstIterator It(ShadowMapData); It; ++It)
	{
		LightGuids.Add(It.Key()->LightGuid);
	}
}

void FShadowMap2D::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Texture);
}

void FShadowMap2D::Serialize(FArchive& Ar)
{
	FShadowMap::Serialize(Ar);
	
	if( Ar.IsCooking() && !Ar.CookingTarget()->SupportsFeature(ETargetPlatformFeatures::DistanceFieldShadows) )
	{
		UShadowMapTexture2D* Dummy = NULL;
		Ar << Dummy;
	}
	else
	{
		Ar << Texture;
	}

	Ar << CoordinateScale << CoordinateBias;

	for (int Channel = 0; Channel < ARRAY_COUNT(bChannelValid); Channel++)
	{
		Ar << bChannelValid[Channel];
	}
}

FShadowMapInteraction FShadowMap2D::GetInteraction() const
{
	if (Texture)
	{
		return FShadowMapInteraction::Texture(Texture, CoordinateScale, CoordinateBias, bChannelValid);
	}
	return FShadowMapInteraction::None();
}


#if WITH_EDITOR

struct FCompareShadowMaps
{
	FORCEINLINE bool operator()( const FShadowMapAllocation& A, const FShadowMapAllocation& B ) const
	{
		return FMath::Max(B.TotalSizeX, B.TotalSizeY) < FMath::Max(A.TotalSizeX, A.TotalSizeY);
	}
};

/**
 * Executes all pending shadow-map encoding requests.
 * @param	InWorld				World in which the textures exist
 * @param	bLightingSuccessful	Whether the lighting build was successful or not.	 
 */
void FShadowMap2D::EncodeTextures(UWorld* InWorld , bool bLightingSuccessful )
{
	if ( bLightingSuccessful )
	{
		GWarn->BeginSlowTask( NSLOCTEXT("ShadowMap2D", "BeginEncodingShadowMapsTask", "Encoding shadow-maps"), false );
		const int32 PackedLightAndShadowMapTextureSize = InWorld->GetWorldSettings()->PackedLightAndShadowMapTextureSize;

		// Reset the pending shadow-map size.
		PendingShadowMapSize = 0;

		Sort(PendingShadowMaps.GetTypedData(), PendingShadowMaps.Num(), FCompareShadowMaps());

		// Allocate texture space for each light-map.
		TIndirectArray<FShadowMapPendingTexture> PendingTextures;

		for(int32 ShadowMapIndex = 0;ShadowMapIndex < PendingShadowMaps.Num();ShadowMapIndex++)
		{
			FShadowMapAllocation& Allocation = PendingShadowMaps[ShadowMapIndex];

			// Find an existing texture which the light-map can be stored in.
			FShadowMapPendingTexture* Texture = NULL;
			for(int32 TextureIndex = 0;TextureIndex < PendingTextures.Num();TextureIndex++)
			{
				FShadowMapPendingTexture& ExistingTexture = PendingTextures[TextureIndex];

				// See if the new one will fit in the existing texture
				if ( ExistingTexture.AddElement( Allocation ) )
				{
					Texture = &ExistingTexture;
					break;
				}
			}

			// If there is no appropriate texture, create a new one.
			if(!Texture)
			{
				int32 NewTextureSizeX = PackedLightAndShadowMapTextureSize;
				int32 NewTextureSizeY = PackedLightAndShadowMapTextureSize;

				if(Allocation.MappedRect.Width() > NewTextureSizeX || Allocation.MappedRect.Height() > NewTextureSizeY)
				{
					NewTextureSizeX = FMath::RoundUpToPowerOfTwo(Allocation.MappedRect.Width());
					NewTextureSizeY = FMath::RoundUpToPowerOfTwo(Allocation.MappedRect.Height());
				}

				// If there is no existing appropriate texture, create a new one.
				Texture				= ::new(PendingTextures) FShadowMapPendingTexture(NewTextureSizeX,NewTextureSizeY);
				Texture->Outer		= Allocation.TextureOuter;
				Texture->Bounds		= Allocation.Bounds;
				Texture->ShadowmapFlags = Allocation.ShadowmapFlags;
				verify( Texture->AddElement( Allocation, true ) );
			}
		}
		
		for(int32 TextureIndex = 0;TextureIndex < PendingTextures.Num();TextureIndex++)
		{
			if (bUpdateStatus && (TextureIndex % 20) == 0)
			{
				GWarn->UpdateProgress(TextureIndex, PendingTextures.Num());
			}

			FShadowMapPendingTexture& PendingTexture = PendingTextures[TextureIndex];
			PendingTexture.StartEncoding(InWorld);
		}

		PendingTextures.Empty();
		PendingShadowMaps.Empty();

		GWarn->EndSlowTask();
	}
	else
	{
		PendingShadowMaps.Empty();
	}
}

void FShadowMap2D::EncodeSingleTexture(FShadowMapPendingTexture& PendingTexture, UShadowMapTexture2D* Texture, TArray< TArray<FFourDistanceFieldSamples> >& MipData)
{
	TArray<FFourDistanceFieldSamples>* TopMipData = new(MipData) TArray<FFourDistanceFieldSamples>();
	TopMipData->Empty(PendingTexture.GetSizeX() * PendingTexture.GetSizeY());
	TopMipData->AddZeroed(PendingTexture.GetSizeX() * PendingTexture.GetSizeY());
	int32 TextureSizeX = Texture->Source.GetSizeX();
	int32 TextureSizeY = Texture->Source.GetSizeY();

	for (int32 AllocationIndex = 0;AllocationIndex < PendingTexture.Allocations.Num();AllocationIndex++)
	{
		FShadowMapAllocation& Allocation = *PendingTexture.Allocations[AllocationIndex];
		bool bChannelUsed[4] = {0};

		for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
		{
			for (TMap<ULightComponent*, TArray<FQuantizedSignedDistanceFieldShadowSample>>::TIterator It(Allocation.ShadowMapData); It; ++It)
			{
				if (It.Key()->ShadowMapChannel == ChannelIndex)
				{
					bChannelUsed[ChannelIndex] = true;
					const TArray<FQuantizedSignedDistanceFieldShadowSample>& SourceSamples = It.Value();

					// Copy the raw data for this light-map into the raw texture data array.
					for (int32 Y = Allocation.MappedRect.Min.Y; Y < Allocation.MappedRect.Max.Y; ++Y)
					{
						for (int32 X = Allocation.MappedRect.Min.X; X < Allocation.MappedRect.Max.X; ++X)
						{
							int32 DestY = Y - Allocation.MappedRect.Min.Y + Allocation.OffsetY;
							int32 DestX = X - Allocation.MappedRect.Min.X + Allocation.OffsetX;

							FFourDistanceFieldSamples& DestSample = (*TopMipData)[DestY * TextureSizeX + DestX];
							const FQuantizedSignedDistanceFieldShadowSample& SourceSample = SourceSamples[Y * Allocation.TotalSizeX + X];

							if ( SourceSample.Coverage > 0 )
							{
								DestSample.Samples[ChannelIndex] = SourceSample;
							}
#if WITH_EDITOR
							if ( SourceSample.Coverage > 0 )
							{
								GNumShadowmapMappedTexels++;
							}
							else
							{
								GNumShadowmapUnmappedTexels++;
							}
#endif
						}
					}
				}
			}
		}

		// Link the shadow-map to the texture.
		Allocation.ShadowMap->Texture = Texture;

		// Free the shadow-map's raw data.
		for (TMap<ULightComponent*, TArray<FQuantizedSignedDistanceFieldShadowSample> >::TIterator It(Allocation.ShadowMapData); It; ++It)
		{
			It.Value().Empty();
		}
		
		int32 PaddedSizeX = Allocation.TotalSizeX;
		int32 PaddedSizeY = Allocation.TotalSizeY;
		int32 BaseX = Allocation.OffsetX - Allocation.MappedRect.Min.X;
		int32 BaseY = Allocation.OffsetY - Allocation.MappedRect.Min.Y;

#if WITH_EDITOR
		if (GLightmassDebugOptions.bPadMappings && (Allocation.PaddingType == LMPT_NormalPadding))
		{
			if ((PaddedSizeX - 2 > 0) && ((PaddedSizeY - 2) > 0))
			{
				PaddedSizeX -= 2;
				PaddedSizeY -= 2;
				BaseX += 1;
				BaseY += 1;
			}
		}
#endif	
		// Calculate the coordinate scale/biases for each shadow-map stored in the texture.
		Allocation.ShadowMap->CoordinateScale = FVector2D(
			(float)PaddedSizeX / (float)PendingTexture.GetSizeX(),
			(float)PaddedSizeY / (float)PendingTexture.GetSizeY()
			);
		Allocation.ShadowMap->CoordinateBias = FVector2D(
			(float)BaseX / (float)PendingTexture.GetSizeX(),
			(float)BaseY / (float)PendingTexture.GetSizeY()
			);

		for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
		{
			Allocation.ShadowMap->bChannelValid[ChannelIndex] = bChannelUsed[ChannelIndex];
		}
	}

	const uint32 NumMips = FMath::Max(FMath::CeilLogTwo(TextureSizeX),FMath::CeilLogTwo(TextureSizeY)) + 1;

	for (uint32 MipIndex = 1;MipIndex < NumMips;MipIndex++)
	{
		const uint32 SourceMipSizeX = FMath::Max(1, TextureSizeX >> (MipIndex - 1));
		const uint32 SourceMipSizeY = FMath::Max(1, TextureSizeY >> (MipIndex - 1));
		const uint32 DestMipSizeX = FMath::Max(1, TextureSizeX >> MipIndex);
		const uint32 DestMipSizeY = FMath::Max(1, TextureSizeY >> MipIndex);

		// Downsample the previous mip-level, taking into account which texels are mapped.
		TArray<FFourDistanceFieldSamples>* NextMipData = new(MipData) TArray<FFourDistanceFieldSamples>();
		NextMipData->Empty(DestMipSizeX * DestMipSizeY);
		NextMipData->AddZeroed(DestMipSizeX * DestMipSizeY);
		const uint32 MipFactorX = SourceMipSizeX / DestMipSizeX;
		const uint32 MipFactorY = SourceMipSizeY / DestMipSizeY;

		for (uint32 Y = 0;Y < DestMipSizeY;Y++)
		{
			for (uint32 X = 0;X < DestMipSizeX;X++)
			{
				float AccumulatedFilterableComponents[4][FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents];

				for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
				{
					for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
					{
						AccumulatedFilterableComponents[ChannelIndex][i] = 0;
					}
				}
				uint32 Coverage[4] = {0};

				for (uint32 SourceY = Y * MipFactorY;SourceY < (Y + 1) * MipFactorY;SourceY++)
				{
					for (uint32 SourceX = X * MipFactorX;SourceX < (X + 1) * MipFactorX;SourceX++)
					{
						for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
						{
							const FFourDistanceFieldSamples& FourSourceSamples = MipData[MipIndex - 1][SourceY * SourceMipSizeX + SourceX];
							const FQuantizedSignedDistanceFieldShadowSample& SourceSample = FourSourceSamples.Samples[ChannelIndex];

							if (SourceSample.Coverage)
							{
								for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
								{
									AccumulatedFilterableComponents[ChannelIndex][i] += SourceSample.GetFilterableComponent(i) * SourceSample.Coverage;
								}

								Coverage[ChannelIndex] += SourceSample.Coverage;
							}
						}
					}
				}

				FFourDistanceFieldSamples& FourDestSamples = (*NextMipData)[Y * DestMipSizeX + X];

				for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
				{
					FQuantizedSignedDistanceFieldShadowSample& DestSample = FourDestSamples.Samples[ChannelIndex];

					if (Coverage[ChannelIndex])
					{
						for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
						{
							DestSample.SetFilterableComponent(AccumulatedFilterableComponents[ChannelIndex][i] / (float)Coverage[ChannelIndex], i);
						}

						DestSample.Coverage = (uint8)(Coverage[ChannelIndex] / (MipFactorX * MipFactorY));
					}
					else
					{
						for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
						{
							AccumulatedFilterableComponents[ChannelIndex][i] = 0;
						}
						DestSample.Coverage = 0;
					}
				}
			}
		}
	}

	const FIntPoint Neighbors[] = 
	{
		// Check immediate neighbors first
		FIntPoint(1,0),
		FIntPoint(0,1),
		FIntPoint(-1,0),
		FIntPoint(0,-1),
		// Check diagonal neighbors if no immediate neighbors are found
		FIntPoint(1,1),
		FIntPoint(-1,1),
		FIntPoint(-1,-1),
		FIntPoint(1,-1)
	};

	// Extrapolate texels which are mapped onto adjacent texels which are not mapped to avoid artifacts when using texture filtering.
	for (int32 MipIndex = 0;MipIndex < MipData.Num();MipIndex++)
	{
		uint32 MipSizeX = FMath::Max(1,TextureSizeX >> MipIndex);
		uint32 MipSizeY = FMath::Max(1,TextureSizeY >> MipIndex);

		for (uint32 DestY = 0;DestY < MipSizeY;DestY++)
		{
			for (uint32 DestX = 0;DestX < MipSizeX;DestX++)
			{
				FFourDistanceFieldSamples& FourDestSamples = MipData[MipIndex][DestY * MipSizeX + DestX];

				for (int32 ChannelIndex = 0; ChannelIndex < 4; ChannelIndex++)
				{
					FQuantizedSignedDistanceFieldShadowSample& DestSample = FourDestSamples.Samples[ChannelIndex];

					if (DestSample.Coverage == 0)
					{
						float ExtrapolatedFilterableComponents[FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents];

						for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
						{
							ExtrapolatedFilterableComponents[i] = 0;
						}

						for (int32 NeighborIndex = 0; NeighborIndex < ARRAY_COUNT(Neighbors); NeighborIndex++)
						{
							if (DestY + Neighbors[NeighborIndex].Y >= 0 
								&& DestY + Neighbors[NeighborIndex].Y < MipSizeY
								&& DestX + Neighbors[NeighborIndex].X >= 0 
								&& DestX + Neighbors[NeighborIndex].X < MipSizeX)
							{
								const FFourDistanceFieldSamples& FourNeighborSamples = MipData[MipIndex][(DestY + Neighbors[NeighborIndex].Y) * MipSizeX + DestX + Neighbors[NeighborIndex].X];
								const FQuantizedSignedDistanceFieldShadowSample& NeighborSample = FourNeighborSamples.Samples[ChannelIndex];

								if (NeighborSample.Coverage > 0)
								{
									if (DestY + Neighbors[NeighborIndex].Y * 2 >= 0 
										&& DestY + Neighbors[NeighborIndex].Y * 2 < MipSizeY
										&& DestX + Neighbors[NeighborIndex].X * 2 >= 0 
										&& DestX + Neighbors[NeighborIndex].X * 2 < MipSizeX)
									{
										// Lookup the second neighbor in the first neighbor's direction
										//@todo - check the second neighbor's coverage?
										const FFourDistanceFieldSamples& SecondFourNeighborSamples = MipData[MipIndex][(DestY + Neighbors[NeighborIndex].Y * 2) * MipSizeX + DestX + Neighbors[NeighborIndex].X * 2];
										const FQuantizedSignedDistanceFieldShadowSample& SecondNeighborSample = FourNeighborSamples.Samples[ChannelIndex];

										for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
										{
											// Extrapolate while maintaining the first derivative, which is especially important for signed distance fields
											ExtrapolatedFilterableComponents[i] = NeighborSample.GetFilterableComponent(i) * 2.0f - SecondNeighborSample.GetFilterableComponent(i);
										}
									}
									else
									{
										// Couldn't find a second neighbor to use for extrapolating, just copy the neighbor's values
										for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
										{
											ExtrapolatedFilterableComponents[i] = NeighborSample.GetFilterableComponent(i);
										}
									}
									break;
								}
							}
						}
						for (int32 i = 0; i < FQuantizedSignedDistanceFieldShadowSample::NumFilterableComponents; i++)
						{
							DestSample.SetFilterableComponent(ExtrapolatedFilterableComponents[i], i);
						}
					}
				}
			}
		}
	}
}

#endif

FArchive& operator<<(FArchive& Ar,FShadowMap*& R)
{
	uint32 ShadowMapType = FShadowMap::SMT_None;

	if (Ar.IsSaving() && R != NULL && R->GetShadowMap2D())
	{
		ShadowMapType = FShadowMap::SMT_2D;
	}

	Ar << ShadowMapType;

	if (Ar.IsLoading() && ShadowMapType == FShadowMap::SMT_2D)
	{
		R = new FShadowMap2D();
	}

	if (R != NULL)
	{
		R->Serialize(Ar);
		
		// Dump old lightmaps
		if( Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_COMBINED_LIGHTMAP_TEXTURES )
		{
			delete R;
			R = NULL;
		}
	}

	return Ar;
}
