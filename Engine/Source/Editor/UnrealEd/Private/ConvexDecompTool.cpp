// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ConvexDecompTool.cpp: Utility for turning graphics mesh into convex hulls.
=============================================================================*/

#include "UnrealEd.h"
#include "ThirdParty/HACD/HACD_1.0/public/HACD.h"
#include "ConvexDecompTool.h"

DEFINE_LOG_CATEGORY_STATIC(LogConvexDecompTool, Log, All);

using namespace HACD_STANDALONE;

class FHACDProgressCallback : public hacd::ICallback
{
public:
	virtual void ReportProgress(const char * Status, hacd::HaF32 Progress)
	{
		GWarn->StatusUpdate( Progress*1000.f, 1000, FText::FromString( FString( Status ) ) );
	}

	virtual bool Cancelled()
	{
		return (GWarn->ReceivedUserCancel() != false);
	}
};

void DecomposeMeshToHulls(UBodySetup* InBodySetup, const TArray<FVector>& InVertices, const TArray<uint32>& InIndices, int32 InMaxHullCount, int32 InMaxHullVerts)
{
	check(InBodySetup != NULL);

	// Fill in data for decomposition tool.
	HACD_API::Desc Desc;
	Desc.mVertexCount		=	InVertices.Num();
	Desc.mVertices			=	(float*)InVertices.GetData();

	Desc.mTriangleCount		=	InIndices.Num()/3;
	Desc.mIndices			=	(uint32*)InIndices.GetData();

	Desc.mMaxMergeHullCount	=	InMaxHullCount;
	Desc.mMaxHullVertices	=	InMaxHullVerts;

	Desc.mUseFastVersion			= 	false;
	Desc.mBackFaceDistanceFactor	= 	0.00000000001f;
	Desc.mDecompositionDepth		= 	2.0f;

	FHACDProgressCallback Callback;
	Desc.mCallback			= &Callback;

	// Perform the work
	HACD_API* HAPI = createHACD_API();

	GWarn->BeginSlowTask( NSLOCTEXT("ConvexDecompTool", "BeginCreatingCollisionTask", "Creating Collision"), true, true );

	uint32 Result = HAPI->performHACD(Desc);

	GWarn->EndSlowTask();

	//UE_LOG(LogConvexDecompTool, Log, TEXT("Result: %d"));

	// Clean out old hulls
	InBodySetup->RemoveSimpleCollision();

	// Iterate over each result hull
	int32 NumHulls = HAPI->getHullCount();
	for(int32 HullIdx=0; HullIdx<NumHulls; HullIdx++)
	{
		// Create a new hull in the aggregate geometry
		const int32 NewConvexIndex = InBodySetup->AggGeom.ConvexElems.Add(FKConvexElem());
		FKConvexElem& Convex = InBodySetup->AggGeom.ConvexElems[NewConvexIndex];

		// Read out each hull vertex
		const HACD_API::Hull* Hull = HAPI->getHull(HullIdx);
		for(uint32 VertIdx=0; VertIdx<Hull->mVertexCount; VertIdx++)
		{
			FVector* V = (FVector*)(Hull->mVertices + (VertIdx*3));

			Convex.VertexData.Add(*V);
		}

		Convex.UpdateElemBox();
	}

	InBodySetup->InvalidatePhysicsData(); // update GUID
}
