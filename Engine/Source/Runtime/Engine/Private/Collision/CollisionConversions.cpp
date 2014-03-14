// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_PHYSX
#include "../PhysicsEngine/PhysXSupport.h"
#include "PhysXCollision.h"
#include "CollisionDebugDrawing.h"

// Forward declare, I don't want to move the entire function right now or we lose change history.
static bool ConvertOverlappedShapeToImpactHit(const PxShape* PShape,  const PxRigidActor* PActor, const FVector& StartLoc, const FVector& EndLoc, FHitResult& OutResult, const PxGeometry& Geom, const PxTransform& QueryTM, const PxFilterData& QueryFilter, bool bReturnPhysMat, uint32 FaceIdx);


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
/* Validate Normal of OutResult. We're on hunt for invalid normal */
static void CheckHitResultNormal(const FHitResult& OutResult, const TCHAR* Message, const FVector & Start=FVector::ZeroVector, const FVector & End = FVector::ZeroVector, const PxGeometry* const Geom=NULL)
{
	if(!OutResult.bStartPenetrating && !OutResult.Normal.IsNormalized())
	{
		UE_LOG(LogPhysics, Warning, TEXT("(%s) Non-normalized OutResult.Normal from capsule conversion: %s (Component- %s)"), Message, *OutResult.Normal.ToString(), *GetNameSafe(OutResult.Component.Get()));
		// now I'm adding Outresult input
		UE_LOG(LogPhysics, Warning, TEXT("Start Loc(%s), End Loc(%s), Hit Loc(%s), ImpactNormal(%s)"), *Start.ToString(), *End.ToString(), *OutResult.Location.ToString(), *OutResult.ImpactNormal.ToString() );
		if (Geom != NULL)
		{
			// I only seen this crash on capsule
			if (Geom->getType() == PxGeometryType::eCAPSULE)
			{
				const PxCapsuleGeometry * Capsule = (PxCapsuleGeometry*)Geom;
				UE_LOG(LogPhysics, Warning, TEXT("Capsule radius (%f), Capsule Halfheight (%f)"), Capsule->radius, Capsule->halfHeight);
			}
		}
		check(OutResult.Normal.IsNormalized());
	}
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

/** 
 * Convert Convex Normal to Capsule Normal
 * For capsule, we'd like to get normal of capsule rather than convex, so the normal can be smooth when we move character
 * 
 * @param HalfHeight	: HalfHeight of the capsule
 * @param Radius		: Radius of the capsule
 * @param OutHit		: The result of hit from physX
 @ @param PointOnGeom	: If not null, use this point and the impact normal to recompute the impact point (which can yield a better normal).
 * @result OutHit.Normal is converted to Capsule Normal
 */
static void ConvertConvexNormalToCapsuleNormal(float HalfHeight, float Radius, struct FHitResult& OutHit, const FVector* PointOnGeom)
{
	if ( OutHit.Time > 0.f )
	{
		// Impact point is contact point where it meet 
		const float ContactZ = OutHit.ImpactPoint.Z;
		// Hit.Location is center of the object
		const FVector CenterOfCapsule = OutHit.Location; 
		// We're looking for end of the normal for this capsule
		FVector NormalEnd = CenterOfCapsule;

		// see if it's upper hemisphere 
		if (ContactZ > CenterOfCapsule.Z + HalfHeight)
		{
			// if upper hemisphere, the normal should point to the center of upper hemisphere
			NormalEnd.Z = CenterOfCapsule.Z + HalfHeight;
		}
		else if (ContactZ < CenterOfCapsule.Z - HalfHeight)
		{
			// if lower hemisphere, the normal should point to the center of lower hemisphere	
			NormalEnd.Z = CenterOfCapsule.Z - HalfHeight;
		}
		else
		{
			// otherwise, normal end is going to be same as contact point 
			// since it's the tube section of the capsule
			NormalEnd.Z = ContactZ;
		}

		// Recompute ImpactPoint if requested
		if (PointOnGeom)
		{
			const FPlane GeomPlane(*PointOnGeom, OutHit.ImpactNormal);
			const float DistToPlane = GeomPlane.PlaneDot(NormalEnd);
			if (DistToPlane >= Radius)
			{
				OutHit.ImpactPoint = NormalEnd - DistToPlane * OutHit.ImpactNormal;
			}
		}

		//DrawDebugLine(ContactPoint, NormalEnd, FColor(0, 255, 0), true);
		//DrawDebugLine(NormalEnd, CenterOfCapsule, FColor(0, 0, 255), true);

		FVector ContactNormal = (NormalEnd - OutHit.ImpactPoint);
		// if ContactNormal is not nearly zero, set it
		// otherwise use previous normal
		const float Size = ContactNormal.Size();
		if (Size >= KINDA_SMALL_NUMBER)
		{
			ContactNormal /= Size;
			OutHit.Normal = ContactNormal;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST || !WITH_EDITOR)
		CheckHitResultNormal(OutHit, TEXT("ConvertConvexNormalToCapsuleNormal"));
#endif
	}
}

static void ConvertConvexNormalToSphereNormal(float Radius, struct FHitResult& OutHit)
{
	if (OutHit.Time > 0.f)
	{
		FVector ContactNormal = (OutHit.Location - OutHit.ImpactPoint);
		const float Size = ContactNormal.Size();
		if (Size >= KINDA_SMALL_NUMBER)
		{
			ContactNormal /= Size;
			OutHit.Normal = ContactNormal;
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST || !WITH_EDITOR)
		CheckHitResultNormal(OutHit, TEXT("ConvertConvexNormalToSphereNormal"));
#endif
	}
}


/** Helper to transform a normal when non-uniform scale is present. */
// TODO: I'm sure there is a more efficient approach.
static void TransformNormalToShapeSpace(const PxMeshScale& meshScale, const PxVec3& nIn, PxVec3& nOut)
{
	// Uniform scale makes this unnecessary
	if (meshScale.scale.x == meshScale.scale.y &&
		meshScale.scale.x == meshScale.scale.z)
	{
		nOut = nIn;
		return;
	}

	PxMat33 R(meshScale.rotation);
	PxMat33 vertex2ShapeSkew = R.getTranspose();
	PxMat33 diagonal = PxMat33::createDiagonal(meshScale.scale);
	vertex2ShapeSkew = vertex2ShapeSkew * diagonal;
	vertex2ShapeSkew = vertex2ShapeSkew * R;

	PxMat33 shape2VertexSkew = vertex2ShapeSkew.getInverse();
	const PxVec3 tmp = shape2VertexSkew.transformTranspose(nIn);
	const PxReal Denom = 1.0f / tmp.magnitude();
	nOut = tmp * Denom;
}



/** Util to find the normal of the face that we hit */
static bool FindGeomOpposingNormal(const PxLocationHit& PHit, FVector& OutNormal, FVector& OutPointOnGeom)
{
	PxTriangleMeshGeometry PTriMeshGeom;
	PxConvexMeshGeometry PConvexMeshGeom;
	if(	PHit.shape->getTriangleMeshGeometry(PTriMeshGeom) && 
		PTriMeshGeom.triangleMesh != NULL &&
		PHit.faceIndex < PTriMeshGeom.triangleMesh->getNbTriangles() )
	{
		const int32 TriIndex = PHit.faceIndex;
		const void* Triangles = PTriMeshGeom.triangleMesh->getTriangles();

		// Grab triangle indices that we hit
		int32 I0, I1, I2;

		if(PTriMeshGeom.triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::eHAS_16BIT_TRIANGLE_INDICES) 
		{
			PxU16* P16BitIndices = (PxU16*)Triangles;
			I0 = P16BitIndices[(TriIndex*3)+0];
			I1 = P16BitIndices[(TriIndex*3)+1];
			I2 = P16BitIndices[(TriIndex*3)+2];
		}
		else
		{
			PxU32* P32BitIndices = (PxU32*)Triangles;
			I0 = P32BitIndices[(TriIndex*3)+0];
			I1 = P32BitIndices[(TriIndex*3)+1];
			I2 = P32BitIndices[(TriIndex*3)+2];
		}

		// Get verts we hit (local space)
		const PxVec3* PVerts = PTriMeshGeom.triangleMesh->getVertices();
		const PxVec3 V0 = PVerts[I0];
		const PxVec3 V1 = PVerts[I1];
		const PxVec3 V2 = PVerts[I2];

		// Find normal of triangle, and convert into world space
		PxVec3 PLocalTriNormal = ((V1 - V0).cross(V2 - V0)).getNormalized();
		TransformNormalToShapeSpace(PTriMeshGeom.scale, PLocalTriNormal, PLocalTriNormal);
	
		const PxTransform PShapeWorldPose = PxShapeExt::getGlobalPose(*PHit.shape, *PHit.actor); 

		const PxVec3 PWorldTriNormal = PShapeWorldPose.rotate(PLocalTriNormal);
		OutNormal = P2UVector(PWorldTriNormal);
		
		if (PTriMeshGeom.scale.isIdentity())
		{
			OutPointOnGeom = P2UVector(PShapeWorldPose.transform(V0));
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (!OutNormal.IsNormalized())
		{
			UE_LOG(LogPhysics, Warning, TEXT("Non-normalized Normal (Hit shape is TriangleMesh): %s (V0:%s, V1:%s, V2:%s)"), *OutNormal.ToString(), *P2UVector(V0).ToString(), *P2UVector(V1).ToString(), *P2UVector(V2).ToString());
			UE_LOG(LogPhysics, Warning, TEXT("WorldTransform \n: %s"), *P2UTransform(PShapeWorldPose).ToString());
		}
#endif
		return true;
	}
	else if(PHit.shape->getConvexMeshGeometry(PConvexMeshGeom) && 
		PConvexMeshGeom.convexMesh != NULL &&
		PHit.faceIndex < PConvexMeshGeom.convexMesh->getNbPolygons() )
	{
		const int32 PolyIndex = PHit.faceIndex;
		PxHullPolygon PPoly;
		bool bSuccess = PConvexMeshGeom.convexMesh->getPolygonData(PolyIndex, PPoly);
		if(bSuccess)
		{
			PxVec3 PPlaneNormal(PPoly.mPlane[0], PPoly.mPlane[1], PPoly.mPlane[2]);

			// Convert to local space, taking scale into account.
			// TODO: can we just change the hit normal within the physx code where we choose the most opposing normal, and avoid doing this here again?
			PxVec3 PLocalPolyNormal;
			TransformNormalToShapeSpace(PConvexMeshGeom.scale, PPlaneNormal.getNormalized(), PLocalPolyNormal);			
			const PxTransform PShapeWorldPose = PHit.actor->getGlobalPose() * PHit.shape->getLocalPose(); 

			const PxVec3 PWorldPolyNormal = PShapeWorldPose.rotate(PLocalPolyNormal);
			OutNormal = P2UVector(PWorldPolyNormal);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (!OutNormal.IsNormalized())
			{
				UE_LOG(LogPhysics, Warning, TEXT("Non-normalized Normal (Hit shape is ConvexMesh): %s (LocalPolyNormal:%s)"), *OutNormal.ToString(), *P2UVector(PLocalPolyNormal).ToString());
				UE_LOG(LogPhysics, Warning, TEXT("WorldTransform \n: %s"), *P2UTransform(PShapeWorldPose).ToString());
			}
#endif
			return true;
		}
	}

	return false;
}


/** Set info in the HitResult (Actor, Component, PhysMaterial, BoneName, Item) based on the supplied shape and face index */
static void SetHitResultFromShapeAndFaceIndex(const PxShape* PShape,  const PxRigidActor* PActor, const uint32 FaceIndex, FHitResult& OutResult, bool bReturnPhysMat)
{
	FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(PShape->userData);
	FDestructibleChunkInfo* ChunkInfo = FPhysxUserData::Get<FDestructibleChunkInfo>(PShape->userData);
	UPrimitiveComponent* PrimComp = FPhysxUserData::Get<UPrimitiveComponent>(PShape->userData);

	if ((BodyInst == NULL && ChunkInfo == NULL))
	{
		BodyInst = FPhysxUserData::Get<FBodyInstance>(PActor->userData);
		ChunkInfo = FPhysxUserData::Get<FDestructibleChunkInfo>(PActor->userData);
	}

	UPrimitiveComponent* OwningComponent = ChunkInfo != NULL ? ChunkInfo->OwningComponent.Get() : NULL;
	if (OwningComponent == NULL && BodyInst != NULL)
	{
		OwningComponent = BodyInst->OwnerComponent.Get();
	}

	// If the shape has a different parent component, we take that one instead of the ChunkInfo. This can happen in some 
	// cases where APEX moves shapes internally to another actor ( ex. FormExtended structures )
	if (PrimComp != NULL && OwningComponent != PrimComp)
	{
		OwningComponent = PrimComp;
	}

	OutResult.PhysMaterial = NULL;

	bool bReturnBody = false;

	// Grab actor/component
	if( OwningComponent )
	{
		OutResult.Actor = OwningComponent->GetOwner();
		OutResult.Component = OwningComponent;

		if(bReturnPhysMat)
		{
			// @fixme: only do this for InGameThread, otherwise, this will be done in AsyncTrace
			if ( IsInGameThread() )
			{
				// This function returns the single material in all cases other than trimesh or heightfield
				PxMaterial* PxMat = PShape->getMaterialFromInternalFaceIndex(FaceIndex);
				if(PxMat != NULL)
				{
					OutResult.PhysMaterial = FPhysxUserData::Get<UPhysicalMaterial>(PxMat->userData);
				}
			}
			else
			{
				//@fixme: this will be fixed properly when we can make FBodyInstance to be TWeakPtr - TTP (263842)
			}
		}

		bReturnBody = OwningComponent->bMultiBodyOverlap;
	}

	// For destructibles give the ChunkInfo-Index as Item
	if (bReturnBody && ChunkInfo)
	{
		OutResult.Item = ChunkInfo->Index;

		UDestructibleComponent* DMComp = Cast<UDestructibleComponent>(OwningComponent);
		OutResult.BoneName = DMComp->GetBoneName(UDestructibleComponent::ChunkIdxToBoneIdx(ChunkInfo->ChunkIndex));
	}
	// If BodyInstance and not destructible, give BodyIndex as Item
	else if (BodyInst)
	{
		OutResult.Item = BodyInst->InstanceBodyIndex;

		if (BodyInst->BodySetup.IsValid())
		{
			OutResult.BoneName = BodyInst->BodySetup->BoneName;
		}
	}
	else
	{
		// invalid index
		OutResult.Item = INDEX_NONE;
		OutResult.BoneName = NAME_None;
	}
}

void ConvertQueryImpactHit(const PxLocationHit& PHit, FHitResult& OutResult, float CheckLength, const PxFilterData& QueryFilter, const FVector& StartLoc, const FVector& EndLoc, const PxGeometry* const Geom, const PxTransform& QueryTM, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	if (Geom != NULL && PHit.hadInitialOverlap())
	{
		ConvertOverlappedShapeToImpactHit(PHit.shape, PHit.actor, StartLoc, EndLoc, OutResult, *Geom, QueryTM, QueryFilter, bReturnPhysMat, PHit.faceIndex);
		return;
	}

	SetHitResultFromShapeAndFaceIndex(PHit.shape,  PHit.actor, PHit.faceIndex, OutResult, bReturnPhysMat);

	// calculate the hit time
	const float HitTime = PHit.distance/CheckLength;

	// figure out where the the "safe" location for this shape is by moving from the startLoc toward the ImpactPoint
	const FVector TraceDir = EndLoc - StartLoc;

	const FVector SafeLocationToFitShape = StartLoc + (HitTime * TraceDir);

	// Other info
	OutResult.Location = SafeLocationToFitShape;
	OutResult.ImpactPoint = P2UVector(PHit.position);
	OutResult.Normal = P2UVector(PHit.normal);
	OutResult.ImpactNormal = OutResult.Normal;

	OutResult.TraceStart = StartLoc;
	OutResult.TraceEnd = EndLoc;
	OutResult.Time = HitTime;

	// See if this is a 'blocking' hit
	PxFilterData PShapeFilter = PHit.shape->getQueryFilterData();
	PxSceneQueryHitType::Enum HitType = FPxQueryFilterCallback::CalcQueryHitType(QueryFilter, PShapeFilter);
	OutResult.bBlockingHit = (HitType == PxSceneQueryHitType::eBLOCK); 

	OutResult.bStartPenetrating = (PxU32)(PHit.hadInitialOverlap());


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST || !WITH_EDITOR)
	CheckHitResultNormal(OutResult, TEXT("Invalid Normal from ConvertQueryImpactHit"), StartLoc, EndLoc, Geom);
#endif

	// Special handling for swept-capsule results
	if(Geom && Geom->getType() == PxGeometryType::eCAPSULE)
	{
		const PxCapsuleGeometry* Capsule = static_cast<const PxCapsuleGeometry*>(Geom);
		
		// Compute better ImpactNormal. This is the same as Normal except when we hit convex/trimesh, and then its the most opposing normal of the geom at point of impact.
		FVector PointOnGeom(P2UVector(PHit.position));
		if (FindGeomOpposingNormal(PHit, OutResult.ImpactNormal, PointOnGeom))
		{
			ConvertConvexNormalToCapsuleNormal(Capsule->halfHeight, Capsule->radius, OutResult, &PointOnGeom);
		}
		else
		{
			ConvertConvexNormalToCapsuleNormal(Capsule->halfHeight, Capsule->radius, OutResult, NULL);
			OutResult.ImpactNormal = OutResult.Normal;
		}
	}
	else if (Geom && Geom->getType() == PxGeometryType::eSPHERE)
	{
		const PxSphereGeometry* Sphere = static_cast<const PxSphereGeometry*>(Geom);

		// Compute better ImpactNormal. This is the same as Normal except when we hit convex/trimesh, and then its the most opposing normal of the geom at point of impact.
		FVector PointOnGeom(P2UVector(PHit.position));
		if (FindGeomOpposingNormal(PHit, OutResult.ImpactNormal, PointOnGeom))
		{
			const FPlane GeomPlane(PointOnGeom, OutResult.ImpactNormal);
			const float DistFromPlane = FMath::Abs(GeomPlane.PlaneDot(OutResult.Location));
			if (DistFromPlane >= Sphere->radius)
			{
				// Use the new (better) impact normal to compute a new impact point by projecting the sphere's location onto the geometry.
				OutResult.ImpactPoint = OutResult.Location - DistFromPlane * OutResult.ImpactNormal;
			}

			// Use the impact point to compute a better sphere surface normal (impact point to center of sphere).
			ConvertConvexNormalToSphereNormal(Sphere->radius, OutResult);
		}
		else
		{
			ConvertConvexNormalToSphereNormal(Sphere->radius, OutResult);
			OutResult.ImpactNormal = OutResult.Normal;
		}
	}
	
	if( PHit.shape->getGeometryType() == PxGeometryType::eHEIGHTFIELD)
	{
		// Lookup physical material for heightfields
		PxMaterial* HitMaterial = PHit.shape->getMaterialFromInternalFaceIndex(PHit.faceIndex);
		if( HitMaterial != NULL )
		{
			OutResult.PhysMaterial = FPhysxUserData::Get<UPhysicalMaterial>(HitMaterial->userData);
		}
	}
	else
	if(bReturnFaceIndex && PHit.shape->getGeometryType() == PxGeometryType::eTRIANGLEMESH)
	{
		PxTriangleMeshGeometry PTriMeshGeom;
		PxConvexMeshGeometry PConvexMeshGeom;
		if(	PHit.shape->getTriangleMeshGeometry(PTriMeshGeom) && 
			PTriMeshGeom.triangleMesh != NULL &&
			PHit.faceIndex < PTriMeshGeom.triangleMesh->getNbTriangles() )
		{
			OutResult.FaceIndex	= PTriMeshGeom.triangleMesh->getTrianglesRemap()[PHit.faceIndex];
		}
		else
		{
			OutResult.FaceIndex	= INDEX_NONE;
		}
	}
	else
	{
		OutResult.FaceIndex	= INDEX_NONE;
	}
}

struct FCompareFHitResultTime
{
	FORCEINLINE bool operator()( const FHitResult& A, const FHitResult& B ) const
	{
		if (A.Time == B.Time)
		{
			// Sort blocking hits after non-blocking hits, if they are at the same time. Also avoid swaps if they are the same.
			// This is important so initial touches are reported before processing stops on the first blocking hit.
			return (A.bBlockingHit == B.bBlockingHit) ? true : B.bBlockingHit;
		}

		return A.Time < B.Time;
	}
};

void ConvertRaycastResults(int32 NumHits, PxRaycastHit* Hits, float CheckLength, const PxFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	PxTransform PStartTM(U2PVector(StartLoc));
	for(int32 i=0; i<NumHits; i++)
	{
		FHitResult NewResult(ForceInit);
		const PxRaycastHit& PHit = Hits[i];

		ConvertQueryImpactHit(PHit, NewResult, CheckLength, QueryFilter, StartLoc, EndLoc, NULL, PStartTM, bReturnFaceIndex, bReturnPhysMat);

		OutHits.Add(NewResult);
	}

	// Sort results from first to last hit
	OutHits.Sort( FCompareFHitResultTime() );
}

bool AddSweepResults(int32 NumHits, PxSweepHit* Hits, float CheckLength, const PxFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const PxGeometry& Geom, const PxTransform& QueryTM, float MaxDistance, bool bReturnPhysMat)
{
	OutHits.Reserve(NumHits);
	bool bHadBlockingHit = false;

	for(int32 i=0; i<NumHits; i++)
	{
		const PxSweepHit& PHit = Hits[i];
		if(PHit.distance <= MaxDistance)
		{
			FHitResult NewResult(ForceInit);
			ConvertQueryImpactHit(PHit, NewResult, CheckLength, QueryFilter, StartLoc, EndLoc, &Geom, QueryTM, false, bReturnPhysMat);
			bHadBlockingHit |= NewResult.bBlockingHit;
			OutHits.Add(NewResult);
		}
	}

	// Sort results from first to last hit
	OutHits.Sort( FCompareFHitResultTime() );
	return bHadBlockingHit;
}

#define DRAW_OVERLAPPING_TRIS 0

/** Util to convert an overlapped shape into a sweep hit result, returns whether it was a blocking hit. */
static bool ConvertOverlappedShapeToImpactHit(const PxShape* PShape,  const PxRigidActor* PActor, const FVector& StartLoc, const FVector& EndLoc, FHitResult& OutResult, const PxGeometry& Geom, const PxTransform& QueryTM, const PxFilterData& QueryFilter, bool bReturnPhysMat, uint32 FaceIdx)
{
	OutResult.TraceStart = StartLoc;
	OutResult.TraceEnd = EndLoc;

	SetHitResultFromShapeAndFaceIndex(PShape, PActor, FaceIdx, OutResult, bReturnPhysMat);

	// Time of zero because initially overlapping
	OutResult.Time = 0.f;
	OutResult.bStartPenetrating = true;

	// See if this is a 'blocking' hit
	PxFilterData PShapeFilter = PShape->getQueryFilterData();
	PxSceneQueryHitType::Enum HitType = FPxQueryFilterCallback::CalcQueryHitType(QueryFilter, PShapeFilter);
	OutResult.bBlockingHit = (HitType == PxSceneQueryHitType::eBLOCK); 

	// Return start location as 'safe location'
	OutResult.Location = P2UVector(QueryTM.p);
	OutResult.ImpactPoint = OutResult.Location; // @todo not really sure of a better thing to do here...

	const PxTransform PShapeWorldPose = PxShapeExt::getGlobalPose(*PShape, *PActor); 


	PxTriangleMeshGeometry PTriMeshGeom;
	if(PShape->getTriangleMeshGeometry(PTriMeshGeom))
	{
		PxU32 HitTris[64];
		bool bOverflow = false;
		int32 NumTrisHit = PxMeshQuery::findOverlapTriangleMesh(Geom, QueryTM, PTriMeshGeom, PShapeWorldPose, HitTris, 64, 0, bOverflow);

#if DRAW_OVERLAPPING_TRIS
		TArray<FOverlapResult> Overlaps;
		DrawGeomOverlaps(World, Geom, QueryTM, Overlaps);

		TArray<FBatchedLine> Lines;
		const FLinearColor LineColor = FLinearColor(1.f,0.7f,0.7f);
		const FLinearColor NormalColor = FLinearColor(1.f,1.f,1.f);
		const float Lifetime = 5.f;
#endif // DRAW_OVERLAPPING_TRIS

		// Track the best triangle plane distance
		float BestPlaneDist = -BIG_NUMBER;
		FVector BestPlaneNormal(0,0,1);
		FVector BestPointOnPlane(0,0,0);
		// Iterate over triangles
		for(int32 TriIdx = 0; TriIdx<NumTrisHit; TriIdx++)
		{
			PxTriangle Tri;
			PxMeshQuery::getTriangle(PTriMeshGeom, PShapeWorldPose, HitTris[TriIdx], Tri);

			const FVector A = P2UVector(Tri.verts[0]);
			const FVector B = P2UVector(Tri.verts[1]);
			const FVector C = P2UVector(Tri.verts[2]);

			FVector TriNormal = ((B-A) ^ (C-A));

			// Use a more accurate normalization that avoids InvSqrtEst
			const float TriNormalSize = TriNormal.Size();
			TriNormal = (TriNormalSize >= KINDA_SMALL_NUMBER ? TriNormal/TriNormalSize : FVector::ZeroVector);

			const FPlane TriPlane(A, TriNormal);

			const FVector QueryCenter = P2UVector(QueryTM.p);
			const float DistToPlane = TriPlane.PlaneDot(QueryCenter);

			if(DistToPlane > BestPlaneDist)
			{
				BestPlaneDist = DistToPlane;
				BestPlaneNormal = TriNormal;
				BestPointOnPlane = A;
			}

#if DRAW_OVERLAPPING_TRIS
			Lines.Add(FBatchedLine(A, B, LineColor, Lifetime, 0.1f, SDPG_Foreground));
			Lines.Add(FBatchedLine(B, C, LineColor, Lifetime, 0.1f, SDPG_Foreground));
			Lines.Add(FBatchedLine(C, A, LineColor, Lifetime, 0.1f, SDPG_Foreground));
			Lines.Add(FBatchedLine(A, A+(50.f*TriNormal), NormalColor, Lifetime, 0.1f, SDPG_Foreground));
#endif // DRAW_OVERLAPPING_TRIS
		}

#if DRAW_OVERLAPPING_TRIS
		if ( World->PersistentLineBatcher )
		{
			World->PersistentLineBatcher->DrawLines(Lines);
		}
#endif // DRAW_OVERLAPPING_TRIS

		OutResult.ImpactNormal = BestPlaneNormal;
	}
	else
	{
		// use vector center of shape to query as good direction to move in
		PxGeometry& PGeom = PShape->getGeometry().any();
		PxVec3 PClosestPoint;
		float Distance = PxGeometryQuery::pointDistance(QueryTM.p, PGeom, PShapeWorldPose, &PClosestPoint);

		if(Distance < KINDA_SMALL_NUMBER)
		{
			//UE_LOG(LogCollision, Warning, TEXT("ConvertOverlappedShapeToImpactHit: Query origin inside shape, giving poor MTD."));			
			PClosestPoint = PxShapeExt::getWorldBounds(*PShape, *PActor).getCenter(); 
		}

		OutResult.ImpactNormal = (OutResult.Location - P2UVector(PClosestPoint)).SafeNormal();
	}

	// Compute depenetration vector and distance if possible.
	PxVec3 PxMtdNormal(0.f);
	PxF32 PxMtdDepth = 0.f;
	PxGeometry& POtherGeom = PShape->getGeometry().any();

	const bool bMtdResult = PxGeometryQuery::computePenetration(PxMtdNormal, PxMtdDepth, Geom, QueryTM, POtherGeom, PShapeWorldPose);
	if (bMtdResult)
	{
		const FVector MtdNormal = P2UVector(PxMtdNormal);
		OutResult.Normal = MtdNormal;
		OutResult.PenetrationDepth = FMath::Abs(PxMtdDepth) + KINDA_SMALL_NUMBER; // TODO: why are we getting negative values here from mtd sometimes?
	}
	else
	{
		OutResult.Normal = OutResult.ImpactNormal;
	}

	return OutResult.bBlockingHit;
}


void ConvertQueryOverlap(const PxShape* PShape, const PxRigidActor* PActor, FOverlapResult& OutOverlap, const PxFilterData& QueryFilter)
{
	// See if this is a 'blocking' hit
	PxFilterData PShapeFilter = PShape->getQueryFilterData();
	// word1 is block, word2 is overlap
	PxSceneQueryHitType::Enum HitType = FPxQueryFilterCallback::CalcQueryHitType(QueryFilter, PShapeFilter);
	bool bBlock = (HitType == PxSceneQueryHitType::eBLOCK); 

	FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(PActor->userData);
	FDestructibleChunkInfo* ChunkInfo = FPhysxUserData::Get<FDestructibleChunkInfo>(PActor->userData);

	// Grab actor/component
	if(BodyInst && BodyInst->OwnerComponent.IsValid())
	{
		OutOverlap.Actor = BodyInst->OwnerComponent->GetOwner();
		OutOverlap.Component = BodyInst->OwnerComponent;
		OutOverlap.ItemIndex = BodyInst->OwnerComponent->bMultiBodyOverlap ? BodyInst->InstanceBodyIndex : INDEX_NONE;
	}
	else if (ChunkInfo && ChunkInfo->OwningComponent.IsValid())
	{
		OutOverlap.Actor = ChunkInfo->OwningComponent->GetOwner();
		OutOverlap.Component = ChunkInfo->OwningComponent.Get();
		OutOverlap.ItemIndex = ChunkInfo->OwningComponent->bMultiBodyOverlap ? UDestructibleComponent::ChunkIdxToBoneIdx(ChunkInfo->ChunkIndex) : INDEX_NONE;
	}

	// Other info
	OutOverlap.bBlockingHit = bBlock;
}

/** Util to add NewOverlap to OutOverlaps if it is not already there */
static void AddUniqueOverlap(TArray<FOverlapResult>& OutOverlaps, const FOverlapResult& NewOverlap)
{
	// Look to see if we already have this overlap (based on component)
	for(int32 TestIdx=0; TestIdx<OutOverlaps.Num(); TestIdx++)
	{
		FOverlapResult& Overlap = OutOverlaps[TestIdx];

		if(Overlap.Component == NewOverlap.Component && Overlap.ItemIndex == NewOverlap.ItemIndex)
		{
			// These should be the same if the component matches!
			check(Overlap.Actor == NewOverlap.Actor);

			// If we had a non-blocking overlap with this component, but now we have a blocking one, use that one instead!
			if(!Overlap.bBlockingHit && NewOverlap.bBlockingHit)
			{
				Overlap = NewOverlap;
			}

			return;
		}
	}

	// Not found, so add it 
	OutOverlaps.Add(NewOverlap);
}

bool ConvertOverlapResults(int32 NumOverlaps, PxOverlapHit* POverlapResults, const PxFilterData& QueryFilter, TArray<FOverlapResult>& OutOverlaps)
{
	OutOverlaps.Empty();

	bool bBlockingFound = false;

	for(int32 i=0; i<NumOverlaps; i++)
	{
		FOverlapResult NewOverlap;		
		
		ConvertQueryOverlap( POverlapResults[i].shape, POverlapResults[i].actor, NewOverlap, QueryFilter);


		if(NewOverlap.bBlockingHit)
		{
			bBlockingFound = true;
		}

		AddUniqueOverlap(OutOverlaps, NewOverlap);
	}

	return bBlockingFound;
}


#endif // WITH_PHYSX
