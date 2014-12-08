// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_PathingGrid.h"
#include "AI/Navigation/NavFilters/RecastFilter_UseDefaultArea.h"
#include "AI/Navigation/RecastNavMesh.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryGenerator_PathingGrid::UEnvQueryGenerator_PathingGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	GenerateAround = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	MaxPathDistance.Value = 100.0f;
	Density.Value = 10.0f;
	PathFromContext.Value = true;
}

void UEnvQueryGenerator_PathingGrid::GenerateItems(FEnvQueryInstance& QueryInstance) const
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

	const int32 ItemCount = FPlatformMath::TruncToInt((PathDistanceValue * 2.0f / DensityValue) + 1);
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

FText UEnvQueryGenerator_PathingGrid::GetDescriptionTitle() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("DescriptionTitle"), Super::GetDescriptionTitle());
	Args.Add(TEXT("DescribeContext"), UEnvQueryTypes::DescribeContext(GenerateAround));

	return FText::Format(LOCTEXT("DescriptionGenerateAroundContext", "{DescriptionTitle}: generate around {DescribeContext}"), Args);
};

FText UEnvQueryGenerator_PathingGrid::GetDescriptionDetails() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("MaxPathDistance"), FText::FromString(UEnvQueryTypes::DescribeFloatParam(MaxPathDistance)));
	Args.Add(TEXT("Density"), FText::FromString(UEnvQueryTypes::DescribeFloatParam(Density)));
	Args.Add(TEXT("PathFromContext"), FText::FromString(UEnvQueryTypes::DescribeBoolParam(PathFromContext)));

	return FText::Format(LOCTEXT("DescriptionDetailsPathingGrid", "max distance: {MaxPathDistance}, density: {Density}, path from context: {PathFromContext}"), Args);
}

#if WITH_RECAST
#define ENVQUERY_CLUSTER_SEARCH 0

void UEnvQueryGenerator_PathingGrid::FindNodeRefsInPathDistance(const ARecastNavMesh* NavMesh, const FVector& ContextLocation, float InMaxPathDistance, bool bPathFromContext, TArray<NavNodeRef>& NodeRefs, FBox& NodeRefsBounds) const
{
	FBox MyBounds(0);

#if ENVQUERY_CLUSTER_SEARCH
	const bool bUseBacktracking = !bPathFromContext;
	NavMesh->GetClustersWithinPathingDistance(ContextLocation, InMaxPathDistance, NodeRefs, bUseBacktracking);

	for (int32 RefIndex = 0; RefIndex < NodeRefs.Num(); RefIndex++)
	{
		FBox ClusterBounds;
		
		const bool bSuccess = NavMesh->GetClusterBounds(NodeRefs[RefIndex], ClusterBounds);
		if (bSuccess)
		{
			MyBounds += ClusterBounds;
		}
	}
#else
	TSharedPtr<FNavigationQueryFilter> NavFilterInstance = NavigationFilter != NULL
		? UNavigationQueryFilter::GetQueryFilter(NavMesh, NavigationFilter)->GetCopy()
		: NavMesh->GetDefaultQueryFilter()->GetCopy();

	NavFilterInstance->SetBacktrackingEnabled(!bPathFromContext);
	NavMesh->GetPolysWithinPathingDistance(ContextLocation, InMaxPathDistance, NodeRefs, NavFilterInstance);

	TArray<FVector> PolyVerts;
	for (int32 RefIndex = 0; RefIndex < NodeRefs.Num(); RefIndex++)
	{
		PolyVerts.Reset();
		
		const bool bSuccess = NavMesh->GetPolyVerts(NodeRefs[RefIndex], PolyVerts);
		if (bSuccess)
		{
			MyBounds += FBox(PolyVerts);
		}
	}
#endif

	NodeRefsBounds = MyBounds;
}

bool UEnvQueryGenerator_PathingGrid::IsNavLocationInPathDistance(const ARecastNavMesh* NavMesh,
		const FNavLocation& NavLocation, const TArray<NavNodeRef>& NodeRefs) const
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

#undef LOCTEXT_NAMESPACE