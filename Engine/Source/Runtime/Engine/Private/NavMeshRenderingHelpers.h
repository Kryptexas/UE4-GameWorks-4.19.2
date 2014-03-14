// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "DebugRenderSceneProxy.h"
#include "AI/Navigation/RecastHelpers.h"

static const FColor NavMeshRenderColor_Recast_TriangleEdges(255,255,255);
static const FColor NavMeshRenderColor_Recast_TileEdges(16,16,16,32);
static const FColor NavMeshRenderColor_Recast_NavMeshEdges(32,63,0,220);
static const FColor NavMeshRenderColor_RecastMesh(140,255,0,164);
static const FColor NavMeshRenderColor_TileBounds(255,255,64,255);
static const FColor NavMeshRenderColor_PathCollidingGeom(255,255,255,40);
static const FColor NavMeshRenderColor_RecastTileBeingRebuilt(255,0,0,64);
static const FColor NavMeshRenderColor_OffMeshConnectionInvalid(64,64,64);

static const float DefaultEdges_LineThickness = 0.0f;
static const float PolyEdges_LineThickness = 1.5f;
static const float NavMeshEdges_LineThickness = 3.5f;
static const float LinkLines_LineThickness = 2.0f;
static const float ClusterLinkLines_LineThickness = 2.0f;

FORCEINLINE FColor DarkenColor(const FColor& Base)
{
	const uint32 Col = Base.DWColor();
	return FColor(((Col >> 1) & 0x007f7f7f) | (Col & 0xff000000));
}

struct FNavMeshSceneProxyData : public TSharedFromThis<FNavMeshSceneProxyData, ESPMode::ThreadSafe>
{
#if WITH_RECAST

	FNavMeshSceneProxyData() : NavMeshDrawOffset(0,0,10.f), bNeedsNewData(true) {}

	void Reset()
	{
		for (int32 Index=0; Index < RECAST_MAX_AREAS; Index++)
			NavMeshGeometry.AreaIndices[Index].Reset();
		NavMeshGeometry.MeshVerts.Reset();
		for (int32 Index=0; Index < RECAST_MAX_AREAS; Index++)
			NavMeshGeometry.AreaIndices[Index].Reset();
		NavMeshGeometry.BuiltMeshIndices.Reset();
		NavMeshGeometry.PolyEdges.Reset();
		NavMeshGeometry.NavMeshEdges.Reset();
		NavMeshGeometry.OffMeshLinks.Reset();
		NavMeshGeometry.Clusters.Reset();
		NavMeshGeometry.ClusterLinks.Reset();
		NavMeshGeometry.OffMeshSegments.Reset();
		for (int32 Index=0; Index < RECAST_MAX_AREAS; Index++)
			NavMeshGeometry.OffMeshSegmentAreas[Index].Reset();
		BatchedElements.Clear();
		TileEdgeLines.Reset();
		NavMeshEdgeLines.Reset();
		NavLinkLines.Reset();
		ClusterLinkLines.Reset();
		PathCollidingGeomIndices.Reset();
		PathCollidingGeomVerts.Reset();
		MeshBuilders.Reset();
		DebugLabels.Reset();

		bNeedsNewData = true;
		bDataGathered = false;
		bSkipDistanceCheck = false;
		bDrawClusters = false;
		bEnableDrawing = false;
		bShowOnlyDefaultAgent = false;
		bSupportsDefaultAgent = false;
		bDrawPathCollidingGeometry = false;
	}

	FRecastDebugGeometry NavMeshGeometry;
	FBatchedElements BatchedElements;

	TArray<FDebugRenderSceneProxy::FDebugLine> TileEdgeLines;
	TArray<FDebugRenderSceneProxy::FDebugLine> NavMeshEdgeLines;
	TArray<FDebugRenderSceneProxy::FDebugLine> NavLinkLines;
	TArray<FDebugRenderSceneProxy::FDebugLine> ClusterLinkLines;

	TArray<int32> PathCollidingGeomIndices;
	TNavStatArray<FVector> PathCollidingGeomVerts;

	FColor NavMeshColors[RECAST_MAX_AREAS];

	struct FDebugMeshData
	{
		TArray<FDynamicMeshVertex> Vertices;
		TArray<int32> Indices;
		FColor ClusterColor;
	};
	TArray<FDebugMeshData>	MeshBuilders;

	struct FDebugText
	{
		const FVector Location;
		const FString Text;
		FDebugText(const FVector& InLocation, const FString& InText) : Location(InLocation), Text(InText)
		{}
	};
	TArray<FDebugText> DebugLabels;

	FVector NavMeshDrawOffset;
	FCriticalSection CriticalSection;
	uint32 bDataGathered : 1;
	uint32 bSkipDistanceCheck : 1;
	uint32 bDrawClusters : 1;
	uint32 bEnableDrawing:1;
	uint32 bShowOnlyDefaultAgent:1;
	uint32 bSupportsDefaultAgent:1;
	uint32 bDrawPathCollidingGeometry:1;
	uint32 bNeedsNewData:1;
#endif // WITH_RECAST
};

#if WITH_RECAST

FORCEINLINE bool LineInView(const FVector& Start, const FVector& End, const FSceneView* View, bool bUseDistanceCheck)
{
	if (bUseDistanceCheck && (FVector::DistSquared(Start, View->ViewMatrices.ViewOrigin) > ARecastNavMesh::GetDrawDistanceSq()
		|| FVector::DistSquared(End, View->ViewMatrices.ViewOrigin) > ARecastNavMesh::GetDrawDistanceSq()))
	{
		return false;
	}

	for(int32 PlaneIdx=0;PlaneIdx<View->ViewFrustum.Planes.Num();++PlaneIdx)
	{
		const FPlane& CurPlane = View->ViewFrustum.Planes[PlaneIdx];
		if(CurPlane.PlaneDot(Start) > 0.f && CurPlane.PlaneDot(End) > 0.f)
		{
			return false;
		}
	}

	return true;
}

FORCEINLINE bool LineInCorrectDistance(const FVector& Start, const FVector& End, const FSceneView* View, float CorrectDistance = -1)
{
	const float MaxDistance = CorrectDistance > 0 ? (CorrectDistance*CorrectDistance) : ARecastNavMesh::GetDrawDistanceSq();

	if ((FVector::DistSquared(Start, View->ViewMatrices.ViewOrigin) > MaxDistance
		|| FVector::DistSquared(End, View->ViewMatrices.ViewOrigin) > MaxDistance))
	{
		return false;
	}
	return true;
}


class FRecastRenderingSceneProxy : public FDebugRenderSceneProxy
{
public:
	FRecastRenderingSceneProxy(const UPrimitiveComponent* InComponent, TSharedPtr<FNavMeshSceneProxyData, ESPMode::ThreadSafe> InProxyData, 
		const FSimpleDelegateGraphTask::FDelegate& InTaskDeletegate = FSimpleDelegateGraphTask::FDelegate(), bool ForceToRender = false) 
		: FDebugRenderSceneProxy(InComponent)
		, ProxyData(InProxyData)
		, bRequestedData(false)
		, bForceRendering(ForceToRender)
	{
		TaskDeletegate = InTaskDeletegate;
		RenderingComponent = Cast<const UNavMeshRenderingComponent>(InComponent);
	}

	virtual ~FRecastRenderingSceneProxy() 
	{
		ProxyData = NULL;
	}


	void RegisterDebugDrawDelgate()
	{
		DebugTextDrawingDelegate = FDebugDrawDelegate::CreateRaw(this, &FRecastRenderingSceneProxy::DrawDebugLabels);
		UDebugDrawService::Register(TEXT("Navigation"), DebugTextDrawingDelegate);
	}

	void UnregisterDebugDrawDelgate()
	{
		if (DebugTextDrawingDelegate.IsBound())
		{
			UDebugDrawService::Unregister(DebugTextDrawingDelegate);
		}
	}

	FORCEINLINE uint8 GetBit(int32 v, uint8 bit) const
	{
		return (v & (1 << bit)) >> bit;
	}

	FORCEINLINE FColor GetClusterColor(int32 Idx) const
	{
		const uint8 r = 1 + GetBit(Idx, 1) + GetBit(Idx, 3) * 2;
		const uint8 g = 1 + GetBit(Idx, 2) + GetBit(Idx, 4) * 2;
		const uint8 b = 1 + GetBit(Idx, 0) + GetBit(Idx, 5) * 2;
		return FColor(r*63, g*63, b*63, 164);
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_RecastRenderingSceneProxy_DrawDynamicElements );

		if (!bRequestedData && ProxyData.IsValid() && ProxyData->bNeedsNewData)
		{
			if ((RenderingComponent.IsValid() && !RenderingComponent->IsPendingKill()) || TaskDeletegate.IsBound())
			{
				FGraphEventRef CompleteHandle = FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
					TaskDeletegate.IsBound() ? TaskDeletegate : FSimpleDelegateGraphTask::FDelegate::CreateUObject(RenderingComponent.Get(), &UNavMeshRenderingComponent::GatherDataForProxy)
					, TEXT("Requesting navmesh data")
					, NULL
					, ENamedThreads::GameThread
					);
				bRequestedData = true;
			}
			return;
		}

		if (ProxyData.IsValid() == false || !ProxyData->bEnableDrawing || (ProxyData->bShowOnlyDefaultAgent && !ProxyData->bSupportsDefaultAgent))
		{
			return;
		}

		// scope to lock access to rendering data (it can be changed on game thread)
		{
			FScopeLock ScopeLock(&ProxyData->CriticalSection);
			ProxyData->bSkipDistanceCheck = GIsEditor && (GEngine->GetDebugLocalPlayer() == NULL);

			FVector const PosX(1.f,0,0);
			FVector const PosY(0,1.f,0);
			FVector const PosZ(0,0,1.f);

			const TArray<FVector>& MeshVerts = ProxyData->NavMeshGeometry.MeshVerts;

			// Draw Mesh
			for (int32 Index = 0; Index < ProxyData->MeshBuilders.Num(); ++Index)
			{
				const FColoredMaterialRenderProxy *MeshColorInstance = new(FMemStack::Get()) FColoredMaterialRenderProxy(GEngine->DebugMeshMaterial->GetRenderProxy(false), ProxyData->MeshBuilders[Index].ClusterColor);						
				FDynamicMeshBuilder	MeshBuilder;
				MeshBuilder.AddVertices( ProxyData->MeshBuilders[Index].Vertices );
				MeshBuilder.AddTriangles( ProxyData->MeshBuilders[Index].Indices );
				MeshBuilder.Draw(PDI, FMatrix::Identity, MeshColorInstance, GetDepthPriorityGroup(View));
			}

			FHitProxyId HitProxyId;
			FBatchedElements BatchedElements;
			int32 Num = ProxyData->NavMeshEdgeLines.Num();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				const FDebugLine &Line = ProxyData->NavMeshEdgeLines[Index];
				if( LineInView(Line.Start,Line.End,View,false) )
				{
					if (LineInCorrectDistance(Line.Start,Line.End,View))
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color,HitProxyId,NavMeshEdges_LineThickness, 0, true);
					}
					else if (GIsEditor)
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color,HitProxyId,DefaultEdges_LineThickness, 0, true);
					}
				}
			}

			Num = ProxyData->ClusterLinkLines.Num();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				const FDebugLine &Line = ProxyData->ClusterLinkLines[Index];
				if( LineInView(Line.Start,Line.End,View,false) )
				{
					if (LineInCorrectDistance(Line.Start,Line.End,View))
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color,HitProxyId,ClusterLinkLines_LineThickness, 0, true);
					}
					else if (GIsEditor)
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color, HitProxyId, DefaultEdges_LineThickness, 0, true);
					}
				}
			}

			Num = ProxyData->TileEdgeLines.Num();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				const FDebugLine &Line = ProxyData->TileEdgeLines[Index];
				if( LineInView(Line.Start,Line.End,View,false) )
				{
					if (LineInCorrectDistance(Line.Start,Line.End,View))
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color,HitProxyId,PolyEdges_LineThickness, 0, true);
					}
					else if (GIsEditor)
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color, HitProxyId, DefaultEdges_LineThickness, 0, true);
					}
				}
			}

			Num = ProxyData->NavLinkLines.Num();
			for (int32 Index = 0; Index < Num; ++Index)
			{
				const FDebugLine &Line = ProxyData->NavLinkLines[Index];
				if( LineInView(Line.Start,Line.End,View,false) )
				{
					if (LineInCorrectDistance(Line.Start,Line.End,View))
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color,HitProxyId,LinkLines_LineThickness, 0, true);
					}
					else if (GIsEditor)
					{
						BatchedElements.AddLine(Line.Start, Line.End, Line.Color, HitProxyId, DefaultEdges_LineThickness, 0, true);
					}
				}
			}

			const bool bNeedToSwitchVerticalAxis = IsES2Platform(GRHIShaderPlatform) && !IsPCPlatform(GRHIShaderPlatform);
			const FTexture2DRHIRef DepthTexture;
			BatchedElements.Draw(
				bNeedToSwitchVerticalAxis,
				View->ViewProjectionMatrix,
				View->ViewRect.Width(),
				View->ViewRect.Height(),
				View->Family->EngineShowFlags.HitProxies,
				1.0f,
				View,
				DepthTexture
				);

			ProxyData->BatchedElements.Draw(
				bNeedToSwitchVerticalAxis,
				View->ViewProjectionMatrix,
				View->ViewRect.Width(),
				View->ViewRect.Height(),
				View->Family->EngineShowFlags.HitProxies,
				1.0f,
				View,
				DepthTexture
				);
		}
	}

	FORCEINLINE bool PointInView(const FVector& Location, const FSceneView* View)
	{
		return View->ViewFrustum.IntersectBox(Location, FVector::ZeroVector);
	}

	void DrawDebugLabels(UCanvas* Canvas, APlayerController*)
	{
		if (ProxyData->bNeedsNewData == true || ProxyData->bEnableDrawing == false)
		{
			return;
		}

		const FColor OldDrawColor = Canvas->DrawColor;
		Canvas->SetDrawColor(FColor::White);
		const FSceneView* View = Canvas->SceneView;
		UFont* Font = GEngine->GetSmallFont();
		const FNavMeshSceneProxyData::FDebugText* DebugText = ProxyData->DebugLabels.GetTypedData();
		for (int32 i = 0 ; i < ProxyData->DebugLabels.Num(); ++i, ++DebugText)
		{
			if (PointInView(DebugText->Location, View))
			{
				const FVector ScreenLoc = Canvas->Project(DebugText->Location);
				Canvas->DrawText(Font, DebugText->Text, ScreenLoc.X, ScreenLoc.Y);
			}
		}

		Canvas->SetDrawColor(OldDrawColor);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		const bool bVisible = !!View->Family->EngineShowFlags.Navigation || bForceRendering;
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = bVisible && IsShown(View);
		Result.bDynamicRelevance = true;
		Result.bNormalTranslucencyRelevance = bVisible && IsShown(View);
		return Result;
	}

	virtual uint32 GetMemoryFootprint( void ) const { return sizeof( *this ) + GetAllocatedSize(); }
	uint32 GetAllocatedSize( void ) const 
	{ 
		return FDebugRenderSceneProxy::GetAllocatedSize() + 
			(sizeof(FNavMeshSceneProxyData) + ProxyData->NavMeshGeometry.GetAllocatedSize()
			+ ProxyData->TileEdgeLines.GetAllocatedSize() + ProxyData->NavMeshEdgeLines.GetAllocatedSize() + ProxyData->NavLinkLines.GetAllocatedSize()
			+ ProxyData->PathCollidingGeomIndices.GetAllocatedSize() + ProxyData->PathCollidingGeomVerts.GetAllocatedSize()
			+ ProxyData->DebugLabels.GetAllocatedSize() + ProxyData->ClusterLinkLines.GetAllocatedSize() + ProxyData->MeshBuilders.GetAllocatedSize()
			+ ProxyData->BatchedElements.GetAllocatedSize());
	}

private:
	TSharedPtr<FNavMeshSceneProxyData, ESPMode::ThreadSafe>	ProxyData;
	FDebugDrawDelegate DebugTextDrawingDelegate;
	TWeakObjectPtr<UNavMeshRenderingComponent> RenderingComponent;
	FSimpleDelegateGraphTask::FDelegate TaskDeletegate;
	uint32 bRequestedData : 1;
	uint32 bForceRendering : 1;
};
#endif // WITH_RECAST

FORCEINLINE void AppendGeometry(TNavStatArray<FVector>& OutVertexBuffer, TArray<int32>& OutIndexBuffer,
								const float* Coords, int32 NumVerts, const int32* Faces, int32 NumFaces)
{
	const int32 VertIndexBase = OutVertexBuffer.Num();
	for (int32 VertIdx = 0; VertIdx < NumVerts; ++VertIdx)
	{
		OutVertexBuffer.Add(Recast2UnrealPoint(&Coords[VertIdx * 3]));
	}

	const int32 FirstNewFaceVertexIndex = OutIndexBuffer.Num();
	OutIndexBuffer.AddUninitialized(NumFaces * 3);
	int32* DestFaceVertIndex = OutIndexBuffer.GetTypedData() + FirstNewFaceVertexIndex;
	const int32* SrcFaceVertIndex = Faces;
	// copy with offset
	for (int32 Index = 0; Index < NumFaces * 3; ++Index, ++DestFaceVertIndex, ++SrcFaceVertIndex)
	{
		*DestFaceVertIndex = *SrcFaceVertIndex + VertIndexBase;
	}
}

FORCEINLINE FVector EvalArc(const FVector& Org, const FVector& Dir, const float h, const float u)
{
	FVector Pt = Org + Dir * u;
	Pt.Z += h * (1-(u*2-1)*(u*2-1));

	return Pt;
}

FORCEINLINE void CacheArc(TArray<FDebugRenderSceneProxy::FDebugLine>& DebugLines, const FVector& Start, const FVector& End, const float Height, const uint32 Segments, const FLinearColor& Color, float LineThickness=0)
{
	if (Segments == 0)
	{
		return;
	}

	const float ArcPtsScale = 1.0f / (float)Segments;
	const FVector Dir = End - Start;
	const float Length = Dir.Size();

	FVector Prev = Start;
	for (uint32 i = 1; i <= Segments; ++i)
	{
		const float u = i * ArcPtsScale;
		const FVector Pt = EvalArc(Start, Dir, Length*Height, u);

		DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(Prev, Pt, Color));
		Prev = Pt;
	}
}

FORCEINLINE void CacheArrowHead(TArray<FDebugRenderSceneProxy::FDebugLine>& DebugLines, const FVector& Tip, const FVector& Origin, const float Size, const FLinearColor& Color, float LineThickness=0)
{
	FVector Ax, Ay, Az(0,1,0);
	Ay = Origin - Tip;
	Ay.Normalize();
	Ax = FVector::CrossProduct(Az, Ay);

	FHitProxyId HitProxyId;
	DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(Tip, FVector(Tip.X + Ay.X*Size + Ax.X*Size/3, Tip.Y + Ay.Y*Size + Ax.Y*Size/3, Tip.Z + Ay.Z*Size + Ax.Z*Size/3), Color));
	DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(Tip, FVector(Tip.X + Ay.X*Size - Ax.X*Size/3, Tip.Y + Ay.Y*Size - Ax.Y*Size/3, Tip.Z + Ay.Z*Size - Ax.Z*Size/3), Color));
}

FORCEINLINE void DrawWireCylinder(TArray<FDebugRenderSceneProxy::FDebugLine>& DebugLines,const FVector& Base,const FVector& X,const FVector& Y,const FVector& Z,FColor Color,float Radius,float HalfHeight,int32 NumSides,uint8 DepthPriority, float LineThickness=0)
{
	const float	AngleDelta = 2.0f * PI / NumSides;
	FVector	LastVertex = Base + X * Radius;

	FHitProxyId HitProxyId;
	for(int32 SideIndex = 0;SideIndex < NumSides;SideIndex++)
	{
		const FVector Vertex = Base + (X * FMath::Cos(AngleDelta * (SideIndex + 1)) + Y * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;

		DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(LastVertex - Z * HalfHeight,Vertex - Z * HalfHeight,Color));
		DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(LastVertex + Z * HalfHeight,Vertex + Z * HalfHeight,Color));
		DebugLines.Add(FDebugRenderSceneProxy::FDebugLine(LastVertex - Z * HalfHeight,LastVertex + Z * HalfHeight,Color));

		LastVertex = Vertex;
	}
}

FORCEINLINE uint8 GetBit(int32 v, uint8 bit)
{
	return (v & (1 << bit)) >> bit;
}

FORCEINLINE FColor GetClusterColor(int32 Idx)
{
	uint8 r = 1 + GetBit(Idx, 1) + GetBit(Idx, 3) * 2;
	uint8 g = 1 + GetBit(Idx, 2) + GetBit(Idx, 4) * 2;
	uint8 b = 1 + GetBit(Idx, 0) + GetBit(Idx, 5) * 2;
	return FColor(r*63, g*63, b*63, 164);
}

#if WITH_RECAST
FORCEINLINE static void AddVertexHelper(FNavMeshSceneProxyData::FDebugMeshData& MeshData, const FVector& Pos, const FColor Color = FColor::White)
{
	const int32 VertexIndex = MeshData.Vertices.Num();
	FDynamicMeshVertex* Vertex = new(MeshData.Vertices) FDynamicMeshVertex;
	Vertex->Position = Pos;
	Vertex->TextureCoordinate = FVector2D::ZeroVector;
	Vertex->TangentX = FVector(1.0f, 0.0f, 0.0f);
	Vertex->TangentZ = FVector(0.0f, 1.0f, 0.0f);
	// store the sign of the determinant in TangentZ.W (-1=0,+1=255)
	Vertex->TangentZ.Vector.W = 255;
	Vertex->Color = Color;
}

FORCEINLINE static void AddTriangleHelper(FNavMeshSceneProxyData::FDebugMeshData& MeshData, int32 V0, int32 V1, int32 V2)
{
	MeshData.Indices.Add(V0);
	MeshData.Indices.Add(V1);
	MeshData.Indices.Add(V2);
}
#endif	//WITH_RECAST
