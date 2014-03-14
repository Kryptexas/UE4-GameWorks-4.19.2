// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "VisualLog.h"

#define DEBUG_DRAW_OFFSET 0
#define PATH_OFFSET_KEEP_VISIBLE_POINTS 1

//----------------------------------------------------------------------//
// FNavPathType
//----------------------------------------------------------------------//
uint32 FNavPathType::NextUniqueId = 0;

//----------------------------------------------------------------------//
// FNavigationPath
//----------------------------------------------------------------------//
const FNavPathType FNavigationPath::Type;

FNavigationPath::FNavigationPath()
	: PathType(FNavigationPath::Type)
	, bUpToDate(true)
	, bIsReady(false)
	, bIsPartial(false)
	, bReachedSearchLimit(false)
{

}

FNavigationPath::FNavigationPath(const TArray<FVector>& Points, AActor* InBase)
	: PathType(FNavigationPath::Type)
	, bUpToDate(true)
	, bIsReady(true)
	, bIsPartial(false)
{
	Base = InBase;

	PathPoints.AddZeroed(Points.Num());
	for (int32 i = 0; i < Points.Num(); i++)
	{
		FBasedPosition BasedPoint(InBase, Points[i]);
		PathPoints[i] = FNavPathPoint(*BasedPoint);
	}
}

void FNavigationPath::DebugDraw(const ANavigationData* NavData, FColor PathColor, UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex) const
{
	static const FColor Grey(100,100,100);
	const int32 NumPathVerts = PathPoints.Num();

	UWorld* World = NavData->GetWorld();

	for (int32 VertIdx = 0; VertIdx < NumPathVerts-1; ++VertIdx)
	{
		// draw box at vert
		FVector const VertLoc = PathPoints[VertIdx].Location + NavigationDebugDrawing::PathOffeset;
		DrawDebugSolidBox(World, VertLoc, NavigationDebugDrawing::PathNodeBoxExtent, VertIdx < int32(NextPathPointIndex) ? Grey : PathColor, bPersistent);

		// draw line to next loc
		FVector const NextVertLoc = PathPoints[VertIdx+1].Location + NavigationDebugDrawing::PathOffeset;
		DrawDebugLine(World, VertLoc, NextVertLoc, VertIdx < int32(NextPathPointIndex)-1 ? Grey : PathColor, bPersistent
			, /*LifeTime*/-1.f, /*DepthPriority*/0
			, /*Thickness*/NavigationDebugDrawing::PathLineThickness);
	}

	// draw last vert
	if (NumPathVerts > 0)
	{
		DrawDebugBox(World, PathPoints[NumPathVerts-1].Location + NavigationDebugDrawing::PathOffeset, FVector(15.f), PathColor, bPersistent);
	}
}

bool FNavigationPath::ContainsNode(NavNodeRef NodeRef) const
{
	for (int32 i = 0; i < PathPoints.Num(); i++)
	{
		if (PathPoints[i].NodeRef == NodeRef)
		{
			return true;
		}
	}

	return false;
}

float FNavigationPath::GetLengthFromPosition(FVector SegmentStart, uint32 NextPathPointIndex) const
{
	if (NextPathPointIndex >= (uint32)PathPoints.Num())
	{
		return 0;
	}
	
	const uint32 PathPointsCount = PathPoints.Num();
	float PathDistance = 0.f;

	for (uint32 PathIndex = NextPathPointIndex; PathIndex < PathPointsCount; ++PathIndex)
	{
		const FVector SegmentEnd = PathPoints[PathIndex].Location;
		PathDistance += FVector::Dist(SegmentStart, SegmentEnd);
		SegmentStart = SegmentEnd;
	}

	return PathDistance;
}

bool FNavigationPath::ContainsSmartLink(class USmartNavLinkComponent* Link) const
{
	for (int32 i = 0; i < PathPoints.Num(); i++)
	{
		if (PathPoints[i].SmartLink.IsValid() &&
			PathPoints[i].SmartLink.Get() == Link)
		{
			return true;
		}
	}

	return false;
}

bool FNavigationPath::ContainsAnySmartLink() const
{
	for (int32 i = 0; i < PathPoints.Num(); i++)
	{
		if (PathPoints[i].SmartLink.IsValid())
		{
			return true;
		}
	}

	return false;
}

#if ENABLE_VISUAL_LOG

void FNavigationPath::DescribeSelfToVisLog(FVisLogEntry* Snapshot) const 
{
	const int32 NumPathVerts = PathPoints.Num();
	FVisLogEntry::FElementToDraw Element(FVisLogEntry::FElementToDraw::Path);
	Element.SetColor(FColor::Blue);
	Element.Points.Reserve(NumPathVerts);
	
	for (int32 VertIdx = 0; VertIdx < NumPathVerts; ++VertIdx)
	{
		Element.Points.Add(PathPoints[VertIdx].Location + NavigationDebugDrawing::PathOffeset);
	}

	Snapshot->ElementsToDraw.Add(Element);
}

#endif // ENABLE_VISUAL_LOG
//----------------------------------------------------------------------//
// FNavMeshPath
//----------------------------------------------------------------------//
const FNavPathType FNavMeshPath::Type;
	
FNavMeshPath::FNavMeshPath()
	: bCorridorEdgesGenerated(false)
	, bDynamic(false)
	, bStrigPulled(false)
	, bWantsStringPulling(true)
	, bWantsPathCorridor(false)
{
	PathType = FNavMeshPath::Type;
}

float FNavMeshPath::GetStringPulledLength(const int32 StartingPoint) const
{
	if (IsValid() == false || StartingPoint >= PathPoints.Num())
	{
		return 0.f;
	}

	float TotalLength = 0.f;
	const FNavPathPoint* PrevPoint = PathPoints.GetTypedData() + StartingPoint;
	const FNavPathPoint* PathPoint = PrevPoint + 1;

	for (int32 PathPointIndex = StartingPoint + 1; PathPointIndex < PathPoints.Num(); ++PathPointIndex, ++PathPoint, ++PrevPoint)
	{
		TotalLength += FVector::Dist(PrevPoint->Location, PathPoint->Location);
	}

	return TotalLength;
}

float FNavMeshPath::GetPathCorridorLength(const int32 StartingEdge) const
{
	if (bCorridorEdgesGenerated == false || StartingEdge >= PathCorridorEdges.Num())
	{
		return 0.f;
	}
	
	const FNavigationPortalEdge* PrevEdge = PathCorridorEdges.GetTypedData() + StartingEdge;
	const FNavigationPortalEdge* CorridorEdge = PrevEdge + 1;
	FVector PrevEdgeMiddle = PrevEdge->GetMiddlePoint();

	float TotalLength = StartingEdge == 0 ? FVector::Dist(PathPoints[0].Location, PrevEdgeMiddle)
		: FVector::Dist(PrevEdgeMiddle, PathCorridorEdges[StartingEdge - 1].GetMiddlePoint());

	for (int32 PathPolyIndex = StartingEdge + 1; PathPolyIndex < PathCorridorEdges.Num(); ++PathPolyIndex, ++PrevEdge, ++CorridorEdge)
	{
		const FVector CurrentEdgeMiddle = CorridorEdge->GetMiddlePoint();
		TotalLength += FVector::Dist(CurrentEdgeMiddle, PrevEdgeMiddle);
		PrevEdgeMiddle = CurrentEdgeMiddle;
	}
	// @todo add distance to last point here!
	return TotalLength;
}

const TArray<FNavigationPortalEdge>* FNavMeshPath::GeneratePathCorridorEdges()
{
#if WITH_RECAST
	// mz@todo this should be done in a bulk, should be refactored
	const int32 CorridorLenght = PathCorridor.Num();
	if (CorridorLenght != 0 && IsInGameThread() && Owner.IsValid())
	{
		const ARecastNavMesh* MyOwner = Cast<ARecastNavMesh>(Owner.Get());
		MyOwner->GetEdgesForPathCorridor(&PathCorridor, &PathCorridorEdges);
		bCorridorEdgesGenerated = PathCorridorEdges.Num() > 0;
	}
#endif // WITH_RECAST
	return &PathCorridorEdges;
}

#if DEBUG_DRAW_OFFSET
	UWorld* GInternalDebugWorld_ = NULL;
#endif

namespace
{
	FORCEINLINE bool CalculateSegmentIntersection2D(const FVector& SegmentStartA, const FVector& SegmentEndA, const FVector& SegmentStartB, const FVector& SegmentEndB, FVector& IntersectionPoint)
	{
		const FVector VectorA = SegmentEndA - SegmentStartA;
		const FVector VectorB = SegmentEndB - SegmentStartB;

		const float S = (-VectorA.Y * (SegmentStartA.X - SegmentStartB.X) + VectorA.X * (SegmentStartA.Y - SegmentStartB.Y)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);
		const float T = ( VectorB.X * (SegmentStartA.Y - SegmentStartB.Y) - VectorB.Y * (SegmentStartA.X - SegmentStartB.X)) / (-VectorB.X * VectorA.Y + VectorA.X * VectorB.Y);

		const bool bIntersects = S >= 0 && S <= 1 && T >= 0 && T <= 1;

		if (bIntersects)
		{
			IntersectionPoint.X = SegmentStartA.X + (T * VectorA.X);
			IntersectionPoint.Y = SegmentStartA.Y + (T * VectorA.Y);
			IntersectionPoint.Z = SegmentStartA.Z + (T * VectorA.Z);
		}

		return bIntersects;
	}

	struct FPathPointInfo
	{
		FPathPointInfo() 
		{

		}
		FPathPointInfo( const FNavPathPoint& InPoint, const FVector& InEdgePt0, const FVector& InEdgePt1) 
			: Point(InPoint)
			, EdgePt0(InEdgePt0)
			, EdgePt1(InEdgePt1) 
		{ 
			/** Empty */ 
		}

		FNavPathPoint Point;
		FVector EdgePt0;
		FVector EdgePt1;
	};

	
	FORCEINLINE bool CheckVisibility(const FPathPointInfo* StartPoint, const FPathPointInfo* EndPoint,  TArray<FNavigationPortalEdge>& PathCorridorEdges, float OffsetDistannce, FPathPointInfo* LastVisiblePoint)
	{
		FVector IntersectionPoint = FVector::ZeroVector;
		FVector StartTrace = StartPoint->Point.Location;
		FVector EndTrace = EndPoint->Point.Location;

		// find closest edge to StartPoint
		float BestDistance = FLT_MAX;
		FNavigationPortalEdge* CurrentEdge = NULL;

		float BestEndPointDistance = FLT_MAX;
		FNavigationPortalEdge* EndPointEdge = NULL;
		for (int32 EdgeIndex =0; EdgeIndex < PathCorridorEdges.Num(); ++EdgeIndex)
		{
			float DistToEdge = FLT_MAX;
			FNavigationPortalEdge* Edge = &PathCorridorEdges[EdgeIndex];
			if (BestDistance > FMath::Square(KINDA_SMALL_NUMBER))
			{
				DistToEdge= FMath::PointDistToSegmentSquared(StartTrace, Edge->Left, Edge->Right);
				if (DistToEdge < BestDistance)
				{
					BestDistance = DistToEdge;
					CurrentEdge = Edge;
#if DEBUG_DRAW_OFFSET
					DrawDebugLine( GInternalDebugWorld_, Edge->Left, Edge->Right, FColor::White, true );
#endif
				}
			}

			if (BestEndPointDistance > FMath::Square(KINDA_SMALL_NUMBER))
			{
				DistToEdge= FMath::PointDistToSegmentSquared(EndTrace, Edge->Left, Edge->Right);
				if (DistToEdge < BestEndPointDistance)
				{
					BestEndPointDistance = DistToEdge;
					EndPointEdge = Edge;
				}
			}
		}

		if (CurrentEdge == NULL || EndPointEdge == NULL )
		{
			LastVisiblePoint->Point.Location = FVector::ZeroVector;
			return false;
		}


		if (BestDistance <= FMath::Square(KINDA_SMALL_NUMBER))
		{
			CurrentEdge++;
		}

		if (CurrentEdge == EndPointEdge)
		{
			return true;
		}

		const FVector RayNormal = (StartTrace-EndTrace) .SafeNormal() * OffsetDistannce;
		StartTrace = StartTrace + RayNormal;
		EndTrace = EndTrace - RayNormal;

		bool bIsVisible = true;
#if DEBUG_DRAW_OFFSET
		DrawDebugLine( GInternalDebugWorld_, StartTrace, EndTrace, FColor::Yellow, true );
#endif
		const FNavigationPortalEdge* LaseEdge = &PathCorridorEdges[PathCorridorEdges.Num()-1];
		while (CurrentEdge <= EndPointEdge)
		{
			FVector Left = CurrentEdge->Left;
			FVector Right = CurrentEdge->Right;

#if DEBUG_DRAW_OFFSET
			DrawDebugLine( GInternalDebugWorld_, Left, Right, FColor::White, true );
#endif
			bool bIntersected = CalculateSegmentIntersection2D( Left, Right, StartTrace, EndTrace, IntersectionPoint );
			if ( !bIntersected)
			{
				const float EdgeHalfLength = (CurrentEdge->Left - CurrentEdge->Right).Size() * 0.5;
				const float Distance = FMath::Min(OffsetDistannce, EdgeHalfLength) *  0.1;
				Left = CurrentEdge->Left + Distance * (CurrentEdge->Right - CurrentEdge->Left).SafeNormal();
				Right = CurrentEdge->Right + Distance * (CurrentEdge->Left - CurrentEdge->Right).SafeNormal();
				FVector ClosestPointOnRay, ClosestPointOnEdge;
				FMath::SegmentDistToSegment(StartTrace, EndTrace, Right, Left, ClosestPointOnRay, ClosestPointOnEdge);
#if DEBUG_DRAW_OFFSET
				DrawDebugSphere( GInternalDebugWorld_, ClosestPointOnEdge, 10, 8, FColor::Red, true );
#endif
				LastVisiblePoint->Point.Location = ClosestPointOnEdge;
				LastVisiblePoint->EdgePt0= CurrentEdge->Left ;
				LastVisiblePoint->EdgePt1= CurrentEdge->Right ;
				return false;
			}
#if DEBUG_DRAW_OFFSET
			DrawDebugSphere( GInternalDebugWorld_, IntersectionPoint, 8, 8, FColor::White, true );
#endif
			CurrentEdge++;
			bIsVisible = true;
		}

		return bIsVisible;
	}
}

void FNavMeshPath::OffsetFromCorners(float Distance)
{
	SCOPE_CYCLE_COUNTER(STAT_Navigation_OffsetFromCorners);

	const ARecastNavMesh* MyOwner = Cast<ARecastNavMesh>(Owner.Get());
#if DEBUG_DRAW_OFFSET
	GInternalDebugWorld_ = MyOwner->GetWorld();
	FlushDebugStrings(GInternalDebugWorld_);
	FlushPersistentDebugLines(GInternalDebugWorld_);
#endif

	if (bCorridorEdgesGenerated == false)
	{
		GeneratePathCorridorEdges(); 
	}
	const float DistanceSq = Distance * Distance;
	int32 CurrentEdge = 0;
	bool bNeedToCopyResults = false;
	int32 SingleNodePassCount = 0;

	FNavPathPoint* PathPoint = PathPoints.GetTypedData();
	// it's possible we'll be inserting points into the path, so we need to buffer the result
	TArray<FPathPointInfo> FirstPassPoints;
	FirstPassPoints.Reserve(PathPoints.Num() * 2);
	FirstPassPoints.Add(FPathPointInfo(*PathPoint, FVector::ZeroVector, FVector::ZeroVector));
	++PathPoint;

	// for every point on path find a related corridor edge
	for (int32 PathNodeIndex = 1; PathNodeIndex < PathPoints.Num()-1 && CurrentEdge < PathCorridorEdges.Num();)
	{
		if (FNavMeshNodeFlags(PathPoint->Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION)
		{
			// put both ends
			FirstPassPoints.Add(FPathPointInfo(*PathPoint, FVector(0), FVector(0)));
			FirstPassPoints.Add(FPathPointInfo(*(PathPoint+1), FVector(0), FVector(0)));
			PathNodeIndex += 2;
			PathPoint += 2;
			continue;
		}

		int32 CloserPoint = -1;
		const FNavigationPortalEdge* Edge = &PathCorridorEdges[CurrentEdge];
		for (int32 EdgeIndex = CurrentEdge; EdgeIndex < PathCorridorEdges.Num(); ++Edge, ++EdgeIndex)
		{
			const float DistToSequence = FMath::PointDistToSegmentSquared(PathPoint->Location, Edge->Left, Edge->Right);
			if (DistToSequence <= FMath::Square(KINDA_SMALL_NUMBER))
			{
				const float LeftDistanceSq = FVector::DistSquared(PathPoint->Location, Edge->Left);
				const float RightDistanceSq = FVector::DistSquared(PathPoint->Location, Edge->Right);
				if (LeftDistanceSq > DistanceSq && RightDistanceSq > DistanceSq)
				{
					++CurrentEdge;
				}
				else
				{
					CloserPoint = LeftDistanceSq < RightDistanceSq ? 0 : 1;
					CurrentEdge = EdgeIndex;
				}
				break;
			}
		}

		if (CloserPoint >= 0)
		{
			bNeedToCopyResults = true;

			Edge = &PathCorridorEdges[CurrentEdge];
			const float ActualOffset = FPlatformMath::Min(Edge->GetLength()/2, Distance);

			FNavPathPoint NewPathPoint = *PathPoint;
			// apply offset 

			const FVector EdgePt0 = Edge->GetPoint(CloserPoint);
			const FVector EdgePt1 = Edge->GetPoint((CloserPoint+1)%2);
			const FVector EdgeDir = EdgePt1 - EdgePt0;
			const FVector EdgeOffset = EdgeDir.SafeNormal() * ActualOffset;
			NewPathPoint = EdgePt0 + EdgeOffset;
			// update NodeRef (could be different if this is n-th pass on the same PathPoint
			NewPathPoint.NodeRef = Edge->ToRef;
			NewPathPoint.Flags = PathPoint->Flags;
			FirstPassPoints.Add(FPathPointInfo(NewPathPoint, EdgePt0, EdgePt1));

			// if we've found a matching edge it's possible there's also another one there using the same edge. 
			// that's why we need to repeat the process with the same path point and next edge
			++CurrentEdge;

			// we need to know if we did more than one iteration on a given point
			// if so then we should not add that point in following "else" statement
			++SingleNodePassCount;
		}
		else
		{
			if (SingleNodePassCount == 0)
			{
				// store unchanged
				FirstPassPoints.Add(FPathPointInfo(*PathPoint, FVector(0), FVector(0)));
			}
			else
			{
				SingleNodePassCount = 0;
			}

			++PathNodeIndex;
			++PathPoint;
		}
	}

	if (bNeedToCopyResults)
	{
		if (FirstPassPoints.Num() < 3 || !MyOwner->bUseBetterOffsetsFromCorners)
		{
			PathPoints.Reset();
			for (int32 Index=0; Index < FirstPassPoints.Num(); ++Index)
			{
				PathPoints.Add(FirstPassPoints[Index].Point);
			}
			return;
		}

		TArray<FNavPathPoint> DestinationPathPoints;
		DestinationPathPoints.Reserve(FirstPassPoints.Num() + 2);

		// don't forget the last point
		FirstPassPoints.Add(FPathPointInfo(PathPoints[PathPoints.Num()-1], FVector::ZeroVector, FVector::ZeroVector));

		int32 StartPointIndex = 0;
		int32 LastVisiblePointIndex = 0;
		int32 TestedPointIndex = 1;
		int32 LastPointIndex = FirstPassPoints.Num()-1;

		bool bVisible = true; 
		bool bStop = false; 

		while (!bStop) 
		{ 
			if (StartPointIndex == TestedPointIndex)
			{
				DestinationPathPoints.Reset();
				bStop = true;
				break;
			}
			if (FNavMeshNodeFlags(FirstPassPoints[StartPointIndex].Point.Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION) 
			{
				DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
				DestinationPathPoints.Add( FirstPassPoints[StartPointIndex+1].Point );

				StartPointIndex++;
				LastVisiblePointIndex = StartPointIndex ;
				TestedPointIndex = LastVisiblePointIndex + 1;

				if (TestedPointIndex > LastPointIndex) 
				{ 
					DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
					DestinationPathPoints.Add( FirstPassPoints[LastPointIndex].Point );
					bStop = true; 
					break; 
				} 
				continue;
			}
			else if(FNavMeshNodeFlags(FirstPassPoints[LastVisiblePointIndex].Point.Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION)
			{
				bVisible = false;
			}
			else if ( FNavMeshNodeFlags(FirstPassPoints[StartPointIndex].Point.Flags).Area != FNavMeshNodeFlags( FirstPassPoints[LastVisiblePointIndex].Point.Flags).Area )
			{
				bVisible = false;
			}
			else
			{
				FPathPointInfo LastVisiblePoint;
				bVisible = CheckVisibility( &FirstPassPoints[StartPointIndex], &FirstPassPoints[TestedPointIndex], PathCorridorEdges, Distance, &LastVisiblePoint );
				if (!bVisible)
				{
					if (LastVisiblePoint.Point.Location.IsNearlyZero())
					{
						DestinationPathPoints.Reset();
						break;
					}
					else if (StartPointIndex == LastVisiblePointIndex)
					{
						/** add new point only if we don't see our next location otherwise use last visible point*/
						LastVisiblePoint.Point.Flags = FirstPassPoints[LastVisiblePointIndex].Point.Flags;
						LastVisiblePointIndex = FirstPassPoints.Insert( LastVisiblePoint, StartPointIndex+1 );
						LastPointIndex = FirstPassPoints.Num()-1;
					}
				}
			}

			if (bVisible) 
			{ 
#if PATH_OFFSET_KEEP_VISIBLE_POINTS
				DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
				LastVisiblePointIndex = TestedPointIndex;
				StartPointIndex = LastVisiblePointIndex;
				TestedPointIndex++;
#else
				LastVisiblePointIndex = TestedPointIndex;
				TestedPointIndex++;
#endif
				if (TestedPointIndex > LastPointIndex) 
				{ 
					DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
					DestinationPathPoints.Add( FirstPassPoints[LastPointIndex].Point );
					bStop = true; 
					break; 
				} 
				continue; 
			} 
			else
			{ 
				DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
				StartPointIndex = LastVisiblePointIndex;
				TestedPointIndex = LastVisiblePointIndex + 1;

				if (TestedPointIndex > LastPointIndex) 
				{ 
					DestinationPathPoints.Add( FirstPassPoints[StartPointIndex].Point );
					DestinationPathPoints.Add( FirstPassPoints[LastPointIndex].Point );
					bStop = true; 
					break; 
				} 
			} 
		} 

		if (DestinationPathPoints.Num())
		{
			PathPoints = DestinationPathPoints;
		}
	}
}

bool FNavMeshPath::IsPathSegmentANavLink(const int32 PathSegmentStartIndex) const
{
	return PathPoints.IsValidIndex(PathSegmentStartIndex)
		&& FNavMeshNodeFlags(PathPoints[PathSegmentStartIndex].Flags).PathFlags & RECAST_STRAIGHTPATH_OFFMESH_CONNECTION;
}

void FNavMeshPath::DebugDraw(const ANavigationData* NavData, FColor PathColor, UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex) const
{
	Super::DebugDraw(NavData, PathColor, Canvas, bPersistent, NextPathPointIndex);

#if WITH_RECAST
	const ARecastNavMesh* RecastNavMesh = Cast<const ARecastNavMesh>(NavData);

	// tmp hack
	const FNavigationPortalEdge* Edge = (const_cast<FNavMeshPath*>(this))->GetPathCorridorEdges()->GetTypedData();
	//const FNavigationPortalEdge* Edge = PathCorridorEdges.GetTypedData();
	const int32 CorridorEdgesCount = PathCorridorEdges.Num();

	for (int32 i = 0; i < CorridorEdgesCount; ++i, ++Edge)
	{
		DrawDebugLine(NavData->GetWorld(), Edge->Left+NavigationDebugDrawing::PathOffeset, Edge->Right+NavigationDebugDrawing::PathOffeset
			, FColor::Blue, bPersistent, /*LifeTime*/-1.f, /*DepthPriority*/0
			, /*Thickness*/NavigationDebugDrawing::PathLineThickness);
	}

	if (Canvas && RecastNavMesh && RecastNavMesh->bDrawLabelsOnPathNodes)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		for (int32 VertIdx = 0; VertIdx < PathPoints.Num(); ++VertIdx)
		{
			// draw box at vert
			FVector const VertLoc = PathPoints[VertIdx].Location 
				+ FVector(0, 0, NavigationDebugDrawing::PathNodeBoxExtent.Z*2)
				+ NavigationDebugDrawing::PathOffeset;
			const FVector ScreenLocation = Canvas->Project(VertLoc);

			FNavMeshNodeFlags NodeFlags(PathPoints[VertIdx].Flags);
			const UClass* NavAreaClass = RecastNavMesh->GetAreaClass(NodeFlags.Area);

			Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d: %s"), VertIdx, *GetNameSafe(NavAreaClass)), ScreenLocation.X, ScreenLocation.Y );
		}
	}
#endif // WITH_RECAST
}

bool FNavMeshPath::ContainsWithSameEnd(const FNavMeshPath* Other) const
{
	if (PathCorridor.Num() < Other->PathCorridor.Num())
	{
		return false;
	}

	const NavNodeRef* ThisPathNode = &PathCorridor[PathCorridor.Num()-1];
	const NavNodeRef* OtherPathNode = &Other->PathCorridor[Other->PathCorridor.Num()-1];
	bool bAreTheSame = true;

	for (int32 NodeIndex = Other->PathCorridor.Num() - 1; NodeIndex >= 0 && bAreTheSame; --NodeIndex, --ThisPathNode, --OtherPathNode)
	{
		bAreTheSame = *ThisPathNode == *OtherPathNode;
	}	

	return bAreTheSame;
}

void FNavMeshPath::PopCorridorEdge()
{
	if (PathCorridorEdges.Num() > 0)
	{
		PathCorridorEdges.RemoveAtSwap(PathCorridorEdges.Num()-1, 1, false);
	}
}

