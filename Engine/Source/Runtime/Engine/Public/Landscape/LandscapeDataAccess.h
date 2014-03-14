// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
LandscapeDataAccess.h: Classes for the editor to access to Landscape data
=============================================================================*/

#ifndef _LANDSCAPEDATAACCESS_H
#define _LANDSCAPEDATAACCESS_H


#define LANDSCAPE_VALIDATE_DATA_ACCESS 1
#define LANDSCAPE_ZSCALE		(1.0f/128.0f)
#define LANDSCAPE_INV_ZSCALE	128.0f

#define LANDSCAPE_XYOFFSET_SCALE	(1.0f/256.f)
#define LANDSCAPE_INV_XYOFFSET_SCALE	256.f

#if WITH_EDITOR

namespace LandscapeDataAccess
{
	const int32 MaxValue = 65535;
	const float MidValue = 32768.f;
	// Reserved 2 bits for other purpose
	// Most significant bit - Visibility, 0 is visible(default), 1 is invisible
	// 2nd significant bit - Triangle flip, not implemented yet
	FORCEINLINE float GetLocalHeight(uint16 Height)
	{
		return ((float)Height - MidValue) * LANDSCAPE_ZSCALE;
	}

	FORCEINLINE uint16 GetTexHeight(float Height)
	{
		return FMath::Clamp<float>(Height * LANDSCAPE_INV_ZSCALE + MidValue, 0.f, MaxValue);
	}
};

//
// FLandscapeDataInterface
//

//@todo.VC10: Apparent VC10 compiler bug here causes an access violation in UnlockMip in Shipping builds
#if _MSC_VER
PRAGMA_DISABLE_OPTIMIZATION
#endif

struct FLandscapeDataInterface
{
private:

	struct FLockedMipDataInfo
	{
		FLockedMipDataInfo()
		:	LockCount(0)
		{}

		TArray<uint8> MipData;
		int32 LockCount;
	};

public:
	// Constructor
	// @param bInAutoDestroy - shall we automatically clean up when the last 
	FLandscapeDataInterface()
	{}

	void* LockMip(UTexture2D* Texture, int32 MipLevel)
	{
		check(MipLevel < Texture->Source.GetNumMips());

		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		if( MipInfo == NULL )
		{
			MipInfo = &LockedMipInfoMap.Add(Texture, TArray<FLockedMipDataInfo>() );
			for (int32 i = 0; i < Texture->Source.GetNumMips(); ++i)
			{
				MipInfo->Add(FLockedMipDataInfo());
			}
		}

		if( (*MipInfo)[MipLevel].MipData.Num() == 0 )
		{
			Texture->Source.GetMipData((*MipInfo)[MipLevel].MipData, MipLevel);
		}
		(*MipInfo)[MipLevel].LockCount++;

		return (*MipInfo)[MipLevel].MipData.GetTypedData();
	}

	void UnlockMip(UTexture2D* Texture, int32 MipLevel)
	{
		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		check(MipInfo);

		if ((*MipInfo)[MipLevel].LockCount <= 0)
			return;
		(*MipInfo)[MipLevel].LockCount--;
		if( (*MipInfo)[MipLevel].LockCount == 0 )
		{
			check( (*MipInfo)[MipLevel].MipData.Num() != 0 );
			(*MipInfo)[MipLevel].MipData.Empty();
		}		
	}

private:
	TMap<UTexture2D*, TArray<FLockedMipDataInfo> > LockedMipInfoMap;
};

//@todo.VC10: Apparent VC10 compiler bug here causes an access violation in UnlockMip in Shipping builds
#if _MSC_VER
PRAGMA_ENABLE_OPTIMIZATION
#endif

	
//
// FLandscapeComponentDataInterface
//
struct FLandscapeComponentDataInterface
{
	friend struct FLandscapeDataInterface;

	// tors
	FLandscapeComponentDataInterface(ULandscapeComponent* InComponent, int32 InMipLevel = 0)
	:	Component(InComponent),
		HeightMipData(NULL),
		bNeedToDeleteDataInterface(false),
		MipLevel(InMipLevel)
	{
		if (Component->GetLandscapeInfo())
		{
			DataInterface = Component->GetLandscapeInfo()->GetDataInterface();
		}
		else
		{
			DataInterface = new FLandscapeDataInterface;
			bNeedToDeleteDataInterface = true;
		}

		// Offset and stride for this component's data in heightmap texture
		HeightmapStride = Component->HeightmapTexture->Source.GetSizeX() >> MipLevel;
		HeightmapComponentOffsetX = FMath::Round( (float)(Component->HeightmapTexture->Source.GetSizeX() >> MipLevel) * Component->HeightmapScaleBias.Z );
		HeightmapComponentOffsetY = FMath::Round( (float)(Component->HeightmapTexture->Source.GetSizeY() >> MipLevel) * Component->HeightmapScaleBias.W );
		HeightmapSubsectionOffset = (Component->SubsectionSizeQuads + 1) >> MipLevel;

		ComponentSizeVerts = (Component->ComponentSizeQuads + 1) >> MipLevel;
		SubsectionSizeVerts = (Component->SubsectionSizeQuads + 1) >> MipLevel;

		if (MipLevel < Component->HeightmapTexture->Source.GetNumMips())
		{
			HeightMipData = (FColor*)DataInterface->LockMip(Component->HeightmapTexture, MipLevel);
			if (Component->XYOffsetmapTexture)
			{
				XYOffsetMipData = (FColor*)DataInterface->LockMip(Component->XYOffsetmapTexture,MipLevel);
			}
		}

		XYOffsetMipData = NULL;
	}

	~FLandscapeComponentDataInterface()
	{
		if (HeightMipData)
		{
			DataInterface->UnlockMip(Component->HeightmapTexture,MipLevel);
			if (Component->XYOffsetmapTexture)
			{
				DataInterface->UnlockMip(Component->XYOffsetmapTexture,MipLevel);
			}
		}

		if (bNeedToDeleteDataInterface)
		{
			delete DataInterface;
			DataInterface = NULL;
		}
	}

	// Accessors
	void VertexIndexToXY(int32 VertexIndex, int32& OutX, int32& OutY) const
	{
//#if LANDSCAPE_VALIDATE_DATA_ACCESS
//		check(MipLevel == 0);
//#endif
		OutX = VertexIndex % ComponentSizeVerts;
		OutY = VertexIndex / ComponentSizeVerts;
	}

	// Accessors
	void QuadIndexToXY(int32 QuadIndex, int32& OutX, int32& OutY) const
	{
//#if LANDSCAPE_VALIDATE_DATA_ACCESS
//		check(MipLevel == 0);
//#endif
		OutX = QuadIndex % ComponentSizeVerts;
		OutY = QuadIndex / ComponentSizeVerts;
	}

	void ComponentXYToSubsectionXY(int32 CompX, int32 CompY, int32& SubNumX, int32& SubNumY, int32& SubX, int32& SubY ) const
	{
		// We do the calculation as if we're looking for the previous vertex.
		// This allows us to pick up the last shared vertex of every subsection correctly.
		SubNumX = (CompX-1) / (SubsectionSizeVerts - 1);
		SubNumY = (CompY-1) / (SubsectionSizeVerts - 1);
		SubX = (CompX-1) % (SubsectionSizeVerts - 1) + 1;
		SubY = (CompY-1) % (SubsectionSizeVerts - 1) + 1;

		// If we're asking for the first vertex, the calculation above will lead
		// to a negative SubNumX/Y, so we need to fix that case up.
		if( SubNumX < 0 )
		{
			SubNumX = 0;
			SubX = 0;
		}

		if( SubNumY < 0 )
		{
			SubNumY = 0;
			SubY = 0;
		}
	}

	FColor* GetRawHeightData() const
	{
		return HeightMipData;
	}

	void UnlockRawHeightData() const
	{
		DataInterface->UnlockMip(Component->HeightmapTexture,0);
	}

	/* Return the raw heightmap data exactly same size for Heightmap texture which belong to only this component */
	void GetHeightmapTextureData( TArray<FColor>& OutData )
	{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		check(HeightMipData);
#endif
		int32 HeightmapSize = ((Component->SubsectionSizeQuads+1) * Component->NumSubsections) >> MipLevel;
		OutData.Empty(FMath::Square(HeightmapSize));
		OutData.AddUninitialized(FMath::Square(HeightmapSize));

		for( int32 SubY=0;SubY<HeightmapSize;SubY++ )
		{
			// X/Y of the vertex we're looking at in component's coordinates.
			int32 CompY = SubY;

			// UV coordinates of the data offset into the texture
			int32 TexV = SubY + HeightmapComponentOffsetY;

			// Copy the data
			FMemory::Memcpy( &OutData[CompY * HeightmapSize], &HeightMipData[HeightmapComponentOffsetX + TexV * HeightmapStride], HeightmapSize * sizeof(FColor));
		}
	}

	bool GetWeightmapTextureData( ULandscapeLayerInfoObject* LayerInfo, TArray<uint8>& OutData )
	{
		int32 LayerIdx = INDEX_NONE;
		for (int32 Idx = 0; Idx < Component->WeightmapLayerAllocations.Num(); Idx++)
		{
			if ( Component->WeightmapLayerAllocations[Idx].LayerInfo == LayerInfo )
			{
				LayerIdx = Idx;
				break;
			}
		}
		if (LayerIdx < 0)
		{
			return false;
		}
		if ( Component->WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex >= Component->WeightmapTextures.Num() )
		{
			return false;
		}
		if ( Component->WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel >= 4 )
		{
			return false;
		}

		int32 WeightmapSize = (Component->SubsectionSizeQuads+1) * Component->NumSubsections;
		OutData.Empty(FMath::Square(WeightmapSize));
		OutData.AddUninitialized(FMath::Square(WeightmapSize));

		FColor* WeightMipData = (FColor*)DataInterface->LockMip(Component->WeightmapTextures[Component->WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex], 0);

		// Channel remapping
		int32 ChannelOffsets[4] = {(int32)STRUCT_OFFSET(FColor,R),(int32)STRUCT_OFFSET(FColor,G),(int32)STRUCT_OFFSET(FColor,B),(int32)STRUCT_OFFSET(FColor,A)};

		uint8* SrcTextureData = (uint8*)WeightMipData + ChannelOffsets[Component->WeightmapLayerAllocations[LayerIdx].WeightmapTextureChannel];

		for( int32 i=0;i<FMath::Square(WeightmapSize);i++ )
		{
			OutData[i] = SrcTextureData[i*4];
		}

		DataInterface->UnlockMip(Component->WeightmapTextures[Component->WeightmapLayerAllocations[LayerIdx].WeightmapTextureIndex], 0);
		return true;
	}

	FColor* GetHeightData( int32 LocalX, int32 LocalY ) const
	{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		check(Component);
		check(HeightMipData);
		check(LocalX >=0 && LocalY >=0 && LocalX < ComponentSizeVerts && LocalY < ComponentSizeVerts );
#endif

		int32 SubNumX;
		int32 SubNumY;
		int32 SubX;
		int32 SubY;
		ComponentXYToSubsectionXY(LocalX, LocalY, SubNumX, SubNumY, SubX, SubY );
		
		return &HeightMipData[SubX + SubNumX*SubsectionSizeVerts + HeightmapComponentOffsetX + (SubY+SubNumY*SubsectionSizeVerts+HeightmapComponentOffsetY)*HeightmapStride];
	}

	FColor* GetXYOffsetData( int32 LocalX, int32 LocalY ) const
	{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		check(Component);
		check(LocalX >=0 && LocalY >=0 && LocalX < Component->ComponentSizeQuads+1 && LocalY < Component->ComponentSizeQuads+1);
#endif

		const int32 WeightmapSize = (Component->SubsectionSizeQuads+1) * Component->NumSubsections;
		int32 SubNumX;
		int32 SubNumY;
		int32 SubX;
		int32 SubY;
		ComponentXYToSubsectionXY(LocalX, LocalY, SubNumX, SubNumY, SubX, SubY );

		return &XYOffsetMipData[SubX + SubNumX*SubsectionSizeVerts + (SubY+SubNumY*SubsectionSizeVerts)*WeightmapSize];
	}

	uint16 GetHeight( int32 LocalX, int32 LocalY ) const
	{
		FColor* Texel = GetHeightData(LocalX, LocalY);
		return (Texel->R << 8) + Texel->G;
	}

	uint16 GetHeight( int32 VertexIndex ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		return GetHeight( X, Y );
	}

	void GetXYOffset( int32 X, int32 Y, float& XOffset, float& YOffset ) const
	{
		if (XYOffsetMipData)
		{
			FColor* Texel = GetXYOffsetData(X, Y);
			XOffset = ((float)((Texel->R << 8) + Texel->G) - 32768.f) * LANDSCAPE_XYOFFSET_SCALE;
			YOffset = ((float)((Texel->B << 8) + Texel->A) - 32768.f) * LANDSCAPE_XYOFFSET_SCALE;
		}
		else
		{
			XOffset = YOffset = 0.f;
		}
	}

	void GetXYOffset( int32 VertexIndex, float& XOffset, float& YOffset ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetXYOffset( X, Y, XOffset, YOffset );
	}

	FVector GetLocalVertex( int32 LocalX, int32 LocalY ) const
	{
		const float ScaleFactor = (float)Component->ComponentSizeQuads / (float)(ComponentSizeVerts-1);
		float XOffset, YOffset;
		GetXYOffset(LocalX, LocalY, XOffset, YOffset);
		return FVector( LocalX * ScaleFactor + XOffset, LocalY * ScaleFactor + YOffset, LandscapeDataAccess::GetLocalHeight( GetHeight(LocalX, LocalY) ) );
	}

	void GetLocalTangentVectors( int32 LocalX, int32 LocalY, FVector& LocalTangentX, FVector& LocalTangentY, FVector& LocalTangentZ ) const
	{
		// Note: these are still pre-scaled, just not rotated

		FColor* Data = GetHeightData( LocalX, LocalY );
		LocalTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
		LocalTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
		LocalTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(LocalTangentZ.X)+FMath::Square(LocalTangentZ.Y)));
		LocalTangentX = FVector(-LocalTangentZ.Z, 0.f, LocalTangentZ.X);
		LocalTangentY = FVector(0.f, LocalTangentZ.Z, -LocalTangentZ.Y);
	}

	FVector GetLocalVertex( int32 VertexIndex ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		return GetLocalVertex( X, Y );
	}

	void GetLocalTangentVectors( int32 VertexIndex, FVector& LocalTangentX, FVector& LocalTangentY, FVector& LocalTangentZ ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetLocalTangentVectors( X, Y, LocalTangentX, LocalTangentY, LocalTangentZ );
	}


	FVector GetWorldVertex( int32 LocalX, int32 LocalY ) const
	{
		return Component->ComponentToWorld.TransformPosition( GetLocalVertex( LocalX, LocalY ) );
	}

	void GetWorldTangentVectors( int32 LocalX, int32 LocalY, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		FColor* Data = GetHeightData( LocalX, LocalY );
		WorldTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
		WorldTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
		WorldTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(WorldTangentZ.X)+FMath::Square(WorldTangentZ.Y)));
		WorldTangentX = FVector(-WorldTangentZ.Z, 0.f, WorldTangentZ.X);
		WorldTangentY = FVector(0.f, WorldTangentZ.Z, -WorldTangentZ.Y);

		WorldTangentX = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentX);
		WorldTangentY = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentY);
		WorldTangentZ = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentZ);
	}

	void GetWorldPositionTangents( int32 LocalX, int32 LocalY, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		FColor* Data = GetHeightData( LocalX, LocalY );

		WorldTangentZ.X = 2.f * (float)Data->B / 255.f - 1.f;
		WorldTangentZ.Y = 2.f * (float)Data->A / 255.f - 1.f;
		WorldTangentZ.Z = FMath::Sqrt(1.f - (FMath::Square(WorldTangentZ.X)+FMath::Square(WorldTangentZ.Y)));
		WorldTangentX = FVector(WorldTangentZ.Z, 0.f, -WorldTangentZ.X);
		WorldTangentY = WorldTangentZ ^ WorldTangentX;

		uint16 Height = (Data->R << 8) + Data->G;

		const float ScaleFactor = (float)Component->ComponentSizeQuads / (float)(ComponentSizeVerts-1);
		float XOffset, YOffset;
		GetXYOffset(LocalX, LocalY, XOffset, YOffset);
		WorldPos = Component->ComponentToWorld.TransformPosition(FVector(LocalX * ScaleFactor + XOffset, LocalY * ScaleFactor + YOffset, LandscapeDataAccess::GetLocalHeight(Height)));
		WorldTangentX = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentX);
		WorldTangentY = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentY);
		WorldTangentZ = Component->ComponentToWorld.TransformVectorNoScale(WorldTangentZ);
	}

	FVector GetWorldVertex( int32 VertexIndex ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		return GetWorldVertex( X, Y );
	}

	void GetWorldTangentVectors( int32 VertexIndex, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetWorldTangentVectors( X, Y, WorldTangentX, WorldTangentY, WorldTangentZ );
	}

	void GetWorldPositionTangents( int32 VertexIndex, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		int32 X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetWorldPositionTangents( X, Y, WorldPos, WorldTangentX, WorldTangentY, WorldTangentZ );
	}

private:
	struct FLandscapeDataInterface* DataInterface;
	ULandscapeComponent* Component;

	// offset of this component's data into heightmap texture
	int32 HeightmapStride;
	int32 HeightmapComponentOffsetX;
	int32 HeightmapComponentOffsetY;
	int32 HeightmapSubsectionOffset;
	FColor* HeightMipData;
	FColor* XYOffsetMipData;

	int32 ComponentSizeVerts;
	int32 SubsectionSizeVerts;

	bool bNeedToDeleteDataInterface;
public:
	const int32 MipLevel;
};

// Helper functions
template<typename T>
void FillCornerValues(uint8& CornerSet, T* CornerValues);



#endif // WITH_EDITOR

#endif // _LANDSCAPEDATAACCESS_H
