// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.h"
#include "BodySetup2D.generated.h"





/** Container for an aggregate of collision shapes */
USTRUCT()
struct FAggregateGeometry2D
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, editfixedsize, Category = KAggregateGeom)
	TArray<struct FKSphereElem> SphereElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category = KAggregateGeom)
	TArray<struct FKBoxElem> BoxElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category = KAggregateGeom)
	TArray<struct FKSphylElem> SphylElems;

	UPROPERTY(EditAnywhere, editfixedsize, Category = KAggregateGeom)
	TArray<struct FKConvexElem> ConvexElems;

	//class FKConvexGeomRenderInfo* RenderInfo;

	FAggregateGeometry2D()
//		: RenderInfo(NULL)
	{}

// 	int32 GetElementCount() const
// 	{
// 		return SphereElems.Num() + SphylElems.Num() + BoxElems.Num() + ConvexElems.Num();
// 	}
// 
// 	int32 GetElementCount(int32 Type) const;
// 
	void EmptyElements()
	{
		BoxElems.Empty();
		ConvexElems.Empty();
		SphylElems.Empty();
		SphereElems.Empty();

//		FreeRenderInfo();
	}
// 
// 
// 
// 	void Serialize(const FArchive& Ar);
// 
// 	void DrawAggGeom(class FPrimitiveDrawInterface* PDI, const FTransform& Transform, const FColor Color, const FMaterialRenderProxy* MatInst, bool bPerHullColor, bool bDrawSolid);
// 
// 	/** Release the RenderInfo (if its there) and safely clean up any resources. Call on the game thread. */
// 	void FreeRenderInfo();
// 
// 	FBox CalcAABB(const FTransform& Transform);
// 
// 	/**
// 	* Calculates a tight box-sphere bounds for the aggregate geometry; this is more expensive than CalcAABB
// 	* (tight meaning the sphere may be smaller than would be required to encompass the AABB, but all individual components lie within both the box and the sphere)
// 	*
// 	* @param Output The output box-sphere bounds calculated for this set of aggregate geometry
// 	*	@param LocalToWorld Transform
// 	*/
// 	void CalcBoxSphereBounds(FBoxSphereBounds& Output, const FTransform& LocalToWorld);
// 
// 	/** Returns the volume of this element */
// 	float GetVolume(const FVector& Scale) const;
};



UCLASS(DependsOn=UEngineTypes)
class PAPER2D_API UBodySetup2D : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FAggregateGeometry2D AggGeom2D;

	/** Default properties of the body instance, copied into objects on instantiation */
	UPROPERTY(EditAnywhere, Category=Collision, meta=(FullyExpand = "true"))
	struct FBodyInstance2D DefaultInstance;

	/** Create Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX) from cooked data */
	void CreatePhysicsMeshes();


#if WITH_EDITOR
	/** Release Physics meshes (ConvexMeshes, TriMesh & TriMeshNegX). Must be called before the BodySetup is destroyed */
	void InvalidatePhysicsData();
#endif

protected:
	/** Physics triangle mesh, created from cooked data in CreatePhysicsMeshes */
	//class PxTriangleMesh* TriMesh;

};
