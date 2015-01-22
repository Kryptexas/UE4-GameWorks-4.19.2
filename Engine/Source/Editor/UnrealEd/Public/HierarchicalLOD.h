// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/*=============================================================================
	HierarchicalLOD.h: Hierarchical LOD definition.
=============================================================================*/


// implementation 
//  http://deim.urv.cat/~rivi/pub/3d/icra04b.pdf

struct FLODCluster
{
	FLODCluster(const FLODCluster& Other);
	FLODCluster(class AActor* Actor1, class AActor* Actor2);

	FLODCluster operator+( const FLODCluster& Other ) const;
	FLODCluster operator+=( const FLODCluster& Other );
	FLODCluster operator-(const FLODCluster& Other) const;
	FLODCluster operator-=(const FLODCluster& Other);
	FLODCluster& operator=(const FLODCluster & Other);
	// invalidate this cluster - happens when this merged to another cluster
	void Invalidate();
	bool IsValid() const;

	const float GetCost() const;
	bool Contains(FLODCluster& Other) const;
	FString ToString() const;
	
	TArray<class AActor*>	Actors;
	FBox					Bound;
	float					FillingFactor;

	// we don't delete the invalid entry, but mark as invalid when it's not any more valid
	// it's initialized to be valid
	bool bValid;

	// if criteria matches, build new LODActor and replace current Actors with that. We don't need 
	// this clears previous actors and sets to this new actor
	// this is required when new LOD is created from these actors, this will be replaced
	// to save memory and to reduce memory increase during this process, we discard previous actors and replace with this actor
	void BuildActor(class UWorld* InWorld, class ULevel* InLevel, const int32 LODIdx);
	// Merge two clusters
	void MergeClusters(const FLODCluster& Other);
	void SubtractCluster(const FLODCluster& Other);
	FBox AddActor(class AActor* NewActor);
};

struct UNREALED_API FHierarchicalLODBuilder
{
	FHierarchicalLODBuilder(class UWorld* InWorld);

	// build hierarchical cluster
	void Build();

private:
	// data structure
	TArray<FLODCluster> Clusters;
	class UWorld*		World;

	// for now it only builds per level, it turns to one actor at the end
	void BuildClusters(class ULevel* InLevel);

	// initialize Clusters variable - each Actor becomes Cluster
	void InitializeClusters(class ULevel* InLevel, const int32 LODIdx);

	// Refresh Cluster Distance for ClusterId
	//void RefreshClusterDistance(int32 ClusterId, const int32 TotalNumClusters, float** InDistanceCacheMatrix);

	void MergeClusters(class ULevel* InLevel, const int32 LODIdx);

	void FindMST(TArray<FLODCluster>& OutClusters);
};
