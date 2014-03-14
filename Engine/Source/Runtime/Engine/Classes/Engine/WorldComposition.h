// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "WorldComposition.generated.h"


/**
 * Helper structure which holds information about level package which participates in world composition
 */
struct FWorldCompositionTile
{
	// Long package name
	FName					PackageName;
	// Found LOD levels since last rescan
	TArray<FName>			LODPackageNames;
	// Tile information
	FWorldTileInfo			Info;

	friend FArchive& operator<<( FArchive& Ar, FWorldCompositionTile& D )
	{
		Ar << D.PackageName << D.Info << D.LODPackageNames;
		return Ar;
	}
						
	/** Matcher */
	struct FPackageNameMatcher
	{
		FPackageNameMatcher( const FName& InPackageName )
			: PackageName( InPackageName )
		{
		}

		bool Matches( const FWorldCompositionTile& Candidate ) const
		{
			return Candidate.PackageName == PackageName;
		}

		const FName& PackageName;
	};
};

/**
 * Helper structure which holds results of distance queries to a world composition
 */
struct FDistanceVisibleLevel
{
	ULevelStreaming*	StreamingLevel;
	int32				LODIndex; 
};

/**
 * WorldComposition represents world structure:
 *	- Holds list of all level packages participating in this world and theirs base parameters (bounding boxes, offset from origin)
 *	- Holds list of streaming level objects to stream in and out based on distance from current view point
 *  - Handles properly levels repositioning during level loading and saving
 */
UCLASS(config=Engine)
class ENGINE_API UWorldComposition : public UObject
{
	GENERATED_UCLASS_BODY()

	typedef TArray<FWorldCompositionTile> TilesList;

	/** Adds or removes level streaming objects to world based on distance settings from current view */
	void UpdateStreamingState (FSceneViewFamily* InViewFamily = NULL) const;
	
	/** Adds or removes level streaming objects to world based on distance settings from current view point */
	void UpdateStreamingState(const FVector& InLocation) const;

	/** Returns currently visible levels for distance based streaming */
	void GetVisibleLevels(const FVector& InLocation, TArray<FDistanceVisibleLevel>& OutLevels) const;

	/** Opens world composition from specified folder (long PackageName)*/
	bool OpenWorldRoot(const FString& InPathToRoot);
	
	/** @returns Currently opened world composition root folder (long PackageName)*/
	FString GetWorldRoot() const;
	
	/** Handles level OnPostLoad event*/
	static void OnLevelPostLoad(ULevel* InLevel);

	/** Handles level just before it going to be saved to disk */
	void OnLevelPreSave(ULevel* InLevel);
	
	/** Handles level just after it was saved to disk */
	void OnLevelPostSave(ULevel* InLevel);
	
	/** Handles level is being added to world */
	void OnLevelAddedToWorld(ULevel* InLevel);

	/** Handles level is being removed from the world */
	void OnLevelRemovedFromWorld(ULevel* InLevel);

	/** @returns Level offset from zero origin, with respect to parent levels */
	FIntPoint GetLevelOffset(ULevel* InLevel) const;

	/** @returns Level bounding box in current shifted space */
	FBox GetLevelBounds(ULevel* InLevel) const;

	/** @returns Whether specified level is marked as always loaded */
	bool IsLevelAlwaysLoaded(ULevel* InLevel) const;

#if WITH_EDITOR
	/** @returns FWorldTileInfo associated with specified package */
	FWorldTileInfo GetTileInfo(const FName& InPackageName) const;
	
	/** Notification from World browser about changes in tile info structure */
	void OnTileInfoUpdated(const FName& InPackageName, const FWorldTileInfo& InInfo);

	/** Notification from World browser that tile info is no longer valid and has to be read from package file  */
	void DiscardTileInfo(const FName& InPackageName);

	/** @returns Tiles list in a world composition */
	TilesList& GetTilesList();

#endif //WITH_EDITOR

private:
	// UObject interface
	/** Handles WorldComposition duplication for PIE */
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
	// UObject interface

	/** Scans world root folder for relevant packages and initializes world composition structures */
	void Rescan();

	/** Loads levels which are marked as always loaded to our world */
	void StreaminAlwaysLoadedLevels();

	/** Resets world composition structures */
	void Reset();
	
	/** @returns  Streaming level object for corresponding FWorldCompositionTile */
	ULevelStreaming* CreateStreamingLevel(FWorldCompositionTile& Info) const;

	/** Calculates tiles absolute positions based on relative positions */
	void CaclulateTilesAbsolutePositions();
	
	/** Fixups internal structures for PIE mode */
	void FixupForPIE(int32 PIEInstanceID);

	/**
	 * Finds tile by package name 
	 * @return index in Tiles array, INDEX_NONE otherwise 
	 */
	int32 FindTileByName(const FName& InPackageName) const;

public:
#if WITH_EDITOR
	// Last location from where streaming state was updated
	mutable FVector				LastViewLocation;
#endif //WITH_EDITOR

private:
	// Path to current world composition (long PackageName)
	FString						WorldRoot;
	
	// List of all packages participating in world composition
	TilesList					Tiles;
	// List of preallocated streaming level objects to stream in and out levels based on distance 
	UPROPERTY()
	TArray<ULevelStreaming*>	TilesStreaming;
};