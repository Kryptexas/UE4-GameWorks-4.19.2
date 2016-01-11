// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LODCluster.h"

/*=============================================================================
	HierarchicalLOD.h: Hierarchical LOD definition.
=============================================================================*/

class ULevel;
class AActor;
class AHierarchicalLODVolume;
class UWorld;

/**
 *
 *	This is Hierarchical LOD builder
 *
 * This builds list of clusters and make sure it's sorted in the order of lower cost to high and merge clusters
 **/
struct UNREALED_API FHierarchicalLODBuilder
{
	FHierarchicalLODBuilder(UWorld* InWorld);

#if WITH_HOT_RELOAD_CTORS
	/** DO NOT USE. This constructor is for internal usage only for hot-reload purposes. */
	FHierarchicalLODBuilder();
#endif // WITH_HOT_RELOAD_CTORS
		
	/**
	* Build, Builds the clusters and spawn LODActors with their merged Static Meshes
	*/
	void Build();
	
	/**
	* PreviewBuild, Builds the clusters and spawns LODActors but without actually creating/merging new StaticMeshes
	*/
	void PreviewBuild();

private:
	/**
	* Builds the clusters (HLODs) for InLevel, and will create the new/merged StaticMeshes if bCreateMeshes is true
	*
	* @param InLevel - Level for which the HLODs are currently being build
	* @param bCreateMeshes - Whether or not to create/merge the StaticMeshes (false if builing preview only)
	*/
	void BuildClusters(ULevel* InLevel, const bool bCreateMeshes);

	/**
	* Initializes the clusters, creating one for each actor within the level eligble for HLOD-ing
	*
	* @param InLevel - Level for which the HLODs are currently being build
	* @param LODIdx - LOD index we are building
	* @param CullCost - Test variable for tweaking HighestCost
	*/
	void InitializeClusters(ULevel* InLevel, const int32 LODIdx, float CullCost);

	/**
	* Merges clusters and builds actors for resulting (valid) clusters
	*
	* @param InLevel - Level for which the HLODs are currently being build
	* @param LODIdx - LOD index we are building, used for determining which StaticMesh LOD to use
	* @param HighestCost - Allowed HighestCost for this LOD level
	* @param MinNumActors - Minimum number of actors for this LOD level
	* @param bCreateMeshes - Whether or not to create/merge the StaticMeshes (false if builing preview only)
	*/
	void MergeClustersAndBuildActors(ULevel* InLevel, const int32 LODIdx, float HighestCost, int32 MinNumActors, const bool bCreateMeshes);

	/**
	* Finds the minimal spanning tree MST for the clusters by sorting on their cost ( Lower == better )
	*/
	void FindMST();

	/* Retrieves HierarchicalLODVolumes and creates a cluster for each individual one
	*
	* @param InLevel - Level for which the HLODs are currently being build
	*/
	void HandleHLODVolumes(ULevel* InLevel);

	/**
	* Determine whether or not this actor is eligble for HLOD creation
	*
	* @param Actor - Actor to test
	* @return bool
	*/
	bool ShouldGenerateCluster(AActor* Actor);
	
	/** Array of LOD Clusters - this is only valid within scope since mem stack allocator */
	TArray<FLODCluster, TMemStackAllocator<>> Clusters;

	/** Owning world HLODs are created for */
	UWorld*	World;

	/** Array of LOD clusters created for the HierachicalLODVolumes found within the level */
	TMap<AHierarchicalLODVolume*, FLODCluster> HLODVolumeClusters;	
};
