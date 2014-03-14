// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator_PathingGrid::UEnvQueryGenerator_PathingGrid(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_PathingGrid::GenerateItems);

	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	MaxPathDistance.Value = 100.0f;
	Density.Value = 10.0f;
	PathFromContext.Value = true;
}

void UEnvQueryGenerator_PathingGrid::GenerateItems(struct FEnvQueryInstance& QueryInstance)
{
#if WITH_RECAST
	const ARecastNavMesh* NavMesh = FEQSHelpers::FindNavMeshForQuery(QueryInstance);
	if (NavMesh == NULL) 
	{
		return;
	}

	float PathDistanceValue = 0.0f;
	float DensityValue = 0.0f;
	bool bFromContextValue = true;
	if (!QueryInstance.GetParamValue(MaxPathDistance, PathDistanceValue, TEXT("MaxPathDistance")) ||
		!QueryInstance.GetParamValue(Density, DensityValue, TEXT("Density")) ||
		!QueryInstance.GetParamValue(PathFromContext, bFromContextValue, TEXT("PathFromContext")))
	{
		return;
	}

	const int32 ItemCount = FPlatformMath::Trunc((PathDistanceValue * 2.0f / DensityValue) + 1);
	const int32 ItemCountHalf = ItemCount / 2;

	TArray<FVector> ContextLocations;
	QueryInstance.PrepareContext(GenerateAround, ContextLocations);
	QueryInstance.ReserveItemData(ItemCountHalf * ItemCountHalf * ContextLocations.Num());

	TArray<NavNodeRef> NavNodeRefs;
	NavMesh->BeginBatchQuery();

	int32 DataOffset = 0;
	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		// find all node refs in pathing distance
		FBox AllowedBounds;
		NavNodeRefs.Reset();
		FindNodeRefsInPathDistance(NavMesh, ContextLocations[ContextIndex], PathDistanceValue, bFromContextValue, NavNodeRefs, AllowedBounds);

		// cast 2D grid on generated node refs
		for (int32 IndexX = 0; IndexX < ItemCount; ++IndexX)
		{
			for (int32 IndexY = 0; IndexY < ItemCount; ++IndexY)
			{
				const FVector TestPoint = ContextLocations[ContextIndex] - FVector(DensityValue * (IndexX - ItemCountHalf), DensityValue * (IndexY - ItemCountHalf), 0);
				if (!AllowedBounds.IsInsideXY(TestPoint))
				{
					continue;
				}

				// trace line on navmesh, and process all hits with collected node refs
				TArray<FNavLocation> Hits;
				NavMesh->ProjectPointMulti(TestPoint, Hits, FVector::ZeroVector, AllowedBounds.Min.Z, AllowedBounds.Max.Z);

				for (int32 HitIndex = 0; HitIndex < Hits.Num(); HitIndex++)
				{
					if (IsNavLocationInPathDistance(NavMesh, Hits[HitIndex], NavNodeRefs))
					{
						// store generated point
						QueryInstance.AddItemData<UEnvQueryItemType_Point>(Hits[HitIndex].Location);
					}
				}
			}
		}
	}

	NavMesh->FinishBatchQuery();
#endif // WITH_RECAST
}

FString UEnvQueryGenerator_PathingGrid::GetDescriptionTitle() const
{
	return FString::Printf(TEXT("%s: generate around %s"),
		*Super::GetDescriptionTitle(), *UEnvQueryTypes::DescribeContext(GenerateAround));
};

FString UEnvQueryGenerator_PathingGrid::GetDescriptionDetails() const
{
	return FString::Printf(TEXT("max distance: %s, density: %s, path from context: %s"),
		*UEnvQueryTypes::DescribeFloatParam(MaxPathDistance),
		*UEnvQueryTypes::DescribeFloatParam(Density),
		*UEnvQueryTypes::DescribeBoolParam(PathFromContext));
}

#if WITH_RECAST
#define ENVQUERY_CLUSTER_SEARCH 0

void UEnvQueryGenerator_PathingGrid::FindNodeRefsInPathDistance(const class ARecastNavMesh* NavMesh, const FVector& ContextLocation, float InMaxPathDistance, bool bPathFromContext, TArray<NavNodeRef>& NodeRefs, FBox& NodeRefsBounds) const
{
	FBox MyBounds(0);

#if ENVQUERY_CLUSTER_SEARCH
	const bool bUseBacktracking = !bPathFromContext;
	NavMesh->GetClustersWithinPathingDistance(ContextLocation, InMaxPathDistance, NodeRefs, bUseBacktracking);

	for (int32 i = 0; i < NodeRefs.Num(); i++)
	{
		FBox ClusterBounds;
		
		const bool bSuccess = NavMesh->GetClusterBounds(NodeRefs[i], ClusterBounds);
		if (bSuccess)
		{
			MyBounds += ClusterBounds;
		}
	}
#else
	if (!bPathFromContext)
	{
		TSharedPtr<FNavigationQueryFilter> BacktrackingFilter = NavMesh->GetDefaultQueryFilter()->GetCopy();
		BacktrackingFilter->SetBacktrackingEnabled(true);

		NavMesh->GetPolysWithinPathingDistance(ContextLocation, InMaxPathDistance, NodeRefs, BacktrackingFilter);
	}
	else
	{
		NavMesh->GetPolysWithinPathingDistance(ContextLocation, InMaxPathDistance, NodeRefs);
	}

	TArray<FVector> PolyVerts;
	for (int32 i = 0; i < NodeRefs.Num(); i++)
	{
		PolyVerts.Reset();
		
		const bool bSuccess = NavMesh->GetPolyVerts(NodeRefs[i], PolyVerts);
		if (bSuccess)
		{
			MyBounds += FBox(PolyVerts);
		}
	}
#endif

	NodeRefsBounds = MyBounds;
}

bool UEnvQueryGenerator_PathingGrid::IsNavLocationInPathDistance(const class ARecastNavMesh* NavMesh,
		const struct FNavLocation& NavLocation, const TArray<NavNodeRef>& NodeRefs) const
{
#if ENVQUERY_CLUSTER_SEARCH
	const NavNodeRef ClusterRef = NavMesh->GetClusterRef(NavLocation.NodeRef);
	return NodeRefs.Contains(ClusterRef);
#else
	return NodeRefs.Contains(NavLocation.NodeRef);
#endif
}

#undef ENVQUERY_CLUSTER_SEARCH

#endif // WITH_RECAST