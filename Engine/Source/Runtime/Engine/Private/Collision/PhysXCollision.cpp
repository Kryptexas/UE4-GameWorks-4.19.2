// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldCollision.cpp: UWorld collision implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Collision.h"

#if WITH_PHYSX

#include "../PhysicsEngine/PhysXSupport.h"
#include "PhysXCollision.h"
#include "CollisionDebugDrawing.h"
#include "CollisionConversions.h"

#if ENABLE_COLLISION_ANALYZER
#include "CollisionAnalyzerModule.h"
#endif

#define VERIFY_GEOMSWEEPSINGLE 0
#define VERIFY_GEOMSWEEPMULTI 0

static float DebugLineLifetime = 2.f;

/* 
 * Type of query for object type or trace type
 * Trace queries correspond to trace functions with TravelChannel/ResponseParams
 * Object queries correspond to trace functions with Object types 
 */
namespace ECollisionQuery
{
	enum Type
	{
		ObjectQuery=0,
		TraceQuery=1
	};
}

#define TRACE_MULTI		1
#define TRACE_SINGLE	0

PxSceneQueryHitType::Enum FPxQueryFilterCallback::CalcQueryHitType(const PxFilterData &PQueryFilter, const PxFilterData &PShapeFilter)
{
	ECollisionQuery::Type QueryType = (ECollisionQuery::Type)PQueryFilter.word0;
	ECollisionChannel const ShapeChannel = (ECollisionChannel)(PShapeFilter.word3 >> 24);
	PxU32 const ShapeBit = ECC_TO_BITFIELD(ShapeChannel);
	if (QueryType == ECollisionQuery::ObjectQuery)
	{
		int32 const MultiTrace = (PQueryFilter.word3 >> 24);
		// do I belong to one of objects of interest?
		if ( ShapeBit & PQueryFilter.word1 )
		{
			if (MultiTrace)
			{
				return PxSceneQueryHitType::eTOUCH;
			}
			else
			{
				return PxSceneQueryHitType::eBLOCK;
			}
		}
	}
	else
	{
		// Then see if the channel wants to be blocked
		ECollisionChannel const QuerierChannel = (ECollisionChannel)(PQueryFilter.word3 >> 24);

		// @todo delete once we fix up object/trace APIs to work separate
		PxU32 ShapeFlags = PShapeFilter.word3 & 0xFFFFFF;
		bool bStaticShape = ((ShapeFlags & EPDF_StaticShape) != 0);

		// if query channel is Touch All, then just return touch
		if (QuerierChannel == ECC_OverlapAll_Deprecated)
		{
			return PxSceneQueryHitType::eTOUCH;
		}
		else if (QuerierChannel == ECC_OverlapAllDynamic_Deprecated)
		{
			return bStaticShape ? PxSceneQueryHitType::eNONE : PxSceneQueryHitType::eTOUCH;
		}
		else if (QuerierChannel == ECC_OverlapAllStatic_Deprecated)
		{
			return !bStaticShape ? PxSceneQueryHitType::eNONE : PxSceneQueryHitType::eTOUCH;
		}
		// @todo delete once we fix up object/trace APIs to work separate

		PxU32 const QuerierBit = ECC_TO_BITFIELD(QuerierChannel);
		PxSceneQueryHitType::Enum QuerierHitType = PxSceneQueryHitType::eNONE;
		PxSceneQueryHitType::Enum ShapeHitType = PxSceneQueryHitType::eNONE;

		// check if Querier wants a hit
		if ((QuerierBit & PShapeFilter.word1) != 0)
		{
			QuerierHitType = PxSceneQueryHitType::eBLOCK;
		}
		else if ((QuerierBit & PShapeFilter.word2)!=0)
		{
			QuerierHitType = PxSceneQueryHitType::eTOUCH;
		}

		if ((ShapeBit & PQueryFilter.word1) != 0)
		{
			ShapeHitType = PxSceneQueryHitType::eBLOCK;
		}
		else if ((ShapeBit & PQueryFilter.word2)!=0)
		{
			ShapeHitType = PxSceneQueryHitType::eTOUCH;
		}

		// return minimum agreed-upon interaction
		return FMath::Min(QuerierHitType, ShapeHitType);
	}

	return PxSceneQueryHitType::eNONE;
}

PxSceneQueryHitType::Enum FPxQueryFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape,    const PxRigidActor* actor, PxSceneQueryFlags& queryFlags)
{
	// Check if the shape is the right compexity for the trace 
	PxFilterData ShapeFilter = shape->getQueryFilterData();
#define ENABLE_PREFILTER_LOGGING 0
#if ENABLE_PREFILTER_LOGGING
	static bool bLoggingEnabled=false;
	if (bLoggingEnabled)
	{
		FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(actor->userData);
		if ( BodyInst && BodyInst->OwnerComponent.IsValid() )
		{
			UE_LOG(LogCollision, Log, TEXT("[PREFILTER] against %s[%s] : About to check "), 
				(BodyInst->OwnerComponent.Get()->GetOwner())? *BodyInst->OwnerComponent.Get()->GetOwner()->GetName():TEXT("NO OWNER"),
				*BodyInst->OwnerComponent.Get()->GetName());
		}

		UE_LOG(LogCollision, Log, TEXT("ShapeFilter : %x %x %x %x"), ShapeFilter.word0, ShapeFilter.word1, ShapeFilter.word2, ShapeFilter.word3);
		UE_LOG(LogCollision, Log, TEXT("FilterData : %x %x %x %x"), filterData.word0, filterData.word1, filterData.word2, filterData.word3);
	}
#endif // ENABLE_PREFILTER_LOGGING

	// See if we are ignoring the actor this shape belongs to (word0 of shape filterdata is actorID)
	if(IgnoreActors.Contains(ShapeFilter.word0))
	{
		//UE_LOG(LogTemp, Log, TEXT("Ignoring Actor: %d"), ShapeFilter.word0);
		return (PrefilterReturnValue = PxSceneQueryHitType::eNONE);
	}

	// Shape : shape's Filter Data
	// Querier : filterData that owns the trace
	PxU32 ShapeFlags = ShapeFilter.word3 & 0xFFFFFF;
	PxU32 QuerierFlags = filterData.word3 & 0xFFFFFF;
	PxU32 CommonFlags = ShapeFlags & QuerierFlags;

	// First check complexity, none of them matches
	if(!(CommonFlags & EPDF_SimpleCollision) && !(CommonFlags & EPDF_ComplexCollision))
	{
		return (PrefilterReturnValue = PxSceneQueryHitType::eNONE);
	}

	PxSceneQueryHitType::Enum Result = FPxQueryFilterCallback::CalcQueryHitType(filterData, ShapeFilter);

	if(bSingleQuery && Result == PxSceneQueryHitType::eTOUCH)
	{
		Result = PxSceneQueryHitType::eNONE; // return eNONE for raycastSingle/sweepSingle as it's meaningless to return eTOUCH for those
	}

#if ENABLE_PREFILTER_LOGGING
	if (bLoggingEnabled)
	{
		FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(actor->userData);
		if ( BodyInst && BodyInst->OwnerComponent.IsValid() )
		{
			ECollisionChannel QuerierChannel = (ECollisionChannel)(filterData.word3 >> 24);
			UE_LOG(LogCollision, Log, TEXT("[PREFILTER] Result for Querier [CHANNEL: %d, FLAG: %x] %s[%s] [%d]"), 
				(int32)QuerierChannel, QuerierFlags,
				(BodyInst->OwnerComponent.Get()->GetOwner())? *BodyInst->OwnerComponent.Get()->GetOwner()->GetName():TEXT("NO OWNER"),
				*BodyInst->OwnerComponent.Get()->GetName(), (int32)Result);
		}
	}
#endif // ENABLE_PREFILTER_LOGGING

	return  (PrefilterReturnValue = Result);
}


PxSceneQueryHitType::Enum FPxQueryFilterCallbackSweep::postFilter(const PxFilterData& filterData, const PxSceneQueryHit& hit)
{
	PxSweepHit& SweepHit = (PxSweepHit&)hit;
	const bool bIsOverlap = SweepHit.hadInitialOverlap();

	if (DiscardInitialOverlaps && bIsOverlap)
	{
		return PxSceneQueryHitType::eNONE;
	}
	else
	{
		if (PrefilterReturnValue == PxSceneQueryHitType::eBLOCK && bIsOverlap)
		{
			// We want to keep initial blocking overlaps and continue the sweep until a non-overlapping blocking hit.
			// We will later report this hit as a blocking hit when we compute the hit type (using FPxQueryFilterCallback::CalcQueryHitType).
			return PxSceneQueryHitType::eTOUCH;
		}
		
		return PrefilterReturnValue;
	}
}


//////////////////////////////////////////////////////////////////////////
#if ENABLE_COLLISION_ANALYZER

bool GCollisionAnalyzerIsRecording = false;

bool bSkipCapture = false;

/** Util to extract type and dimensions from physx geom being swept, and pass info to CollisionAnalyzer, if its recording */
void CaptureGeomSweep(const UWorld* World, const FVector& Start, const FVector& End, const PxQuat& PRot, ECAQueryType::Type QueryType, const PxGeometry& PGeom, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TArray<FHitResult>& Results, double CPUTime)
{
	if(bSkipCapture || !IsInGameThread() || !GCollisionAnalyzerIsRecording)
	{
		return;
	}

	ECAQueryShape::Type QueryShape = ECAQueryShape::Raycast;
	FVector Dims(0,0,0);
	FQuat Rot = FQuat::Identity;

	switch(PGeom.getType())
	{
	case PxGeometryType::eCAPSULE:
		{
			QueryShape = ECAQueryShape::CapsuleSweep;
			PxCapsuleGeometry* PCapsuleGeom = (PxCapsuleGeometry*)&PGeom;
			Dims = FVector(PCapsuleGeom->radius, PCapsuleGeom->radius, PCapsuleGeom->halfHeight + PCapsuleGeom->radius);
			Rot = ConvertToUECapsuleRot(PRot);
			break;
		}

	case PxGeometryType::eSPHERE:
		{
			QueryShape = ECAQueryShape::SphereSweep;
			PxSphereGeometry* PSphereGeom = (PxSphereGeometry*)&PGeom;
			Dims = FVector(PSphereGeom->radius);
			break;
		}

	case PxGeometryType::eBOX:
		{
			QueryShape = ECAQueryShape::BoxSweep;
			PxBoxGeometry* PBoxGeom = (PxBoxGeometry*)&PGeom;
			Dims = P2UVector(PBoxGeom->halfExtents);
			Rot = P2UQuat(PRot);
			break;
		}

	case PxGeometryType::eCONVEXMESH:
		{
			QueryShape = ECAQueryShape::ConvexSweep;
			break;
		}

	default:
		UE_LOG(LogCollision, Warning, TEXT("CaptureGeomSweep: Unknown geom type."));
	}

	// Do a touch all query to find things we _didn't_ hit
	bSkipCapture = true;
	TArray<FHitResult> TouchAllResults;
	GeomSweepMulti(World, PGeom, PRot, TouchAllResults, Start, End, DefaultCollisionChannel, Params, ResponseParams, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));
	bSkipCapture = false;

	// Now tell analyzer
	FCollisionAnalyzerModule::Get()->CaptureQuery(Start, End, Rot, QueryType, QueryShape, Dims, TraceChannel, Params, ResponseParams, ObjectParams, Results, TouchAllResults, CPUTime);
}

/** Util to capture a raycast with the CollisionAnalyzer if recording */
void CaptureRaycast(const UWorld* World, const FVector& Start, const FVector& End, ECAQueryType::Type QueryType, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TArray<FHitResult>& Results, double CPUTime)
{
	if(bSkipCapture || !IsInGameThread() || !GCollisionAnalyzerIsRecording)
	{
		return;
	}

	// Do a touch all query to find things we _didn't_ hit
	bSkipCapture = true;
	TArray<FHitResult> TouchAllResults;
	RaycastMulti(World, TouchAllResults, Start, End, DefaultCollisionChannel, Params, ResponseParams, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));
	bSkipCapture = false;

	FCollisionAnalyzerModule::Get()->CaptureQuery(Start, End, FQuat::Identity, QueryType, ECAQueryShape::Raycast, FVector(0,0,0), TraceChannel, Params, ResponseParams, ObjectParams, Results, TouchAllResults, CPUTime);
}

#define STARTQUERYTIMER() double StartTime = FPlatformTime::Seconds()
#define CAPTUREGEOMSWEEP(World, Start, End, Rot, QueryType, PGeom, TraceChannel, Params, ResponseParam, ObjectParam, Results)	do { CaptureGeomSweep(World, Start, End, Rot, QueryType, PGeom, TraceChannel, Params, ResponseParam, ObjectParam, Results, FPlatformTime::Seconds() - StartTime); } while(0)
#define CAPTURERAYCAST(World, Start, End, QueryType, TraceChannel, Params, ResponseParam, ObjectParam, Results)	do { CaptureRaycast(World, Start, End, QueryType, TraceChannel, Params, ResponseParam, ObjectParam, Results, FPlatformTime::Seconds() - StartTime); } while(0)

#else

#define STARTQUERYTIMER() 
#define CAPTUREGEOMSWEEP(...)
#define CAPTURERAYCAST(...)

#endif // ENABLE_COLLISION_ANALYZER


//////////////////////////////////////////////////////////////////////////
// RAYCAST

bool RaycastTest(const UWorld * World, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastAny);
	STARTQUERYTIMER();

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits

	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
		FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);

		FPhysScene* PhysScene = World->GetPhysicsScene();

		// Enable scene locks, in case they are required
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);
		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		const PxVec3 PDir = U2PVector(Delta/DeltaMag);

		PxSceneQueryHit PQueryHit;
		bHaveBlockingHit = SyncScene->raycastAny(U2PVector(Start), PDir, DeltaMag, PQueryHit, PQueryFilterData, &PQueryCallback);

		// Test async scene if we have no blocking hit, and async tests are requested
		if( !bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);
			bHaveBlockingHit = AsyncScene->raycastAny(U2PVector(Start), PDir, DeltaMag, PQueryHit, PQueryFilterData, &PQueryCallback);
		}
	}

	TArray<FHitResult> Hits;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawLineTraces(World, Start, End, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	CAPTURERAYCAST(World, Start, End, ECAQueryType::Test, TraceChannel, Params, ResponseParams, ObjectParams, Hits);

	return bHaveBlockingHit;
}

bool RaycastSingle(const UWorld * World, struct FHitResult& OutHit, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastSingle);
	STARTQUERYTIMER();

	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits

	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
		PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::ePOSITION | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;
		FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);
		PQueryCallback.bSingleQuery = true; // this needs to be set for raycastSingle so we return eNONE instead of eTOUCH

		PxVec3 PStart = U2PVector(Start);
		PxVec3 PDir = U2PVector(Delta/DeltaMag);

		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// Enable scene locks, in case they are required
		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		PxRaycastHit PHit;
		bHaveBlockingHit = SyncScene->raycastSingle(U2PVector(Start), PDir, DeltaMag, POutputFlags, PHit, PQueryFilterData, &PQueryCallback);

		// Test async scene if async tests are requested
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene() )
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			bool bHaveBlockingHitAsync;
			PxRaycastHit PHitAsync;
			bHaveBlockingHitAsync = AsyncScene->raycastSingle(U2PVector(Start), PDir, DeltaMag, POutputFlags, PHitAsync, PQueryFilterData, &PQueryCallback);

			// If we have a blocking hit in the async scene and there was no sync blocking hit, or if the async blocking hit came first,
			// then this becomes the blocking hit.  We can test the PxSceneQueryImpactHit::distance since the DeltaMag is the same for both queries.
			if( bHaveBlockingHitAsync && (!bHaveBlockingHit || PHitAsync.distance < PHit.distance) )
			{
				PHit = PHitAsync;
				bHaveBlockingHit = true;
			}
		}

		if(bHaveBlockingHit) // If we got a hit, 
		{
			PxTransform PStartTM(U2PVector(Start));
			ConvertQueryImpactHit(PHit, OutHit, DeltaMag, PFilter, Start, End, NULL, PStartTM, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial);
		}
	}


#if ENABLE_COLLISION_ANALYZER || !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TArray<FHitResult> Hits;
	if (bHaveBlockingHit)
	{
		Hits.Add(OutHit);
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawLineTraces(World, Start, End, Hits, DebugLineLifetime);	
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTURERAYCAST(World, Start, End, ECAQueryType::Single, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
#endif

	return bHaveBlockingHit;
}

bool RaycastMulti(const UWorld * World, TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastMultiple);
	STARTQUERYTIMER();

	OutHits.Reset();

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, true);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
		PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::ePOSITION | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;
		FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);

		PxRaycastHit PHits[HIT_BUFFER_SIZE];
		bool bBlockingHit = false;
		const PxVec3 PDir = U2PVector(Delta/DeltaMag);

		// Enable scene locks, in case they are required
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		PxI32 NumHits = SyncScene->raycastMultiple(U2PVector(Start), PDir, DeltaMag, POutputFlags, PHits, HIT_BUFFER_MAX_SYNC_QUERIES, bBlockingHit, PQueryFilterData, &PQueryCallback);

		if(NumHits == -1)
		{
			UE_LOG(LogCollision, Warning, TEXT("RaycastMulti : Buffer overflow from synchronous scene query"));
			UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
			UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
			// raycastMultiple still fills in the buffer with an arbitrary set of overlapping objects, so we have a full buffer of results
			// we can use, though that is not the entire set
			NumHits = HIT_BUFFER_MAX_SYNC_QUERIES;
		}

		// Test async scene if async tests are requested and there was no overflow
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			// Write into the same PHits buffer
			PxRaycastHit* PHitsAsync = PHits+NumHits;
			const PxI32 AsyncBufferSize = (ARRAY_COUNT(PHits) - NumHits);
			check(AsyncBufferSize > 0);
			bool bBlockingHitAsync = false;

			// If we have a blocking hit from the sync scene, there is no need to raycast past that hit
			const float RayLength = bBlockingHit ? PHits[NumHits-1].distance : DeltaMag;

			PxI32 NumAsyncHits = 0;
			if(RayLength > SMALL_NUMBER) // don't bother doing the trace if the sync scene trace gave a hit time of zero
			{
				NumAsyncHits = AsyncScene->raycastMultiple(U2PVector(Start), PDir, RayLength, POutputFlags, PHitsAsync, AsyncBufferSize, bBlockingHitAsync, PQueryFilterData, &PQueryCallback);
			}

			if(NumAsyncHits == -1)
			{
				UE_LOG(LogCollision, Log, TEXT("RaycastMulti : Buffer overflow from asyncrhonous scene query"));
				UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
				// raycastMultiple still fills in the buffer with an arbitrary set of overlapping objects, so we have a full buffer of results
				// we can use, though that is not the entire set
				NumAsyncHits = AsyncBufferSize;
			}

			PxI32 TotalNumHits = NumHits + NumAsyncHits;

			// If there is a blocking hit in the sync scene, and it is closer than the blocking hit in the async scene (or there is no blocking hit in the async scene),
			// then move it to the end of the array to get it out of the way.
			if (bBlockingHit && NumAsyncHits > 0)
			{
				if (!bBlockingHitAsync || PHits[NumHits-1].distance < PHits[TotalNumHits-1].distance)
				{
					PHits[TotalNumHits-1] = PHits[NumHits-1];
				}
			}

			// Merge results
			NumHits = TotalNumHits;

			bBlockingHit = bBlockingHit || bBlockingHitAsync;

			// Now eliminate hits which are farther than the nearest blocking hit, or even those that are the exact same distance as the blocking hit,
			// to ensure the blocking hit is the last in the array after sorting in ConvertRaycastResults (below).
			if (bBlockingHit)
			{
				const PxF32 MaxDistance = PHits[NumHits-1].distance;
				PxI32 TestHitCount = NumHits-1;
				for (PxI32 HitNum = TestHitCount; HitNum-- > 0;)
				{
					if (PHits[HitNum].distance >= MaxDistance)
					{
						PHits[HitNum] = PHits[--TestHitCount];
					}
				}
				if (TestHitCount < NumHits-1)
				{
					PHits[TestHitCount] = PHits[NumHits-1];
					NumHits = TestHitCount + 1;
				}
			}
		}

		if(NumHits > 0)
		{
			ConvertRaycastResults(NumHits, PHits, DeltaMag, PFilter, OutHits, Start, End, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial);
		}

		bHaveBlockingHit = bBlockingHit;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawLineTraces(World, Start, End, OutHits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTURERAYCAST(World, Start, End, ECAQueryType::Multi, TraceChannel, Params, ResponseParams, ObjectParams, OutHits);

	return bHaveBlockingHit;
}

//////////////////////////////////////////////////////////////////////////
// GEOM SWEEP

bool GeomSweepTest(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepAny);
	STARTQUERYTIMER();

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits

	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER | PxSceneQueryFilterFlag::ePOSTFILTER);
		PxSceneQueryFlags POutputFlags; 

		FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params.IgnoreActors);
		PQueryCallbackSweep.DiscardInitialOverlaps = !Params.bFindInitialOverlaps;

		PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxVec3 PDir = U2PVector(Delta/DeltaMag);

		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// Enable scene locks, in case they are required
		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		PxSceneQueryHit PQueryHit;
		bHaveBlockingHit = SyncScene->sweepAny(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PQueryHit, PQueryFilterData, &PQueryCallbackSweep);

		// Test async scene if async tests are requested and there was no blocking hit was found in the sync scene (since no hit info other than a boolean yes/no is recorded)
		if( !bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			bHaveBlockingHit = AsyncScene->sweepAny(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PQueryHit, PQueryFilterData, &PQueryCallbackSweep);
		}
	}

	TArray<FHitResult> Hits;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryType::Test, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, Hits);

	return bHaveBlockingHit;
}

bool GeomSweepSingle(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, FHitResult& OutHit, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepSingle);
	STARTQUERYTIMER();

	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	const FVector Delta = End - Start;
	const float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
		//UE_LOG(LogCollision, Log, TEXT("PFilter: %x %x %x %x"), PFilter.word0, PFilter.word1, PFilter.word2, PFilter.word3);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
		PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::ePOSITION | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;
		FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params.IgnoreActors);
		PQueryCallbackSweep.bSingleQuery = true; //LOC_MOD need to do this to return eNONE instead of eTOUCH for r
		PQueryCallbackSweep.DiscardInitialOverlaps = !Params.bFindInitialOverlaps;	

		PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxVec3 PDir = U2PVector(Delta/DeltaMag);

		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// Enable scene locks, in case they are required
		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		PxSweepHit PHit;
		bHaveBlockingHit = SyncScene->sweepSingle(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PHit, PQueryFilterData, &PQueryCallbackSweep);

#if VERIFY_GEOMSWEEPSINGLE
		// If we hit but did not start overlapping...
		if(bHaveBlockingHit && !PHit.hadInitialOverlap())
		{
			const float HitTime = PHit.distance/DeltaMag;
			FVector HitLocation = Start + (Delta/DeltaMag) * HitTime;
			PxTransform PEndTM(U2PVector(HitLocation), PGeomRot);

			PxSweepHit PTestHit;
			bool bTestBlockingHit = SyncScene->sweepSingle(PGeom, PEndTM, -PDir, DeltaMag, POutputFlags, PTestHit, PQueryFilterData, &PQueryCallbackSweep);
			if(bTestBlockingHit)
			{
				UE_LOG(LogCollision, Warning, TEXT("GeomSweepSingle : Cannot sweep back from end location."));

				PxSweepHit PReproHit;
				bool bReproBlockingHit = SyncScene->sweepSingle(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PReproHit, PQueryFilterData, &PQueryCallbackSweep);
			}
		}
#endif

		// Test async scene if async tests are requested
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			bool bHaveBlockingHitAsync;
			PxSweepHit PHitAsync;
			bHaveBlockingHitAsync = AsyncScene->sweepSingle(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PHitAsync, PQueryFilterData, &PQueryCallbackSweep);

			// If we have a blocking hit in the async scene and there was no sync blocking hit, or if the async blocking hit came first,
			// then this becomes the blocking hit.  We can test the PxSceneQueryImpactHit::distance since the DeltaMag is the same for both queries.
			if( bHaveBlockingHitAsync && (!bHaveBlockingHit || PHitAsync.distance < PHit.distance) )
			{
				PHit = PHitAsync;
				bHaveBlockingHit = true;
			}
		}

		if(bHaveBlockingHit) // If we got a hit, convert it to unreal type
		{
			ConvertQueryImpactHit(PHit, OutHit, DeltaMag, PFilter, Start, End, &PGeom, PStartTM, false, Params.bReturnPhysicalMaterial);
		}
	}


#if ENABLE_COLLISION_ANALYZER || !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TArray<FHitResult> Hits;
	if (bHaveBlockingHit)
	{
		Hits.Add(OutHit);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryType::Single, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
#endif

	return bHaveBlockingHit;
}


bool GeomSweepMulti(const UWorld * World, const PxGeometry& PGeom, const PxQuat& PGeomRot, TArray<FHitResult>& OutHits, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepMultiple);
	STARTQUERYTIMER();

	OutHits.Reset();

	// Track if we get any 'blocking' hits
	bool bBlockingHit = false;

	// Create filter data used to filter collisions
	PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, true);
	PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER| PxSceneQueryFilterFlag::ePOSTFILTER);
	PxSceneQueryFlags POutputFlags = PxSceneQueryFlag::ePOSITION | PxSceneQueryFlag::eNORMAL | PxSceneQueryFlag::eDISTANCE;
	FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params.IgnoreActors);
	PQueryCallbackSweep.DiscardInitialOverlaps = !Params.bFindInitialOverlaps;

	const FVector Delta = End - Start;
	const float DeltaMag = Delta.Size();
	if(DeltaMag > KINDA_SMALL_NUMBER)
	{
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		const PxTransform PStartTM(U2PVector(Start), PGeomRot);
		const PxVec3 PDir = U2PVector(Delta/DeltaMag);

		// Keep track of closest blocking hit distance.
		float MinBlockDistance = DeltaMag;

		PxSweepHit PHits[HIT_BUFFER_SIZE];
		bool bBlockingHitSync;
		PxI32 NumHits = SyncScene->sweepMultiple(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PHits, HIT_BUFFER_MAX_SYNC_QUERIES, bBlockingHitSync, PQueryFilterData, &PQueryCallbackSweep);
		if(NumHits == -1)
		{
			UE_LOG(LogCollision, Warning, TEXT("GeomSweepMulti : Buffer overflow from synchronous scene query"));
			UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
			UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
			// sweepMultiple still fills in the buffer with the blocking hit and an arbitrary subset of nearer touches
			NumHits = HIT_BUFFER_MAX_SYNC_QUERIES;
		}

		if(bBlockingHitSync)
		{
			/** If we got a blocking hit, it will be the last in the results - get the blocking distance */
			MinBlockDistance = PHits[NumHits-1].distance;
			bBlockingHit = true;
		}

#if VERIFY_GEOMSWEEPMULTI
		// If we hit something, test we can sweep back...
		{
			if(OutHits.Num() == 0)
			{
				FVector HitLocation = End;
				float TestLength = DeltaMag;
				if(bBlockingHitSync)
				{
					TestLength = PHits[NumHits-1].distance;
					FHitResult TestHit;
					ConvertQueryImpactHit(PHits[NumHits-1], TestHit, DeltaMag, PFilter, Start, End, &PGeom, PStartTM, false, Params.bReturnPhysicalMaterial);
					HitLocation = TestHit.Location;
				}

				UE_LOG(LogCollision, Warning, TEXT("%d Start: %s End: %s"), bBlockingHitSync, *Start.ToString(), *HitLocation.ToString() );

				PxTransform PEndTM(U2PVector(HitLocation), PGeomRot);

				PxSweepHit PTestHits[HIT_BUFFER_MAX_SYNC_QUERIES];
				bool bTestBlockingHit;
				PxI32 NumTestHits = SyncScene->sweepMultiple(PGeom, PEndTM, -PDir, TestLength, POutputFlags, PTestHits, HIT_BUFFER_MAX_SYNC_QUERIES, bTestBlockingHit, PQueryFilterData, &PQueryCallbackSweep);

				if(bTestBlockingHit)
				{
					UE_LOG(LogCollision, Warning, TEXT("GeomSweepMulti : Cannot sweep back from end location."));

					PxSweepHit PReproHits[HIT_BUFFER_MAX_SYNC_QUERIES];
					bool bReproBlockingHit;
					PxI32 NumReproHits = SyncScene->sweepMultiple(PGeom, PStartTM, PDir, DeltaMag, POutputFlags, PReproHits, HIT_BUFFER_MAX_SYNC_QUERIES, bReproBlockingHit, PQueryFilterData, &PQueryCallbackSweep);
				}
			}
			else
			{
				UE_LOG(LogCollision, Warning, TEXT("STUCK Start: %s End: %s"), *Start.ToString(), *End.ToString());
			}
		}
#endif

		// Test async scene if async tests are requested and there was no overflow
		if( Params.bTraceAsyncScene && MinBlockDistance > SMALL_NUMBER && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			// Write into the same PHits buffer
			PxSweepHit* PHitsAsync = PHits + NumHits;
			const PxI32 AsyncBufferSize = (ARRAY_COUNT(PHits) - NumHits);
			check(AsyncBufferSize > 0);
			bool bBlockingHitAsync = false;
			PxI32 NumAsyncHits = AsyncScene->sweepMultiple(PGeom, PStartTM, PDir, MinBlockDistance, POutputFlags, PHitsAsync, AsyncBufferSize, bBlockingHitAsync, PQueryFilterData, &PQueryCallbackSweep);
			if(NumAsyncHits == -1)
			{
				UE_LOG(LogCollision, Log, TEXT("GeomSweepMulti : Buffer overflow from asynchronous scene query"));
				UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
				// sweepMultiple still fills in the buffer with the blocking hit and an arbitrary subset of nearer touches
				NumAsyncHits = AsyncBufferSize;
			}

			NumHits += NumAsyncHits;

			if(bBlockingHitAsync)
			{
				MinBlockDistance = FMath::Min<float>(PHits[NumHits-1].distance, MinBlockDistance);
				bBlockingHit = true;
			}
		}

		// Convert all hits to unreal structs. This will remove any hits further than MinBlockDistance, and sort results.
		if(NumHits > 0)
		{
			bBlockingHit |= AddSweepResults(NumHits, PHits, DeltaMag, PFilter, OutHits, Start, End, PGeom, PStartTM, MinBlockDistance, Params.bReturnPhysicalMaterial);
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, OutHits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryType::Multi, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, OutHits);

	return bBlockingHit;
}

//////////////////////////////////////////////////////////////////////////
// GEOM OVERLAP

bool GeomOverlapTest(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomOverlapAny);

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	// Create filter data used to filter collisions
	PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
	PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
	FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);

	PxOverlapHit POverlapResult;
	bool bHit = false;

	FPhysScene* PhysScene = World->GetPhysicsScene();
	PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

	{
		// Enable scene locks, in case they are required
		SCOPED_SCENE_READ_LOCK(SyncScene);

		bHit = SyncScene->overlapAny(PGeom, PGeomPose, POverlapResult, PQueryFilterData, &PQueryCallback);
	}

	// if we got a hit, need to see if it was a touch or a block
	// @todo UE4 james remove this once overlapAny only returns blocking hits
	if(bHit)
	{
		FOverlapResult NewOverlap;
		ConvertQueryOverlap(POverlapResult.shape, POverlapResult.actor, NewOverlap, PFilter);
		bHaveBlockingHit = NewOverlap.bBlockingHit;
	}

	// Test async scene if async tests are requested and there was no blocking hit was found in the sync scene (since no hit info other than a boolean yes/no is recorded)
	if( !bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene() )
	{
		PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
		SCOPED_SCENE_READ_LOCK(AsyncScene);

		PxOverlapHit PAnotherOverlapResult;
		bHit = AsyncScene->overlapAny(PGeom, PGeomPose,  PAnotherOverlapResult, PQueryFilterData, &PQueryCallback);

		if(bHit)
		{
			FOverlapResult NewOverlap;
			ConvertQueryOverlap( PAnotherOverlapResult.shape, PAnotherOverlapResult.actor, NewOverlap, PFilter);
			bHaveBlockingHit = NewOverlap.bBlockingHit;
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		TArray<FOverlapResult> Overlaps;
		DrawGeomOverlaps(World, PGeom, PGeomPose, Overlaps, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	return bHaveBlockingHit;
}

bool GeomOverlapSingle(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, FOverlapResult& OutOverlap, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomOverlapSingle);
	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	// Create filter data used to filter collisions
	PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, false);
	PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
	FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);

	// Enable scene locks, in case they are required
	FPhysScene* PhysScene = World->GetPhysicsScene();
	PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

	SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

	PxOverlapHit POverlapResult;
	bool bHit = SyncScene->overlapAny(PGeom, PGeomPose, POverlapResult, PQueryFilterData, &PQueryCallback);

	// if we got a hit, need to see if it was a touch or a block
	// @todo UE4 james remove this once overlapAny only returns blocking hits
	if(bHit)
	{
		ConvertQueryOverlap( POverlapResult.shape, POverlapResult.actor, OutOverlap, PFilter);
		bHaveBlockingHit = OutOverlap.bBlockingHit;
	}

	// Test async scene if async tests are requested and there was no blocking hit was found in the sync scene (since no hit info other than a boolean yes/no is recorded)
	if( !bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene() )
	{
		PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
		SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

		PxOverlapHit PAnotherOverlapResult;
		bHit = AsyncScene->overlapAny(PGeom, PGeomPose, PAnotherOverlapResult, PQueryFilterData, &PQueryCallback);

		if(bHit)
		{
			FOverlapResult NewOverlap;
			ConvertQueryOverlap(PAnotherOverlapResult.shape, PAnotherOverlapResult.actor, NewOverlap, PFilter);
			bHaveBlockingHit = NewOverlap.bBlockingHit;
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
	{
		TArray<FOverlapResult> Overlaps;
		if (bHit)
		{
			Overlaps.Add(OutOverlap);
		}

		DrawGeomOverlaps(World, PGeom, PGeomPose, Overlaps, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	return bHaveBlockingHit;
}

bool GeomOverlapMulti(const UWorld * World, const PxGeometry& PGeom, const PxTransform& PGeomPose, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if(World == NULL || World->GetPhysicsScene() == NULL)
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomOverlapMultiple);

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	// overlapMultiple only supports sphere/capsule/box 
	if (PGeom.getType()==PxGeometryType::eSPHERE || PGeom.getType()==PxGeometryType::eCAPSULE || PGeom.getType()==PxGeometryType::eBOX || PGeom.getType()==PxGeometryType::eCONVEXMESH )
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, ObjectParams, true);
		PxSceneQueryFilterData PQueryFilterData(PFilter, PxSceneQueryFilterFlag::eSTATIC | PxSceneQueryFilterFlag::eDYNAMIC | PxSceneQueryFilterFlag::ePREFILTER);
		FPxQueryFilterCallback PQueryCallback(Params.IgnoreActors);

		// Enable scene locks, in case they are required
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		SCOPED_SCENE_READ_LOCK_INDEXED(SyncScene, 1);

		PxOverlapHit POverlapArray[OVERLAP_BUFFER_SIZE]; 
		PxI32 NumHits = SyncScene->overlapMultiple(PGeom, PGeomPose, POverlapArray,  OVERLAP_BUFFER_SIZE_MAX_SYNC_QUERIES, PQueryFilterData, &PQueryCallback);
		
		if(NumHits == -1)
		{
			UE_LOG(LogCollision, Warning, TEXT("GeomOverlapMulti : Buffer overflow from synchronous scene query"));
			UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
			UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
			// overlapMultiple still fills in the buffer with an arbitrary set of overlapping objects, so we have a full buffer of results
			// we can use, though that is not the entire set
			NumHits = OVERLAP_BUFFER_SIZE_MAX_SYNC_QUERIES;
		}

		// Test async scene if async tests are requested and there was no overflow
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{		
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK_INDEXED(AsyncScene, 2);

			// Write into the same PHits buffer
			PxOverlapHit* PAsyncOverlapArray = POverlapArray + NumHits;
			const PxI32 AsyncBufferSize = (ARRAY_COUNT(POverlapArray) - NumHits);
			check(AsyncBufferSize > 0);
			PxI32 NumAsyncHits = AsyncScene->overlapMultiple(PGeom, PGeomPose, PAsyncOverlapArray, AsyncBufferSize, PQueryFilterData, &PQueryCallback);

			if(NumAsyncHits == -1)
			{
				UE_LOG(LogCollision, Log, TEXT("GeomOverlapMulti : Buffer overflow from asynchronous scene query"));
				UE_LOG(LogCollision, Warning, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Warning, TEXT("--------%s"), *Params.ToString());
				// overlapMultiple still fills in the buffer with an arbitrary set of overlapping objects, so we have a full buffer of results
				// we can use, though that is not the entire set
				NumAsyncHits = AsyncBufferSize;
			}

			NumHits += NumAsyncHits;
		}

		if(NumHits > 0)
		{
			bHaveBlockingHit = ConvertOverlapResults(NumHits, POverlapArray, PFilter, OutOverlaps);
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if((World->DebugDrawTraceTag != NAME_None) && (World->DebugDrawTraceTag == Params.TraceTag))
		{
			DrawGeomOverlaps(World, PGeom, PGeomPose, OutOverlaps, DebugLineLifetime);
		}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}
	else
	{
		UE_LOG(LogCollision, Log, TEXT("GeomOverlapMulti : unsupported shape - only supports sphere, capsule, box"));
	}

	return bHaveBlockingHit;
}

//////////////////////////////////////////////////////////////////////////

static PxQuat CapsuleRotator(0.f, 0.707106781f, 0.f, 0.707106781f );

PxQuat ConvertToPhysXCapsuleRot(const FQuat& GeomRot)
{
	// Rotation required because PhysX capsule points down X, we want it down Z
	return U2PQuat(GeomRot) * CapsuleRotator;
}

FQuat ConvertToUECapsuleRot(const PxQuat & PGeomRot)
{
	return P2UQuat(PGeomRot * CapsuleRotator.getConjugate());
}

PxTransform ConvertToPhysXCapsulePose(const FTransform& GeomPose)
{
	PxTransform PFinalPose;

	PFinalPose.p = U2PVector(GeomPose.GetTranslation());
	// Rotation required because PhysX capsule points down X, we want it down Z
	PFinalPose.q = ConvertToPhysXCapsuleRot(GeomPose.GetRotation());
	return PFinalPose;
}

//////////////////////////////////////////////////////////////////////////


void CreateShapeFilterData(const uint8 MyChannel, const int32 ActorID, const FCollisionResponseContainer& ResponseToChannels, uint32 SkelMeshCompID, uint16 BodyIndex, PxFilterData& OutQueryData, PxFilterData& OutSimData, bool bEnableCCD, bool bEnableContactNotify, bool bStaticShape)
{
	/**
	 * Format for SimData : 
	 * 		word0 (body index)
	 *		word1 (blocking channels)
	 *		word2 (skeletal mesh component ID)
	 * 		word3 (MyChannel (top 8) + Flags (lower 24))
	 * 
	 * Format for QueryData : 
	 *		word0 (actor ID)
	 *		word1 (blocking channels)
	 *		word2 (touching channels)
	 *		word3 (MyChannel (top 8) as ECollisionChannel + Flags (lower 24))
	 */
	 
	OutQueryData.setToDefault();
	OutSimData.setToDefault();

	// query word 0 has ActorID
	OutQueryData.word0 = ActorID;

	// query word 0 bodyindex 
	OutSimData.word0 = BodyIndex;
	// query word 2 stores asset instance ID
	OutSimData.word2 = SkelMeshCompID;

	// word1 encodes 'what i block', word2 encodes 'what i touch'
	for(int32 i=0; i<ARRAY_COUNT(ResponseToChannels.EnumArray); i++)
	{
		// if i block, set that in word1
		if(ResponseToChannels.EnumArray[i] == ECR_Block)
		{
			PxU32 const ChannelBit = CRC_TO_BITFIELD(i);
			OutQueryData.word1 |= ChannelBit;
			OutSimData.word1 |= ChannelBit;
		}
		else if(ResponseToChannels.EnumArray[i] == ECR_Overlap)
		{
			// if i touch, set that in word2
			OutQueryData.word2 |= CRC_TO_BITFIELD(i);
		}
	}

	// enable CCD at word 3
	if(bEnableCCD)
	{
		OutSimData.word3 |= EPDF_CCD;
	}

	// Contact notify
	if(bEnableContactNotify)
	{
		OutSimData.word3 |= EPDF_ContactNotify;
	}

	// Whether shape is static
	if(bStaticShape)
	{
		OutSimData.word3 |= EPDF_StaticShape;
	}
	
	// make sure it doesn't have flag beyond usage
	// we only allow lower 24 bits for flag
	// warn if that's used before setting MyChannel
	check((OutSimData.word3 & 0xFF000000) == 0);

	// pack MyChannel
	OutSimData.word3 |=  (MyChannel << 24);

	// QueryData word3 is same as SimData
	OutQueryData.word3 = OutSimData.word3;
}

PxFilterData CreateObjectQueryFilterData(const bool bTraceComplex, const int32 MultiTrace/*=1 if multi. 0 otherwise*/, const struct FCollisionObjectQueryParams & ObjectParam)
{
	/**
	 * Format for QueryData : 
	 *		word0 (meta data - ECollisionQuery. Extendable)
	 *
	 *		For object queries
	 *
	 *		word1 (object type queries)
	 *		word2 (unused)
	 *		word3 (Multi (1) or single (0) (top 8) + Flags (lower 24))
	 */

	PxFilterData PNewData;

	PNewData.word0 = ECollisionQuery::ObjectQuery;

	if (bTraceComplex)
	{
		PNewData.word3 |= EPDF_ComplexCollision;
	}
	else
	{
		PNewData.word3 |= EPDF_SimpleCollision;
	}

	// get object param bits
	PNewData.word1 = ObjectParam.GetQueryBitfield();

	// make sure it doesn't have flag beyond usage
	// we only allow lower 24 bits for flag
	// warn if that's used before setting MyChannel
	check((PNewData.word3 & 0xFF000000) == 0);

	// if 'nothing', then set no bits
	PNewData.word3 |= (MultiTrace) << 24;

	return PNewData;
}

PxFilterData CreateTraceQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer & InCollisionResponseContainer)
{
	/**
	 * Format for QueryData : 
	 *		word0 (meta data - ECollisionQuery. Extendable)
	 *
	 *		For trace queries
	 *
	 *		word1 (blocking channels)
	 *		word2 (touching channels)
	 *		word3 (MyChannel (top 8) as ECollisionChannel + Flags (lower 24))
	 */

	PxFilterData PNewData;

	PNewData.word0 = ECollisionQuery::TraceQuery;

	if (bTraceComplex)
	{
		PNewData.word3 |= EPDF_ComplexCollision;
	}
	else
	{
		PNewData.word3 |= EPDF_SimpleCollision;
	}

	// word1 encodes 'what i block', word2 encodes 'what i touch'
	for(int32 i=0; i<ARRAY_COUNT(InCollisionResponseContainer.EnumArray); i++)
	{
		if(InCollisionResponseContainer.EnumArray[i] == ECR_Block)
		{
			// if i block, set that in word1
			PNewData.word1 |= CRC_TO_BITFIELD(i);
		}
		else if(InCollisionResponseContainer.EnumArray[i] == ECR_Overlap)
		{
			// if i touch, set that in word2
			PNewData.word2 |= CRC_TO_BITFIELD(i);
		}
	}

	// make sure it doesn't have flag beyond usage
	// we only allow lower 24 bits for flag
	// warn if that's used before setting MyChannel
	check((PNewData.word3 & 0xFF000000) == 0);

	// if 'nothing', then set no bits
	PNewData.word3 |= (MyChannel) << 24;

	return PNewData;
}

/** Utility for creating a PhysX PxFilterData for performing a query (trace) against the scene */
PxFilterData CreateQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer & InCollisionResponseContainer, const struct FCollisionObjectQueryParams & ObjectParam, const bool bMultitrace)
{
	if (ObjectParam.IsValid() )
	{
		return CreateObjectQueryFilterData(bTraceComplex, (bMultitrace? TRACE_MULTI : TRACE_SINGLE), ObjectParam);
	}
	else
	{
		return CreateTraceQueryFilterData(MyChannel, bTraceComplex, InCollisionResponseContainer);
	}
}

PxGeometry * GetGeometryFromShape(GeometryFromShapeStorage & LocalStorage, const PxShape * PShape, bool bTriangleMeshAllowed /*= false*/)
{
	switch (PShape->getGeometryType())
	{
	case PxGeometryType::eSPHERE:
		PShape->getSphereGeometry(LocalStorage.SphereGeom);
		return &LocalStorage.SphereGeom;
	case PxGeometryType::eBOX:
		PShape->getBoxGeometry(LocalStorage.BoxGeom);
		return &LocalStorage.BoxGeom;
	case PxGeometryType::eCAPSULE:
		PShape->getCapsuleGeometry(LocalStorage.CapsuleGeom);
		return &LocalStorage.CapsuleGeom;
	case PxGeometryType::eCONVEXMESH:
		PShape->getConvexMeshGeometry(LocalStorage.ConvexGeom);
		return &LocalStorage.ConvexGeom;
	case PxGeometryType::eTRIANGLEMESH:
		if (bTriangleMeshAllowed)
		{
			PShape->getTriangleMeshGeometry(LocalStorage.TriangleGeom);
			return &LocalStorage.TriangleGeom;
		}
	default:
		return NULL;
	}
}

#endif // WITH_PHYSX
