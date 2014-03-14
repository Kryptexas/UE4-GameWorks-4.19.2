// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GeomFitUtils.cpp: Utilities for fitting collision models to static meshes.
=============================================================================*/

#include "UnrealEd.h"
#include "BSPOps.h"
#include "../Private/GeomFitUtils.h"
#include "RawMesh.h"
#include "MeshUtilities.h"

#define LOCAL_EPS (0.01f)
static void AddVertexIfNotPresent(TArray<FVector> &vertices, FVector &newVertex)
{
	bool isPresent = 0;

	for(int32 i=0; i<vertices.Num() && !isPresent; i++)
	{
		float diffSqr = (newVertex - vertices[i]).SizeSquared();
		if(diffSqr < LOCAL_EPS * LOCAL_EPS)
			isPresent = 1;
	}

	if(!isPresent)
		vertices.Add(newVertex);

}

/* ******************************** KDOP ******************************** */

// This function takes the current collision model, and fits a k-DOP around it.
// It uses the array of k unit-length direction vectors to define the k bounding planes.

// THIS FUNCTION REPLACES EXISTING SIMPLE COLLISION MODEL WITH KDOP
#define MY_FLTMAX (3.402823466e+38F)

void GenerateKDopAsSimpleCollision(UStaticMesh* StaticMesh,TArray<FVector> &dirs)
{
	// Make sure rendering is done - so we are not changing data being used by collision drawing.
	FlushRenderingCommands();

	UBodySetup* bs = StaticMesh->BodySetup;
	if(bs && (bs->AggGeom.GetElementCount() > 0))
	{
		// If we already have some simplified collision for this mesh - check before we clobber it.
		bool doReplace = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "StaticMeshAlreadyHasGeom", "Static Mesh already has simple collision.\nAre you sure you want replace it?") );

		if(doReplace)
		{
			bs->RemoveSimpleCollision();
		}
		else
		{
			return;
		}
	}
	else
	{
		// Otherwise, create one here.
		StaticMesh->CreateBodySetup();
		bs = StaticMesh->BodySetup;
	}

	// Do k- specific stuff.
	int32 kCount = dirs.Num();
	TArray<float> maxDist;
	for(int32 i=0; i<kCount; i++)
		maxDist.Add(-MY_FLTMAX);

	// Construct temporary UModel for kdop creation. We keep no refs to it, so it can be GC'd.
	UModel* TempModel = new UModel(FPostConstructInitializeProperties(),NULL,1);

	// For each vertex, project along each kdop direction, to find the max in that direction.
	const FStaticMeshLODResources& RenderData = StaticMesh->RenderData->LODResources[0];
	for(int32 i=0; i<RenderData.GetNumVertices(); i++)
	{
		for(int32 j=0; j<kCount; j++)
		{
			float dist = RenderData.PositionVertexBuffer.VertexPosition(i) | dirs[j];
			maxDist[j] = FMath::Max(dist, maxDist[j]);
		}
	}

	// Make sure that the min size is reached
	const float MinSize = 0.5f;

	for(int32 i=0; i<kCount; i++)
	{
		if (FMath::Abs(maxDist[i]) < MinSize)
		{
			maxDist[i] = MinSize;
		}
	}

	// Now we have the planes of the kdop, we work out the face polygons.
	TArray<FPlane> planes;
	for(int32 i=0; i<kCount; i++)
		planes.Add( FPlane(dirs[i], maxDist[i]) );

	for(int32 i=0; i<planes.Num(); i++)
	{
		FPoly*	Polygon = new(TempModel->Polys->Element) FPoly();
		FVector Base, AxisX, AxisY;

		Polygon->Init();
		Polygon->Normal = planes[i];
		Polygon->Normal.FindBestAxisVectors(AxisX,AxisY);

		Base = planes[i] * planes[i].W;

		new(Polygon->Vertices) FVector(Base + AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base + AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base - AxisX * HALF_WORLD_MAX - AxisY * HALF_WORLD_MAX);
		new(Polygon->Vertices) FVector(Base - AxisX * HALF_WORLD_MAX + AxisY * HALF_WORLD_MAX);

		for(int32 j=0; j<planes.Num(); j++)
		{
			if(i != j)
			{
				if(!Polygon->Split(-FVector(planes[j]), planes[j] * planes[j].W))
				{
					Polygon->Vertices.Empty();
					break;
				}
			}
		}

		if(Polygon->Vertices.Num() < 3)
		{
			// If poly resulted in no verts, remove from array
			TempModel->Polys->Element.RemoveAt(TempModel->Polys->Element.Num()-1);
		}
		else
		{
			// Other stuff...
			Polygon->iLink = i;
			Polygon->CalcNormal(1);
		}
	}

	if(TempModel->Polys->Element.Num() < 4)
	{
		TempModel = NULL;
		return;
	}

	// Build bounding box.
	TempModel->BuildBound();

	// Build BSP for the brush.
	FBSPOps::bspBuild(TempModel,FBSPOps::BSP_Good,15,70,1,0);
	FBSPOps::bspRefresh(TempModel,1);
	FBSPOps::bspBuildBounds(TempModel);

	bs->CreateFromModel(TempModel, true);
	
	// create all body instances
	RefreshCollisionChange(StaticMesh);

	// Mark staticmesh as dirty, to help make sure it gets saved.
	StaticMesh->MarkPackageDirty();
}

/* ******************************** SPHERE ******************************** */

// Can do bounding circles as well... Set elements of limitVect to 1.f for directions to consider, and 0.f to not consider.
// Have 2 algorithms, seem better in different cirumstances

// This algorithm taken from Ritter, 1990
// This one seems to do well with asymmetric input.
static void CalcBoundingSphere(const FRawMesh& RawMesh, FSphere& sphere, FVector& LimitVec)
{
	FBox Box;

	if(RawMesh.VertexPositions.Num() == 0)
		return;

	int32 minIx[3], maxIx[3]; // Extreme points.

	// First, find AABB, remembering furthest points in each dir.
	Box.Min = RawMesh.VertexPositions[0] * LimitVec;
	Box.Max = Box.Min;

	minIx[0] = minIx[1] = minIx[2] = 0;
	maxIx[0] = maxIx[1] = maxIx[2] = 0;

	for(uint32 i=1; i<(uint32)RawMesh.VertexPositions.Num(); i++) 
	{
		FVector p = RawMesh.VertexPositions[i] * LimitVec;

		// X //
		if(p.X < Box.Min.X)
		{
			Box.Min.X = p.X;
			minIx[0] = i;
		}
		else if(p.X > Box.Max.X)
		{
			Box.Max.X = p.X;
			maxIx[0] = i;
		}

		// Y //
		if(p.Y < Box.Min.Y)
		{
			Box.Min.Y = p.Y;
			minIx[1] = i;
		}
		else if(p.Y > Box.Max.Y)
		{
			Box.Max.Y = p.Y;
			maxIx[1] = i;
		}

		// Z //
		if(p.Z < Box.Min.Z)
		{
			Box.Min.Z = p.Z;
			minIx[2] = i;
		}
		else if(p.Z > Box.Max.Z)
		{
			Box.Max.Z = p.Z;
			maxIx[2] = i;
		}
	}

	//  Now find extreme points furthest apart, and initial centre and radius of sphere.
	float d2 = 0.f;
	for(int32 i=0; i<3; i++)
	{
		FVector diff = (RawMesh.VertexPositions[maxIx[i]] - RawMesh.VertexPositions[minIx[i]]) * LimitVec;
		float tmpd2 = diff.SizeSquared();

		if(tmpd2 > d2)
		{
			d2 = tmpd2;
			FVector centre = RawMesh.VertexPositions[minIx[i]] + (0.5f * diff);
			centre *= LimitVec;
			sphere.Center.X = centre.X;
			sphere.Center.Y = centre.Y;
			sphere.Center.Z = centre.Z;
			sphere.W = 0.f;
		}
	}

	// radius and radius squared
	float r = 0.5f * FMath::Sqrt(d2);
	float r2 = r * r;

	// Now check each point lies within this sphere. If not - expand it a bit.
	for(uint32 i=0; i<(uint32)RawMesh.VertexPositions.Num(); i++) 
	{
		FVector cToP = (RawMesh.VertexPositions[i] * LimitVec) - sphere.Center;
		float pr2 = cToP.SizeSquared();

		// If this point is outside our current bounding sphere..
		if(pr2 > r2)
		{
			// ..expand sphere just enough to include this point.
			float pr = FMath::Sqrt(pr2);
			r = 0.5f * (r + pr);
			r2 = r * r;

			sphere.Center += ((pr-r)/pr * cToP);
		}
	}

	sphere.W = r;
}

// This is the one thats already used by unreal.
// Seems to do better with more symmetric input...
static void CalcBoundingSphere2(const FRawMesh& RawMesh, FSphere& sphere, FVector& LimitVec)
{
	FBox Box(0);
	
	for(uint32 i=0; i<(uint32)RawMesh.VertexPositions.Num(); i++)
	{
		Box += RawMesh.VertexPositions[i] * LimitVec;
	}

	FVector centre, extent;
	Box.GetCenterAndExtents(centre, extent);

	sphere.Center.X = centre.X;
	sphere.Center.Y = centre.Y;
	sphere.Center.Z = centre.Z;
	sphere.W = 0;

	for( uint32 i=0; i<(uint32)RawMesh.VertexPositions.Num(); i++ )
	{
		float Dist = FVector::DistSquared(RawMesh.VertexPositions[i] * LimitVec, sphere.Center);
		if( Dist > sphere.W )
			sphere.W = Dist;
	}
	sphere.W = FMath::Sqrt(sphere.W);
}

// // //

void GenerateSphereAsSimpleCollision(UStaticMesh* StaticMesh)
{
	UBodySetup* bs = StaticMesh->BodySetup;
	if(bs && (bs->AggGeom.GetElementCount() > 0))
	{
		bool doReplace = EAppReturnType::Yes == FMessageDialog::Open( EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "StaticMeshAlreadyHasGeom", "Static Mesh already has simple collision.\nAre you sure you want replace it?") );

		if(doReplace)
		{
			bs->RemoveSimpleCollision();
		}
		else
		{
			return;
		}
	}
	else
	{
		// Otherwise, create one here.
		StaticMesh->CreateBodySetup();
		bs = StaticMesh->BodySetup;
	}

	// Calculate bounding sphere.
	FRawMesh RawMesh;
	FStaticMeshSourceModel& SrcModel = StaticMesh->SourceModels[0];
	SrcModel.RawMeshBulkData->LoadRawMesh(RawMesh);

	FSphere bSphere, bSphere2, bestSphere;
	FVector unitVec = FVector(1,1,1);
	CalcBoundingSphere(RawMesh, bSphere, unitVec);
	CalcBoundingSphere2(RawMesh, bSphere2, unitVec);

	if(bSphere.W < bSphere2.W)
		bestSphere = bSphere;
	else
		bestSphere = bSphere2;

	// Dont use if radius is zero.
	if(bestSphere.W <= 0.f)
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Prompt_10", "Could not create geometry.") );
		return;
	}

	int ex = bs->AggGeom.SphereElems.AddZeroed();
	FKSphereElem* s = &bs->AggGeom.SphereElems[ex];
	s->Center = bestSphere.Center;
	s->Radius = bestSphere.W;

	// Create new GUID
	bs->InvalidatePhysicsData();

	// refresh collision change back to staticmesh components
	RefreshCollisionChange(StaticMesh);

	// Mark staticmesh as dirty, to help make sure it gets saved.
	StaticMesh->MarkPackageDirty();
}


/**
 * Refresh Collision Change
 * 
 * Collision has been changed, so it will need to recreate physics state to reflect it
 * Utilities functions to propagate BodySetup change for StaticMesh
 *
 * @param	InStaticMesh	StaticMesh that collision has been changed for
 */
void RefreshCollisionChange(const UStaticMesh * InStaticMesh)
{
	for (FObjectIterator Iter(UStaticMeshComponent::StaticClass()); Iter; ++Iter)
	{
		UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(*Iter);
		if  (StaticMeshComponent->StaticMesh == InStaticMesh)
		{
			// it needs to recreate IF it already has been created
			if (StaticMeshComponent->IsPhysicsStateCreated())
			{
				StaticMeshComponent->RecreatePhysicsState();
			}
		}
	}

	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}
