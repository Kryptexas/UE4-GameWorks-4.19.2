// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=======================================================================================
	PhysXCollision.h: Collision related data structures/types specific to PhysX
=========================================================================================*/

#pragma once

#if WITH_PHYSX


/** Temporary result buffer size */
#define HIT_BUFFER_SIZE							512		// Hit buffer size for traces and sweeps. This is the total size allowed for sync + async tests.
#define HIT_BUFFER_MAX_SYNC_QUERIES				496		// Maximum number of queries to fill in the hit buffer for synchronous traces and sweeps. Async sweeps get the difference (HIT_BUFFER_SIZE - NumberOfHitsInSyncTest).
#define OVERLAP_BUFFER_SIZE						1024
#define OVERLAP_BUFFER_SIZE_MAX_SYNC_QUERIES	992

checkAtCompileTime(HIT_BUFFER_SIZE > 0, InvalidPhysXHitBufferSize);
checkAtCompileTime(HIT_BUFFER_MAX_SYNC_QUERIES < HIT_BUFFER_SIZE, InvalidPhysXSyncBufferSize);



/** 
 *	Macro for obtaining a specific geometry from the shape passed in
 *	NB. this macro is bad in terms of hiding the variable names, so made variable name to use Macro, so it does not overlap with the ones used outside 
 **/
#define GET_GEOMETRY_FROM_SHAPE( OutGeom, Shape ) \
	PxGeometry*				OutGeom = NULL; \
	PxSphereGeometry		PMacroSphereGeom;	\
	PxBoxGeometry			PMacroBoxGeom;		\
	PxCapsuleGeometry		PMacroCapsuleGeom;	\
	PxConvexMeshGeometry	PMacroConvexGeom;	\
	\
	if(Shape->getGeometryType() == PxGeometryType::eSPHERE)\
	{\
		Shape->getSphereGeometry(PMacroSphereGeom);\
		OutGeom = &PMacroSphereGeom;\
	}\
	else if(Shape->getGeometryType() == PxGeometryType::eBOX)\
	{\
		Shape->getBoxGeometry(PMacroBoxGeom);\
		OutGeom = &PMacroBoxGeom;\
	}\
	else if(Shape->getGeometryType() == PxGeometryType::eCAPSULE)\
	{\
		Shape->getCapsuleGeometry(PMacroCapsuleGeom);\
		OutGeom = &PMacroCapsuleGeom;\
	}\
	else if(Shape->getGeometryType() == PxGeometryType::eCONVEXMESH)\
	{\
		Shape->getConvexMeshGeometry(PMacroConvexGeom);\
		OutGeom = &PMacroConvexGeom;\
	}\
	else \
	{\
		OutGeom = NULL; \
	}


//TODO: update places that use macro with this function
struct GeometryFromShapeStorage
{
	PxSphereGeometry SphereGeom;
	PxBoxGeometry BoxGeom;
	PxCapsuleGeometry CapsuleGeom;
	PxConvexMeshGeometry ConvexGeom;
	PxTriangleMeshGeometry TriangleGeom;
};

PxGeometry * GetGeometryFromShape(GeometryFromShapeStorage & LocalStorage, const PxShape * PShape, bool bTriangleMeshAllowed = false);

// FILTER

/** Unreal PhysX scene query filter callback object */
class FPxQueryFilterCallback : public PxSceneQueryFilterCallback
{
public:
	/** List of ActorIds for this query to ignore */
	TArray<uint32, TInlineAllocator<1> >	IgnoreActors;
	PxSceneQueryHitType::Enum				PrefilterReturnValue;

	/** Whether this is a raycastSingle or sweepSingle, because in 3.3 you can't return eTOUCH for single queries */
	bool bSingleQuery;


	FPxQueryFilterCallback()
	{
		PrefilterReturnValue = PxSceneQueryHitType::eNONE;
		bSingleQuery = false;
	}

	FPxQueryFilterCallback(const TArray<uint32, TInlineAllocator<1> > InIgnoreActors)
	{
		PrefilterReturnValue = PxSceneQueryHitType::eNONE;
		IgnoreActors = InIgnoreActors;
		bSingleQuery = false;
	}

	/** 
	 * Calculate Result Query HitType from Query Filter and Shape Filter
	 * 
	 * @param PQueryFilter	: Querier FilterData
	 * @param PShapeFilter	: The Shape FilterData querier is testing against
	 *
	 * @return PxSceneQueryHitType from both FilterData
	 */
	static PxSceneQueryHitType::Enum CalcQueryHitType(const PxFilterData &PQueryFilter, const PxFilterData &PShapeFilter);
	
	virtual PxSceneQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxSceneQueryFlags& queryFlags) OVERRIDE;


	virtual PxSceneQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxSceneQueryHit& hit) OVERRIDE
	{
		// Currently not used
		return PxSceneQueryHitType::eBLOCK;
	}
};


class FPxQueryFilterCallbackSweep : public FPxQueryFilterCallback
{
public:
	bool DiscardInitialOverlaps;

	FPxQueryFilterCallbackSweep(const TArray<uint32, TInlineAllocator<1> > InIgnoreActors)
		: FPxQueryFilterCallback(InIgnoreActors)
	{
		DiscardInitialOverlaps = false;
	}

	virtual PxSceneQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxSceneQueryHit& hit) OVERRIDE;
};
// RAYCAST

/** Trace a ray against the world and return if a blocking hit is found */
bool RaycastTest(const UWorld * World, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** Trace a ray against the world and return the first blocking hit */
bool RaycastSingle(const UWorld * World, struct FHitResult& OutHit, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** 
 *  Trace a ray against the world and return touching hits and then first blocking hit
 *  Results are sorted, so a blocking hit (if found) will be the last element of the array
 *  Only the single closest blocking result will be generated, no tests will be done after that
 */
bool RaycastMulti(const UWorld * World, TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

// GEOM OVERLAP

/** Function for testing overlap between a supplied PxGeometry and the world. */
bool GeomOverlapTest(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** Function for testing overlap between a supplied PxGeometry and the world. */
bool GeomOverlapSingle(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, FOverlapResult& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** 
 * Function for overlapping a supplied PxGeometry against the world 
 * Note that this does not clear OutOverlaps, but adds unique results to it
 */
bool GeomOverlapMulti(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);


// GEOM SWEEP

/** Function used for sweeping a supplied PxGeometry against the world as a test */
bool GeomSweepTest(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** Function for sweeping a supplied PxGeometry against the world */
bool GeomSweepSingle(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, FHitResult& OutHit, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

/** Function for sweeping a supplied PxGeometry against the world */
bool GeomSweepMulti(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, TArray<FHitResult>& OutHits, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

// MISC

/** Convert from unreal to physx capsule rotation */
PxQuat ConvertToPhysXCapsuleRot(const FQuat& GeomRot);
/** Convert from physx to unreal capsule rotation */
FQuat ConvertToUECapsuleRot(const PxQuat & PGeomRot);
/** Convert from unreal to physx capsule pose */
PxTransform ConvertToPhysXCapsulePose(const FTransform& GeomPose);

// FILTER DATA

/** Utility for creating a PhysX PxFilterData for filtering query (trace) and sim (physics) from the Unreal filtering info. */
void CreateShapeFilterData(const uint8 MyChannel, const int32 ActorID, const FCollisionResponseContainer& ResponseToChannels, uint32 SkelMeshCompID, uint16 BodyIndex, PxFilterData& OutQueryData, PxFilterData& OutSimData, bool bEnableCCD, bool bEnableContactNotify, bool bStaticShape);

/** Utility for creating a PhysX PxFilterData for performing a query (trace) against the scene */
PxFilterData CreateQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer & InCollisionResponseContainer, const struct FCollisionObjectQueryParams & ObjectParam, const bool bMultitrace);

#endif // WITH_PHYX

