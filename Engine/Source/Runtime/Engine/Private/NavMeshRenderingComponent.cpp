// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NavMeshRenderingComponent.cpp: A component that renders a nav mesh.
 =============================================================================*/

#include "EnginePrivate.h"
#include "DebugRenderSceneProxy.h"
#include "NavigationOctree.h"
#include "AI/Navigation/RecastHelpers.h"
#include "AI/Navigation/RecastNavMeshGenerator.h"

#include "NavMeshRenderingHelpers.h"

UNavMeshRenderingComponent::UNavMeshRenderingComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	AlwaysLoadOnClient = false;
	AlwaysLoadOnServer = false;
	bSelectable = false;
}

FPrimitiveSceneProxy* UNavMeshRenderingComponent::CreateSceneProxy()
{
#if WITH_RECAST && WITH_EDITOR
	FPrimitiveSceneProxy* SceneProxy = NULL;
	if (IsVisible())
	{
		if (!ProxyData.IsValid())
		{
			ProxyData = MakeShareable(new FNavMeshSceneProxyData());
		}
		ProxyData->bNeedsNewData = true;
		SceneProxy = new FRecastRenderingSceneProxy(this, ProxyData);
	}
	return SceneProxy;
#else
	return NULL;
#endif
}

void UNavMeshRenderingComponent::GatherDataForProxy()
{
	if (ProxyData.IsValid())
	{
#if WITH_RECAST
		FScopeLock ScopeLock(&ProxyData.Get()->CriticalSection);
		GatherData( ProxyData.Get() );

		ProxyData->bNeedsNewData = false;
#endif
	}
}

void UNavMeshRenderingComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

#if WITH_RECAST && WITH_EDITOR
	if (SceneProxy != NULL)
	{
		static_cast<FRecastRenderingSceneProxy*>(SceneProxy)->RegisterDebugDrawDelgate();
	}
#endif
}

void UNavMeshRenderingComponent::DestroyRenderState_Concurrent()
{
#if WITH_RECAST && WITH_EDITOR
	if (SceneProxy != NULL)
	{
		static_cast<FRecastRenderingSceneProxy*>(SceneProxy)->UnregisterDebugDrawDelgate();
	}
#endif

	Super::DestroyRenderState_Concurrent();
}

FBoxSphereBounds UNavMeshRenderingComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox BoundingBox(0);
#if WITH_RECAST
	ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(GetOwner());
	if (NavMesh)
	{
		BoundingBox = NavMesh->GetNavMeshBounds();
	}
#endif
	return FBoxSphereBounds(BoundingBox);
}

void UNavMeshRenderingComponent::GatherData(struct FNavMeshSceneProxyData* CurrentData) const
{
#if WITH_RECAST
	const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(GetOwner());

	if (CurrentData && NavMesh && NavMesh->bEnableDrawing && (!NavMesh->bShowOnlyDefaultAgent || NavMesh->IsSupportingDefaultAgent()))
	{
		FHitProxyId HitProxyId = FHitProxyId();
		CurrentData->Reset();
		CurrentData->bEnableDrawing = NavMesh->bEnableDrawing;
		CurrentData->bShowOnlyDefaultAgent = NavMesh->bShowOnlyDefaultAgent;
		CurrentData->bSupportsDefaultAgent = NavMesh->IsSupportingDefaultAgent();
		CurrentData->bDrawPathCollidingGeometry = NavMesh->bDrawPathCollidingGeometry;

		CurrentData->NavMeshDrawOffset.Z = NavMesh->DrawOffset;
		CurrentData->NavMeshGeometry.bGatherPolyEdges = NavMesh->bDrawPolyEdges;
		CurrentData->NavMeshGeometry.bGatherNavMeshEdges = NavMesh->bDrawNavMeshEdges;

		const FNavDataConfig& NavConfig = NavMesh->GetConfig();
		CurrentData->NavMeshColors[RECAST_DEFAULT_AREA] = NavConfig.Color.DWColor() > 0 ? NavConfig.Color : NavMeshRenderColor_RecastMesh;
		for (uint8 i = 0; i < RECAST_DEFAULT_AREA; ++i)
		{
			CurrentData->NavMeshColors[i] = NavMesh->GetAreaIDColor(i);
		}

		// just a little trick to make sure navmeshes with different sized are not drawn with same offset
		CurrentData->NavMeshDrawOffset.Z += NavMesh->GetConfig().AgentRadius / 10.f;

		NavMesh->BeginBatchQuery();
		NavMesh->GetDebugGeometry(CurrentData->NavMeshGeometry);
		const TArray<FVector>& MeshVerts = CurrentData->NavMeshGeometry.MeshVerts;

		// @fixme, this is going to double up on lots of interior lines
		if (NavMesh->bDrawTriangleEdges)
		{
			for (int32 AreaIdx = 0; AreaIdx < RECAST_MAX_AREAS; ++AreaIdx)
			{
				const TArray<int32>& MeshIndices = CurrentData->NavMeshGeometry.AreaIndices[AreaIdx];
				for (int32 Idx=0; Idx<MeshIndices.Num(); Idx += 3)
				{
					CurrentData->BatchedElements.AddLine(MeshVerts[MeshIndices[Idx+0]] + CurrentData->NavMeshDrawOffset, MeshVerts[MeshIndices[Idx+1]] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TriangleEdges,HitProxyId,DefaultEdges_LineThickness, 0, true);
					CurrentData->BatchedElements.AddLine(MeshVerts[MeshIndices[Idx+1]] + CurrentData->NavMeshDrawOffset, MeshVerts[MeshIndices[Idx+2]] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TriangleEdges,HitProxyId,DefaultEdges_LineThickness, 0, true);
					CurrentData->BatchedElements.AddLine(MeshVerts[MeshIndices[Idx+2]] + CurrentData->NavMeshDrawOffset, MeshVerts[MeshIndices[Idx+0]] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TriangleEdges,HitProxyId,DefaultEdges_LineThickness, 0, true);
				}
			}
		}

		// make lines for tile edges
		if (NavMesh->bDrawPolyEdges)
		{
			const TArray<FVector>& TileEdgeVerts = CurrentData->NavMeshGeometry.PolyEdges;
			for (int32 Idx=0; Idx < TileEdgeVerts.Num(); Idx += 2)
			{
				CurrentData->TileEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(TileEdgeVerts[Idx] + CurrentData->NavMeshDrawOffset, TileEdgeVerts[Idx+1] + CurrentData->NavMeshDrawOffset, NavMeshRenderColor_Recast_TileEdges));
			}
		}

		// make lines for navmesh edges
		if (NavMesh->bDrawNavMeshEdges)
		{
			const FColor EdgesColor = DarkenColor(CurrentData->NavMeshColors[RECAST_DEFAULT_AREA]);
			const TArray<FVector>& NavMeshEdgeVerts = CurrentData->NavMeshGeometry.NavMeshEdges;
			for (int32 Idx=0; Idx < NavMeshEdgeVerts.Num(); Idx += 2)
			{
				CurrentData->NavMeshEdgeLines.Add( FDebugRenderSceneProxy::FDebugLine(NavMeshEdgeVerts[Idx] + CurrentData->NavMeshDrawOffset, NavMeshEdgeVerts[Idx+1] + CurrentData->NavMeshDrawOffset, EdgesColor));
			}
		}

		if (NavMesh->bDrawTileBounds)
		{
			FBox OuterBBox;
			int32 NumTilesX, NumTilesY;
			NavMesh->GetDebugTileBounds(OuterBBox, NumTilesX, NumTilesY);

			float DrawZ = (OuterBBox.Min.Z + OuterBBox.Max.Z) * 0.5f;		// @hack average
			FVector LL(OuterBBox.Min.X, OuterBBox.Min.Y, DrawZ);
			FVector UR(OuterBBox.Max.X, OuterBBox.Max.Y, DrawZ);
			FVector UL(LL.X, UR.Y, DrawZ);
			FVector LR(UR.X, LL.Y, DrawZ);
			CurrentData->BatchedElements.AddLine(LL, UL, NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);
			CurrentData->BatchedElements.AddLine(UL, UR, NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);
			CurrentData->BatchedElements.AddLine(UR, LR, NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);
			CurrentData->BatchedElements.AddLine(LR, LL, NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);

			float const TileSizeX = (LL.X - LR.X) / NumTilesX;
			float const TileSizeY = (LL.Y - UL.Y) / NumTilesY;

			for (int32 Idx=1; Idx<NumTilesX; ++Idx)
			{
				float const DrawX = FMath::Lerp(LL.X, LR.X, (float)Idx/(float)NumTilesX);
				CurrentData->BatchedElements.AddLine(FVector(DrawX, LL.Y, DrawZ), FVector(DrawX, UL.Y, DrawZ), NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);
			}
			for (int32 Idx=1; Idx<NumTilesY; ++Idx)
			{
				float const DrawY = FMath::Lerp(LL.Y, UL.Y, (float)Idx/(float)NumTilesY);
				CurrentData->BatchedElements.AddLine(FVector(LL.X, DrawY, DrawZ), FVector(LR.X, DrawY, DrawZ), NavMeshRenderColor_TileBounds,HitProxyId,DefaultEdges_LineThickness, 0, true);
			}
		}

		if (NavMesh->bDrawPathCollidingGeometry)
		{
			// draw all geometry gathered in navoctree
			const FNavigationOctree* NavOctree = NavMesh->GetWorld()->GetNavigationSystem()->GetNavOctree();

			for (FNavigationOctree::TConstIterator<> It(*NavOctree); It.HasPendingNodes(); It.Advance())
			{
				const FNavigationOctree::FNode& Node = It.GetCurrentNode();
				for (FNavigationOctree::ElementConstIt ElementIt(Node.GetElementIt()); ElementIt; ElementIt++)
				{
					const FNavigationOctreeElement& Element = *ElementIt;
					if (Element.ShouldUseGeometry(&NavMesh->NavDataConfig) && Element.Data.CollisionData.Num())
					{
						const FRecastGeometryCache CachedGeometry(Element.Data.CollisionData.GetTypedData());
						AppendGeometry(CurrentData->PathCollidingGeomVerts, CurrentData->PathCollidingGeomIndices,
							CachedGeometry.Verts, CachedGeometry.Header.NumVerts, CachedGeometry.Indices, CachedGeometry.Header.NumFaces);
					}
				}
			}
		}
		
		// offset all navigation-link positions
		if (!NavMesh->bDrawClusters)
		{
			for (int32 OffMeshLineIndex = 0; OffMeshLineIndex < CurrentData->NavMeshGeometry.OffMeshLinks.Num(); ++OffMeshLineIndex)
			{
				FRecastDebugGeometry::FOffMeshLink& Link = CurrentData->NavMeshGeometry.OffMeshLinks[OffMeshLineIndex];
				const bool bLinkValid = (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left) && (Link.ValidEnds & FRecastDebugGeometry::OMLE_Right);

				if (NavMesh->bDrawFailedNavLinks || (NavMesh->bDrawNavLinks && bLinkValid))
				{
					const FVector V0 = Link.Left + CurrentData->NavMeshDrawOffset;
					const FVector V1 = Link.Right + CurrentData->NavMeshDrawOffset;
					const FColor LinkColor = ((Link.Direction && Link.ValidEnds) || (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left)) ? DarkenColor(CurrentData->NavMeshColors[Link.AreaID]) : NavMeshRenderColor_OffMeshConnectionInvalid;

					CacheArc(CurrentData->NavLinkLines, V0, V1, 0.4f, 4, LinkColor, LinkLines_LineThickness);

					const FVector VOffset(0, 0, FVector::Dist(V0, V1) * 1.333f);
					CacheArrowHead(CurrentData->NavLinkLines, V1, V0+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
					if (Link.Direction)
					{
						CacheArrowHead(CurrentData->NavLinkLines, V0, V1+VOffset, 30.f, LinkColor, LinkLines_LineThickness);
					}

					// if the connection as a whole is valid check if there are any of ends is invalid
					if (LinkColor != NavMeshRenderColor_OffMeshConnectionInvalid)
					{
						if (Link.Direction && (Link.ValidEnds & FRecastDebugGeometry::OMLE_Left) == 0)
						{
							// left end invalid - mark it
							DrawWireCylinder(CurrentData->NavLinkLines, V0, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, Link.Radius, NavMesh->AgentMaxStepHeight, 16, 0, DefaultEdges_LineThickness);
						}
						if ((Link.ValidEnds & FRecastDebugGeometry::OMLE_Right) == 0)
						{
							DrawWireCylinder(CurrentData->NavLinkLines, V1, FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), NavMeshRenderColor_OffMeshConnectionInvalid, Link.Radius, NavMesh->AgentMaxStepHeight, 16, 0, DefaultEdges_LineThickness);
						}
					}
				}					
			}
		}

		if (NavMesh->bDrawTileLabels || NavMesh->bDrawPolygonLabels || NavMesh->bDrawDefaultPolygonCost)
		{
			// calculate appropriate points for displaying debug labels
			const int32 TilesCount = NavMesh->GetNavMeshTilesCount();
			CurrentData->DebugLabels.Reserve(TilesCount);

			for (int32 TileIndex = 0; TileIndex < TilesCount; ++TileIndex)
			{
				int32 X = -1;
				int32 Y = -1;
				int32 Layer = -1;
				NavMesh->GetNavMeshTileXY(TileIndex, X, Y, Layer);

				if (X >= 0 && Y >= 0)
				{
					const FBox TileBoundingBox = NavMesh->GetNavMeshTileBounds(TileIndex);
					FVector TileLabelLocation = TileBoundingBox.GetCenter();
					TileLabelLocation.Z = TileBoundingBox.Max.Z;

					FNavLocation NavLocation(TileLabelLocation);
					if (!NavMesh->ProjectPoint(TileLabelLocation, NavLocation, FVector(NavMesh->TileSizeUU/100, NavMesh->TileSizeUU/100, TileBoundingBox.Max.Z-TileBoundingBox.Min.Z)))
					{
						NavMesh->ProjectPoint(TileLabelLocation, NavLocation, FVector(NavMesh->TileSizeUU/2, NavMesh->TileSizeUU/2, TileBoundingBox.Max.Z-TileBoundingBox.Min.Z));
					}

					if (NavMesh->bDrawTileLabels)
					{
						CurrentData->DebugLabels.Add(FNavMeshSceneProxyData::FDebugText(
							/*Location*/NavLocation.Location + CurrentData->NavMeshDrawOffset
							, /*Text*/FString::Printf(TEXT("(%d,%d:%d)"), X, Y, Layer)
							));
					}

					if (NavMesh->bDrawPolygonLabels || NavMesh->bDrawDefaultPolygonCost)
					{
						TArray<FNavPoly> Polys;
						NavMesh->GetPolysInTile(TileIndex, Polys);

						if (NavMesh->bDrawDefaultPolygonCost)
						{
							float DefaultCosts[RECAST_MAX_AREAS];
							float FixedCosts[RECAST_MAX_AREAS];

							NavMesh->GetDefaultQueryFilter()->GetAllAreaCosts(DefaultCosts, FixedCosts, RECAST_MAX_AREAS);

							for(int k = 0; k < Polys.Num(); ++k)
							{
								uint32 AreaID = NavMesh->GetPolyAreaID(Polys[k].Ref);

								CurrentData->DebugLabels.Add(FNavMeshSceneProxyData::FDebugText(
									/*Location*/Polys[k].Center + CurrentData->NavMeshDrawOffset
									, /*Text*/FString::Printf(TEXT("\\%.3f; %.3f\\"), DefaultCosts[AreaID], FixedCosts[AreaID])
									));
							}
						}
						else
						{
							for(int k = 0; k < Polys.Num(); ++k)
							{
								CurrentData->DebugLabels.Add(FNavMeshSceneProxyData::FDebugText(
									/*Location*/Polys[k].Center + CurrentData->NavMeshDrawOffset
									, /*Text*/FString::Printf(TEXT("[%08X]"), Polys[k].Ref)
									));
							}
						}
					}							
				}
			}
		}

		CurrentData->bSkipDistanceCheck = GIsEditor && (GEngine->GetDebugLocalPlayer() == NULL);
		CurrentData->bDrawClusters = NavMesh->bDrawClusters;
		NavMesh->FinishBatchQuery();

		// Draw Mesh
		if (NavMesh->bDrawClusters)
		{
			for (int32 Idx = 0; Idx < CurrentData->NavMeshGeometry.Clusters.Num(); ++Idx)
			{
				const TArray<int32>& MeshIndices = CurrentData->NavMeshGeometry.Clusters[Idx].MeshIndices;

				if (MeshIndices.Num() == 0)
				{
					continue;
				}

				FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
				for (int32 VertIdx=0; VertIdx < MeshVerts.Num(); ++VertIdx)
				{
					AddVertexHelper(DebugMeshData, MeshVerts[VertIdx] + CurrentData->NavMeshDrawOffset);
				}
				for (int32 TriIdx=0; TriIdx < MeshIndices.Num(); TriIdx+=3)
				{
					AddTriangleHelper(DebugMeshData, MeshIndices[TriIdx], MeshIndices[TriIdx+1], MeshIndices[TriIdx+2]);
				}

				DebugMeshData.ClusterColor = GetClusterColor(Idx);
				CurrentData->MeshBuilders.Add(DebugMeshData);
			}
		}
		else
		{			
			for (int32 AreaType = 0; AreaType < RECAST_MAX_AREAS; ++AreaType)
			{
				const TArray<int32>& MeshIndices = CurrentData->NavMeshGeometry.AreaIndices[AreaType];

				if (MeshIndices.Num() == 0)
				{
					continue;
				}

				FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
				for (int32 VertIdx=0; VertIdx < MeshVerts.Num(); ++VertIdx)
				{
					AddVertexHelper(DebugMeshData, MeshVerts[VertIdx] + CurrentData->NavMeshDrawOffset);
				}
				for (int32 TriIdx=0; TriIdx < MeshIndices.Num(); TriIdx+=3)
				{
					AddTriangleHelper(DebugMeshData, MeshIndices[TriIdx], MeshIndices[TriIdx+1], MeshIndices[TriIdx+2]);
				}

				DebugMeshData.ClusterColor = CurrentData->NavMeshColors[AreaType];
				CurrentData->MeshBuilders.Add(DebugMeshData);
			}
		}

		// Draw path generation input geometry
		if (CurrentData->bDrawPathCollidingGeometry)
		{
			FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
			for (int32 VertIdx=0; VertIdx < CurrentData->PathCollidingGeomVerts.Num(); ++VertIdx)
			{
				AddVertexHelper(DebugMeshData, CurrentData->PathCollidingGeomVerts[VertIdx]);
			}
			DebugMeshData.Indices.Append(CurrentData->PathCollidingGeomIndices);
			DebugMeshData.ClusterColor = NavMeshRenderColor_PathCollidingGeom;
			CurrentData->MeshBuilders.Add(DebugMeshData);
		}

		if (CurrentData->NavMeshGeometry.BuiltMeshIndices.Num() > 0)
		{
			FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
			for (int32 VertIdx=0; VertIdx < MeshVerts.Num(); ++VertIdx)
			{
				AddVertexHelper(DebugMeshData, MeshVerts[VertIdx] + CurrentData->NavMeshDrawOffset);
			}
			DebugMeshData.Indices.Append(CurrentData->NavMeshGeometry.BuiltMeshIndices);
			DebugMeshData.ClusterColor = NavMeshRenderColor_RecastTileBeingRebuilt;
			CurrentData->MeshBuilders.Add(DebugMeshData);

			// if we got here it means there's something being built, or is fresh so we better redraw in some time
			FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
				FSimpleDelegateGraphTask::FDelegate::CreateUObject(NavMesh, &ARecastNavMesh::MarkComponentsRenderStateDirty)
				, TEXT("Requesting navmesh redraw")
				, NULL
				, ENamedThreads::GameThread
				);
		}

		if (NavMesh->bDrawClusters)
		{
			for (int i = 0; i < CurrentData->NavMeshGeometry.ClusterLinks.Num(); i++)
			{
				const FRecastDebugGeometry::FClusterLink& CLink = CurrentData->NavMeshGeometry.ClusterLinks[i];
				const FVector V0 = CLink.FromCluster + CurrentData->NavMeshDrawOffset;
				const FVector V1 = CLink.ToCluster + CurrentData->NavMeshDrawOffset + FVector(0,0,20.0f);

				CacheArc(CurrentData->ClusterLinkLines, V0, V1, 0.4f, 4, FColor::Black, ClusterLinkLines_LineThickness);
				const FVector VOffset(0, 0, FVector::Dist(V0, V1) * 1.333f);
				CacheArrowHead(CurrentData->ClusterLinkLines, V1, V0+VOffset, 30.f, FColor::Black, ClusterLinkLines_LineThickness);
			}
		}

		// cache segment links
		if (NavMesh->bDrawNavLinks)
		{
			for (int32 iArea = 0; iArea < RECAST_MAX_AREAS; iArea++)
			{
				const TArray<int32>& Indices = CurrentData->NavMeshGeometry.OffMeshSegmentAreas[iArea];
				FNavMeshSceneProxyData::FDebugMeshData DebugMeshData;
				int32 VertBase = 0;

				for (int32 i = 0; i < Indices.Num(); i++)
				{
					FRecastDebugGeometry::FOffMeshSegment& SegInfo = CurrentData->NavMeshGeometry.OffMeshSegments[Indices[i]];
					const FVector A0 = SegInfo.LeftStart + CurrentData->NavMeshDrawOffset;
					const FVector A1 = SegInfo.LeftEnd + CurrentData->NavMeshDrawOffset;
					const FVector B0 = SegInfo.RightStart + CurrentData->NavMeshDrawOffset;
					const FVector B1 = SegInfo.RightEnd + CurrentData->NavMeshDrawOffset;
					const FVector Edge0 = B0 - A0;
					const FVector Edge1 = B1 - A1;
					const float Len0 = Edge0.Size();
					const float Len1 = Edge1.Size();
					const FColor SegColor = DarkenColor(CurrentData->NavMeshColors[SegInfo.AreaID]);
					const FColor ColA = (SegInfo.ValidEnds & FRecastDebugGeometry::OMLE_Left) ? FColor::White : FColor::Black;
					const FColor ColB = (SegInfo.ValidEnds & FRecastDebugGeometry::OMLE_Right) ? FColor::White : FColor::Black;

					const int32 NumArcPoints = 8;
					const float ArcPtsScale = 1.0f / NumArcPoints;

					FVector Prev0 = EvalArc(A0, Edge0, Len0*0.25f, 0);
					FVector Prev1 = EvalArc(A1, Edge1, Len1*0.25f, 0);
					AddVertexHelper(DebugMeshData, Prev0, ColA);
					AddVertexHelper(DebugMeshData, Prev1, ColA);
					for (int32 j = 1; j <= NumArcPoints; j++)
					{
						const float u = j * ArcPtsScale;
						FVector Pt0 = EvalArc(A0, Edge0, Len0*0.25f, u);
						FVector Pt1 = EvalArc(A1, Edge1, Len1*0.25f, u);

						AddVertexHelper(DebugMeshData, Pt0, (j == NumArcPoints) ? ColB : FColor::White);
						AddVertexHelper(DebugMeshData, Pt1, (j == NumArcPoints) ? ColB : FColor::White);

						AddTriangleHelper(DebugMeshData, VertBase+0, VertBase+2, VertBase+1);
						AddTriangleHelper(DebugMeshData, VertBase+2, VertBase+3, VertBase+1);
						AddTriangleHelper(DebugMeshData, VertBase+0, VertBase+1, VertBase+2);
						AddTriangleHelper(DebugMeshData, VertBase+2, VertBase+1, VertBase+3);

						VertBase += 2;
						Prev0 = Pt0;
						Prev1 = Pt1;
					}
					VertBase += 2;

					DebugMeshData.ClusterColor = SegColor;
				}

				if (DebugMeshData.Indices.Num())
				{
					CurrentData->MeshBuilders.Add(DebugMeshData);
				}
			}
		}

		CurrentData->NavMeshGeometry.PolyEdges.Empty();
		CurrentData->NavMeshGeometry.NavMeshEdges.Empty();
	}
#endif //WITH_RECAST
}
