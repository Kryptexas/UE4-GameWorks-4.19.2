// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldCompositionUtility.h: Support structures for world composition
=============================================================================*/

#pragma once

#ifndef __WorldCompositionUtility_h__
#define __WorldCompositionUtility_h__

#define WORLDTILE_LOD_PACKAGE_SUFFIX	TEXT("_LOD")
#define WORLDTILE_LOD_MAX_INDEX			4

/** World layer information for tile tagging */
class FWorldTileLayer
{
public:
	FWorldTileLayer()
		: Name(TEXT("Uncategorized"))
		, Reserved0(0)
		, Reserved1(ForceInit)
		, StreamingDistance(50000)
		, DistanceStreamingEnabled(true)
	{
	}

	bool operator==(const FWorldTileLayer& OtherLayer) const
	{
		return	(Name == OtherLayer.Name) && 
				(StreamingDistance == OtherLayer.StreamingDistance) &&
				(DistanceStreamingEnabled == OtherLayer.DistanceStreamingEnabled);

	}

	friend FArchive& operator<<( FArchive& Ar, FWorldTileLayer& D );

public:
	/** Human readable name for this layer */
	FString					Name;
	int32					Reserved0;
	FIntPoint				Reserved1;
	/** Distance starting from where tiles belonging to this layer will be streamed in */
	int32					StreamingDistance;
	bool					DistanceStreamingEnabled;
};

/** 
 *  Describes LOD entry in a world tile 
 */
class FWorldTileLODInfo
{
public:
	FWorldTileLODInfo()
		: RelativeStreamingDistance(10000)
		, GenDetailsPercentage(70.f)
		, GenMaxDeviation(5.f)
		, Reserved0(0)
		, Reserved1(0)
	{
	}

	friend FArchive& operator<<( FArchive& Ar, FWorldTileLayer& D );

	bool operator==(const FWorldTileLODInfo& OtherInfo) const
	{
		return (RelativeStreamingDistance == OtherInfo.RelativeStreamingDistance) &&
				(GenDetailsPercentage == OtherInfo.GenDetailsPercentage) &&
				(GenMaxDeviation == OtherInfo.GenMaxDeviation);
	}

public:
	/**  Relative to LOD0 streaming distance, absolute distance = LOD0 + StreamingDistanceDelta */
	int32	RelativeStreamingDistance;
	
	/**  LOD generation settings */
	float	GenDetailsPercentage;
	float	GenMaxDeviation;
	
	/**  Reserved for additional options */
	int32	Reserved0;
	int32	Reserved1;
};

/** 
 *  Tile information used by WorldComposition. 
 *  Defines properties necessary for tile positioning in the world. Stored with package summary 
 */
class FWorldTileInfo
{
public:
	FWorldTileInfo()
		: Position(0,0)
		, AbsolutePosition(0,0)
		, Bounds(0)
		, bAlwaysLoaded(false)
		, ZOrder(0)
	{
	}
	
	COREUOBJECT_API friend FArchive& operator<<( FArchive& Ar, FWorldTileInfo& D );

	bool operator==(const FWorldTileInfo& OtherInfo) const
	{
		return (Position == OtherInfo.Position) &&
				(Bounds.Min.Equals(OtherInfo.Bounds.Min, 0.5f)) &&
				(Bounds.Max.Equals(OtherInfo.Bounds.Max, 0.5f)) &&
				(bAlwaysLoaded == OtherInfo.bAlwaysLoaded) &&
				(ParentTilePackageName == OtherInfo.ParentTilePackageName) &&
				(Layer == OtherInfo.Layer) &&
				(ZOrder == OtherInfo.ZOrder) &&
				(LODList == OtherInfo.LODList);
	}
	
	/** @return Streaming distance for a particular tile LOD */
	int32 GetStreamingDistance(int32 LODIndex) const
	{
		int32 Distance = Layer.StreamingDistance;
		if (LODList.IsValidIndex(LODIndex))	
		{
			Distance+= LODList[LODIndex].RelativeStreamingDistance;
		}
		return Distance;
	}

	/** Reads FWorldTileInfo from a specified package */
	COREUOBJECT_API static bool Read(const FString& InPackageFileName, FWorldTileInfo& OutInfo);
	
public:
	/** Tile position in the world relative to parent */
	FIntPoint			Position; 
	/** Absolute tile position in the world. Calculated in runtime */
	FIntPoint			AbsolutePosition; 
	/** Tile bounding box  */
	FBox				Bounds;
	/** Tile assigned layer  */
	FWorldTileLayer		Layer;
	/** Whether this tile always loaded*/
	bool				bAlwaysLoaded;
	/** Parent tile package name */
	FString				ParentTilePackageName;
	/** LOD information */
	TArray<FWorldTileLODInfo> LODList;
	/** Sorting order */
	int32				ZOrder;
};


#endif	// __WorldCompositionUtility_h__