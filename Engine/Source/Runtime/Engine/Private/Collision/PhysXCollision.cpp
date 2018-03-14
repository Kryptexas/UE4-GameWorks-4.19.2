// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Collision/PhysXCollision.h"
#include "Engine/World.h"
#include "Collision.h"
#include "CollisionDebugDrawingPublic.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysXSupport.h"

float DebugLineLifetime = 2.f;

#if WITH_PHYSX

#include "Collision/CollisionDebugDrawing.h"
#include "Collision/CollisionConversions.h"
#include "ScopedTimers.h"

/**
 * Helper to lock/unlock multiple scenes that also makes sure to unlock everything when it goes out of scope.
 * Multiple locks on the same scene are NOT SAFE. You can't call LockRead() if already locked.
 * Multiple unlocks on the same scene are safe (repeated unlocks do nothing after the first successful unlock).
 */
struct FScopedMultiSceneReadLock
{
	FScopedMultiSceneReadLock()
	{
		for (int32 i=0; i < ARRAY_COUNT(SceneLocks); i++)
		{
			SceneLocks[i] = nullptr;
		}
	}

	~FScopedMultiSceneReadLock()
	{
		UnlockAll();
	}

	inline void LockRead(const UWorld* World, PxScene* Scene, EPhysicsSceneType SceneType)
	{
		checkSlow(SceneLocks[SceneType] == nullptr); // no nested locks allowed.
		SCENE_LOCK_READ(Scene);
		SceneLocks[SceneType] = Scene;
	}

	inline void UnlockRead(PxScene* Scene, EPhysicsSceneType SceneType)
	{
		checkSlow(SceneLocks[SceneType] == Scene || SceneLocks[SceneType] == nullptr);
		SCENE_UNLOCK_READ(Scene);
		SceneLocks[SceneType] = nullptr;
	}

	inline void UnlockAll()
	{
		for (int32 i=0; i < ARRAY_COUNT(SceneLocks); i++)
		{
			SCENE_UNLOCK_READ(SceneLocks[i]);
			SceneLocks[i] = nullptr;
		}
	}

	PxScene* SceneLocks[PST_MAX];
};


/** 
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

PxQueryHitType::Enum FPxQueryFilterCallback::CalcQueryHitType(const PxFilterData &PQueryFilter, const PxFilterData &PShapeFilter, bool bPreFilter)
{
	ECollisionQuery::Type QueryType = (ECollisionQuery::Type)PQueryFilter.word0;
	FMaskFilter QuerierMaskFilter;
	const ECollisionChannel QuerierChannel = GetCollisionChannelAndExtraFilter(PQueryFilter.word3, QuerierMaskFilter);

	FMaskFilter ShapeMaskFilter;
	const ECollisionChannel ShapeChannel = GetCollisionChannelAndExtraFilter(PShapeFilter.word3, ShapeMaskFilter);

	if ((QuerierMaskFilter & ShapeMaskFilter) != 0)	//If ignore mask hit something, ignore it
	{
		return PxQueryHitType::eNONE;
	}

	const PxU32 ShapeBit = ECC_TO_BITFIELD(ShapeChannel);
	if (QueryType == ECollisionQuery::ObjectQuery)
	{
		const int32 MultiTrace = (int32) QuerierChannel;
		// do I belong to one of objects of interest?
		if ( ShapeBit & PQueryFilter.word1)
		{
			if (bPreFilter)	//In the case of an object query we actually want to return all object types (or first in single case). So in PreFilter we have to trick physx by not blocking in the multi case, and blocking in the single case.
			{

				return MultiTrace ? PxQueryHitType::eTOUCH : PxQueryHitType::eBLOCK;
			}
			else
			{
				return PxQueryHitType::eBLOCK;	//In the case where an object query is being resolved for the user we just return a block because object query doesn't have the concept of overlap at all and block seems more natural
			}
		}
	}
	else
	{
		// Then see if the channel wants to be blocked
		// @todo delete once we fix up object/trace APIs to work separate
		PxU32 ShapeFlags = PShapeFilter.word3 & 0xFFFFFF;
		bool bStaticShape = ((ShapeFlags & EPDF_StaticShape) != 0);

		// if query channel is Touch All, then just return touch
		if (QuerierChannel == ECC_OverlapAll_Deprecated)
		{
			return PxQueryHitType::eTOUCH;
		}
		// @todo delete once we fix up object/trace APIs to work separate

		PxU32 const QuerierBit = ECC_TO_BITFIELD(QuerierChannel);
		PxQueryHitType::Enum QuerierHitType = PxQueryHitType::eNONE;
		PxQueryHitType::Enum ShapeHitType = PxQueryHitType::eNONE;

		// check if Querier wants a hit
		if ((QuerierBit & PShapeFilter.word1) != 0)
		{
			QuerierHitType = PxQueryHitType::eBLOCK;
		}
		else if ((QuerierBit & PShapeFilter.word2)!=0)
		{
			QuerierHitType = PxQueryHitType::eTOUCH;
		}

		if ((ShapeBit & PQueryFilter.word1) != 0)
		{
			ShapeHitType = PxQueryHitType::eBLOCK;
		}
		else if ((ShapeBit & PQueryFilter.word2)!=0)
		{
			ShapeHitType = PxQueryHitType::eTOUCH;
		}

		// return minimum agreed-upon interaction
		return FMath::Min(QuerierHitType, ShapeHitType);
	}

	return PxQueryHitType::eNONE;
}

#if DETECT_SQ_HITCHES
int GSQHitchDetection = 0;
FAutoConsoleVariableRef CVarSQHitchDetection(TEXT("p.SQHitchDetection"), GSQHitchDetection,
	TEXT("Whether to detect scene query hitches. 0 is off. 1 repeats a slow scene query once and prints extra information. 2+ repeat slow query n times without recording (useful when profiling)")
);

int GSQHitchDetectionForceNames = 0;
FAutoConsoleVariableRef CVarSQHitchDetectionForceNames(TEXT("p.SQHitchDetectionForceNames"), GSQHitchDetectionForceNames,
	TEXT("Whether name resolution is forced off the game thread. This is not 100% safe, but can be useful when looking at hitches off GT")
);

float GSQHitchDetectionThreshold = 0.05f;
FAutoConsoleVariableRef CVarSQHitchDetectionThreshold(TEXT("p.SQHitchDetectionThreshold"), GSQHitchDetectionThreshold,
	TEXT("Determines the threshold in milliseconds for a scene query hitch.")
);
#endif

#if WITH_PHYSX
/** Various info we want to capture for hitch detection reporting */
struct FHitchDetectionInfo
{
#if DETECT_SQ_HITCHES
	FVector Start;
	FVector End;
	PxTransform Pose;
	ECollisionChannel TraceChannel;
	const FCollisionQueryParams& Params;
	bool bInTM;

	FHitchDetectionInfo(const FVector& InStart, const FVector& InEnd, ECollisionChannel InTraceChannel, const FCollisionQueryParams& InParams)
		: Start(InStart)
		, End(InEnd)
		, TraceChannel(InTraceChannel)
		, Params(InParams)
		, bInTM(false)
	{
	}

	FHitchDetectionInfo(const PxTransform& InPose, ECollisionChannel InTraceChannel, const FCollisionQueryParams& InParams)
		: Pose(InPose)
		, TraceChannel(InTraceChannel)
		, Params(InParams)
		, bInTM(true)
	{
	}

	FString ToString() const
	{
		if (bInTM)
		{
			return FString::Printf(TEXT("Pose:%s TraceChannel:%d Params:%s"), *P2UTransform(Pose).ToString(), (int32)TraceChannel, *Params.ToString());
		}
		else
		{
			return FString::Printf(TEXT("Start:%s End:%s TraceChannel:%d Params:%s"), *Start.ToString(), *End.ToString(), (int32)TraceChannel, *Params.ToString());
		}
	};
#else
	FHitchDetectionInfo(const FVector&, const FVector&, ECollisionChannel, const FCollisionQueryParams&) {}
	FHitchDetectionInfo(const PxTransform& InPose, ECollisionChannel InTraceChannel, const FCollisionQueryParams& InParams) {}
	FString ToString() const { return FString(); }
#endif
};

template <typename BufferType>
struct FScopedSQHitchRepeater
{
#if DETECT_SQ_HITCHES
	double HitchDuration;
	FDurationTimer HitchTimer;
	int32 LoopCounter;
	BufferType& UserBuffer;	//The buffer the user would normally use when no repeating happens
	BufferType* OriginalBuffer;	//The buffer as it was before the query, this is needed to maintain the same buffer properties for each loop
	BufferType* RepeatBuffer;			//Dummy buffer for loops
	FPxQueryFilterCallback& QueryCallback;
	FHitchDetectionInfo HitchDetectionInfo;

	bool RepeatOnHitch()
	{
		if(GSQHitchDetection)
		{
			if (LoopCounter == 0)
			{
				HitchTimer.Stop();
			}

			const bool bLoop = (LoopCounter < GSQHitchDetection) && (HitchDuration * 1000.0) >= GSQHitchDetectionThreshold;
			++LoopCounter;
			
			QueryCallback.bRecordHitches = QueryCallback.bRecordHitches ? true : bLoop && GSQHitchDetection == 1;
			if (bLoop)
			{
				if (!RepeatBuffer)
				{
					RepeatBuffer = new BufferType(*OriginalBuffer);
				}
				else
				{
					*RepeatBuffer = *OriginalBuffer;	//make a copy to make sure we have the same behavior every iteration
				}
			}
			return bLoop;
		}
		else
		{
			return false;
		}
	}

	FScopedSQHitchRepeater(BufferType& OutBuffer, FPxQueryFilterCallback& PQueryCallback, const FHitchDetectionInfo& InHitchDetectionInfo)
		: HitchDuration(0.0)
		, HitchTimer(HitchDuration)
		, LoopCounter(0)
		, UserBuffer(OutBuffer)
		, OriginalBuffer(nullptr)
		, RepeatBuffer(nullptr)
		, QueryCallback(PQueryCallback)
		, HitchDetectionInfo(InHitchDetectionInfo)
	{
		if (GSQHitchDetection)
		{
			OriginalBuffer = new BufferType(UserBuffer);
			HitchTimer.Start();
		}
	}

	BufferType& GetBuffer()
	{
		return LoopCounter == 0 ? UserBuffer : *RepeatBuffer;
	}

	~FScopedSQHitchRepeater()
	{
		if (QueryCallback.bRecordHitches)
		{
			const double DurationInMS = HitchDuration * 1000.0;
			UE_LOG(LogCollision, Warning, TEXT("SceneQueryHitch: took %.3fms with %d calls to PreFilter"), DurationInMS, QueryCallback.PreFilterHitchInfo.Num());
			UE_LOG(LogCollision, Warning, TEXT("\t%s"), *HitchDetectionInfo.ToString());
			for (const FPxQueryFilterCallback::FPreFilterRecord& Record : QueryCallback.PreFilterHitchInfo)
			{
				UE_LOG(LogCollision, Warning, TEXT("\tPreFilter:%s, result=%d"), *Record.OwnerComponentReadableName, (int32)Record.Result);
			}
			QueryCallback.PreFilterHitchInfo.Empty();
		}

		QueryCallback.bRecordHitches = false;
		delete RepeatBuffer;
		delete OriginalBuffer;
	}
#else
	FScopedSQHitchRepeater(BufferType& OutBuffer, FPxQueryFilterCallback& PQueryCallback, const FHitchDetectionInfo& InHitchDetectionInfo) 
		: UserBuffer(OutBuffer)
	{
	}

	BufferType& UserBuffer;	//The buffer the user would normally use when no repeating happens

	BufferType& GetBuffer() const { return UserBuffer; }
	bool RepeatOnHitch() const { return false; }
#endif
};
#endif // WITH_PHYSX

PxQueryHitType::Enum FPxQueryFilterCallback::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_PreFilter);

	ensureMsgf(shape, TEXT("Invalid shape encountered in FPxQueryFilterCallback::preFilter, actor: %p, filterData: %x %x %x %x"), actor, filterData.word0, filterData.word1, filterData.word2, filterData.word3);

#if DETECT_SQ_HITCHES
	FPreFilterRecord* PreFilterRecord = nullptr;
	if (bRecordHitches && (IsInGameThread() || GSQHitchDetectionForceNames))
	{
		PreFilterHitchInfo.AddZeroed();
		PreFilterRecord = &PreFilterHitchInfo[PreFilterHitchInfo.Num() - 1];
		if (actor)
		{
			if (FBodyInstance* OwnerBI = FPhysxUserData::Get<FBodyInstance>(actor->userData))
			{
				if (UPrimitiveComponent* OwnerComp = OwnerBI->OwnerComponent.Get())
				{
					PreFilterRecord->OwnerComponentReadableName = OwnerComp->GetReadableName();
				}
			}
		}
	}
#endif

	if(!shape)
	{
		// Early out to avoid crashing.
		return (PrefilterReturnValue = PxQueryHitType::eNONE);
	}

	// Check if the shape is the right complexity for the trace 
	const PxFilterData ShapeFilter = shape->getQueryFilterData();

#define ENABLE_PREFILTER_LOGGING 0
#if ENABLE_PREFILTER_LOGGING
	static bool bLoggingEnabled=false;
	if (bLoggingEnabled)
	{
		FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(actor->userData);
		if ( BodyInst && BodyInst->OwnerComponent.IsValid() )
		{
			UE_LOG(LogCollision, Warning, TEXT("[PREFILTER] against %s[%s] : About to check "),
				(BodyInst->OwnerComponent.Get()->GetOwner())? *BodyInst->OwnerComponent.Get()->GetOwner()->GetName():TEXT("NO OWNER"),
				*BodyInst->OwnerComponent.Get()->GetName());
		}

		UE_LOG(LogCollision, Warning, TEXT("ShapeFilter : %x %x %x %x"), ShapeFilter.word0, ShapeFilter.word1, ShapeFilter.word2, ShapeFilter.word3);
		UE_LOG(LogCollision, Warning, TEXT("FilterData : %x %x %x %x"), filterData.word0, filterData.word1, filterData.word2, filterData.word3);
	}
#endif // ENABLE_PREFILTER_LOGGING

	// Shape : shape's Filter Data
	// Querier : filterData that owns the trace
	PxU32 ShapeFlags = ShapeFilter.word3 & 0xFFFFFF;
	PxU32 QuerierFlags = filterData.word3 & 0xFFFFFF;
	PxU32 CommonFlags = ShapeFlags & QuerierFlags;

	// First check complexity, none of them matches
	if(!(CommonFlags & EPDF_SimpleCollision) && !(CommonFlags & EPDF_ComplexCollision))
	{
		return (PrefilterReturnValue = PxQueryHitType::eNONE);
	}

	PxQueryHitType::Enum Result = FPxQueryFilterCallback::CalcQueryHitType(filterData, ShapeFilter, true);

	if (Result == PxQueryHitType::eTOUCH && bIgnoreTouches)
	{
		Result = PxQueryHitType::eNONE;
	}

	if (Result == PxQueryHitType::eBLOCK && bIgnoreBlocks)
	{
		Result = PxQueryHitType::eNONE;
	}

	// If not already rejected, check ignore actor and component list.
	if (Result != PxQueryHitType::eNONE)
	{
		// See if we are ignoring the actor this shape belongs to (word0 of shape filterdata is actorID)
		if (IgnoreActors.Contains(ShapeFilter.word0))
		{
			//UE_LOG(LogTemp, Log, TEXT("Ignoring Actor: %d"), ShapeFilter.word0);
			Result = PxQueryHitType::eNONE;
		}

		// We usually don't have ignore components so we try to avoid the virtual getSimulationFilterData() call below. 'word2' of shape sim filter data is componentID.
		if (IgnoreComponents.Num() > 0 && IgnoreComponents.Contains(shape->getSimulationFilterData().word2))
		{
			//UE_LOG(LogTemp, Log, TEXT("Ignoring Component: %d"), shape->getSimulationFilterData().word2);
			Result = PxQueryHitType::eNONE;
		}
	}

#if ENABLE_PREFILTER_LOGGING
	if (bLoggingEnabled)
	{
		FBodyInstance* BodyInst = FPhysxUserData::Get<FBodyInstance>(actor->userData);
		if ( BodyInst && BodyInst->OwnerComponent.IsValid() )
		{
			ECollisionChannel QuerierChannel = GetCollisionChannel(filterData.word3);
			UE_LOG(LogCollision, Log, TEXT("[PREFILTER] Result for Querier [CHANNEL: %d, FLAG: %x] %s[%s] [%d]"), 
				(int32)QuerierChannel, QuerierFlags,
				(BodyInst->OwnerComponent.Get()->GetOwner())? *BodyInst->OwnerComponent.Get()->GetOwner()->GetName():TEXT("NO OWNER"),
				*BodyInst->OwnerComponent.Get()->GetName(), (int32)Result);
		}
	}
#endif // ENABLE_PREFILTER_LOGGING

	if(bIsOverlapQuery && Result == PxQueryHitType::eBLOCK)
	{
		Result = PxQueryHitType::eTOUCH;	//In the case of overlaps, physx only understands touches. We do this at the end to ensure all filtering logic based on block vs overlap is correct
	}

#if DETECT_SQ_HITCHES
	if (PreFilterRecord)
	{
		PreFilterRecord->Result = Result;
	}
#endif

	return  (PrefilterReturnValue = Result);
}


PxQueryHitType::Enum FPxQueryFilterCallbackSweep::postFilter(const PxFilterData& filterData, const PxQueryHit& hit)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_PostFilter);

	PxSweepHit& SweepHit = (PxSweepHit&)hit;
	const bool bIsOverlap = SweepHit.hadInitialOverlap();

	if (bIsOverlap && DiscardInitialOverlaps)
	{
		return PxQueryHitType::eNONE;
	}
	else
	{
		if (bIsOverlap && PrefilterReturnValue == PxQueryHitType::eBLOCK)
		{
			// We want to keep initial blocking overlaps and continue the sweep until a non-overlapping blocking hit.
			// We will later report this hit as a blocking hit when we compute the hit type (using FPxQueryFilterCallback::CalcQueryHitType).
			return PxQueryHitType::eTOUCH;
		}
		
		return PrefilterReturnValue;
	}
}

#endif // WITH_PHYSX

//////////////////////////////////////////////////////////////////////////

//@TODO: BOX2D: Can we break the collision analyzer's dependence on PhysX?
#if ENABLE_COLLISION_ANALYZER

#include "ICollisionAnalyzer.h"
#include "CollisionAnalyzerModule.h"

bool GCollisionAnalyzerIsRecording = false;

bool bSkipCapture = false;

/** Util to convert from PhysX shape and rotation to unreal shape enum, dimension vector and rotation */
static void P2UGeomAndRot(const PxGeometry& PGeom, const PxQuat& PRot, ECAQueryShape::Type& OutQueryShape, FVector& OutDims, FQuat& OutQuat)
{
	OutQueryShape = ECAQueryShape::Capsule;
	OutDims = FVector(0, 0, 0);
	OutQuat = FQuat::Identity;

	switch (PGeom.getType())
	{
	case PxGeometryType::eCAPSULE:
	{
		OutQueryShape = ECAQueryShape::Capsule;
		PxCapsuleGeometry* PCapsuleGeom = (PxCapsuleGeometry*)&PGeom;
		OutDims = FVector(PCapsuleGeom->radius, PCapsuleGeom->radius, PCapsuleGeom->halfHeight + PCapsuleGeom->radius);
		OutQuat = ConvertToUECapsuleRot(PRot);
		break;
	}

	case PxGeometryType::eSPHERE:
	{
		OutQueryShape = ECAQueryShape::Sphere;
		PxSphereGeometry* PSphereGeom = (PxSphereGeometry*)&PGeom;
		OutDims = FVector(PSphereGeom->radius);
		break;
	}

	case PxGeometryType::eBOX:
	{
		OutQueryShape = ECAQueryShape::Box;
		PxBoxGeometry* PBoxGeom = (PxBoxGeometry*)&PGeom;
		OutDims = P2UVector(PBoxGeom->halfExtents);
		OutQuat = P2UQuat(PRot);
		break;
	}

	case PxGeometryType::eCONVEXMESH:
	{
		OutQueryShape = ECAQueryShape::Convex;
		break;
	}

	default:
		UE_LOG(LogCollision, Warning, TEXT("CaptureGeomSweep: Unknown geom type."));
	}
}

/** Util to extract type and dimensions from physx geom being swept, and pass info to CollisionAnalyzer, if its recording */
void CaptureGeomSweep(const UWorld* World, const FVector& Start, const FVector& End, const PxQuat& PRot, ECAQueryMode::Type QueryMode, const PxGeometry& PGeom, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TArray<FHitResult>& Results, double CPUTime)
{
	if(bSkipCapture || !GCollisionAnalyzerIsRecording || !IsInGameThread())
	{
		return;
	}

	// Convert from PhysX to Unreal types
	ECAQueryShape::Type QueryShape = ECAQueryShape::Sphere;
	FVector Dims(0,0,0);
	FQuat Rot = FQuat::Identity;
	P2UGeomAndRot(PGeom, PRot, QueryShape, Dims, Rot);

	// Do a touch all query to find things we _didn't_ hit
	bSkipCapture = true;
	TArray<FHitResult> TouchAllResults;
	GeomSweepMulti_PhysX(World, PGeom, PRot, TouchAllResults, Start, End, DefaultCollisionChannel, Params, ResponseParams, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));
	bSkipCapture = false;

	// Now tell analyzer
	FCollisionAnalyzerModule::Get()->CaptureQuery(Start, End, Rot, ECAQueryType::GeomSweep, QueryShape, QueryMode, Dims, TraceChannel, Params, ResponseParams, ObjectParams, Results, TouchAllResults, CPUTime);
}

/** Util to capture a raycast with the CollisionAnalyzer if recording */
void CaptureRaycast(const UWorld* World, const FVector& Start, const FVector& End, ECAQueryMode::Type QueryMode, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TArray<FHitResult>& Results, double CPUTime)
{
	if(bSkipCapture || !GCollisionAnalyzerIsRecording || !IsInGameThread())
	{
		return;
	}

	// Do a touch all query to find things we _didn't_ hit
	bSkipCapture = true;
	TArray<FHitResult> TouchAllResults;
	RaycastMulti(World, TouchAllResults, Start, End, DefaultCollisionChannel, Params, ResponseParams, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));
	bSkipCapture = false;

	FCollisionAnalyzerModule::Get()->CaptureQuery(Start, End, FQuat::Identity, ECAQueryType::Raycast, ECAQueryShape::Sphere, QueryMode, FVector(0,0,0), TraceChannel, Params, ResponseParams, ObjectParams, Results, TouchAllResults, CPUTime);
}

void CaptureOverlap(const UWorld* World, const PxGeometry& PGeom, const PxTransform& PGeomPose, ECAQueryMode::Type QueryMode, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, TArray<FOverlapResult>& Results, double CPUTime)
{
	if (bSkipCapture || !GCollisionAnalyzerIsRecording || !IsInGameThread())
	{
		return;
	}

	ECAQueryShape::Type QueryShape = ECAQueryShape::Sphere;
	FVector Dims(0, 0, 0);
	FQuat Rot = FQuat::Identity;
	P2UGeomAndRot(PGeom, PGeomPose.q, QueryShape, Dims, Rot);

	TArray<FHitResult> HitResults;
	for(const FOverlapResult& OverlapResult : Results)
	{
		FHitResult NewResult = FHitResult(0.f);
		NewResult.bBlockingHit = OverlapResult.bBlockingHit;
		NewResult.Actor = OverlapResult.Actor;
		NewResult.Component = OverlapResult.Component;
		NewResult.Item = OverlapResult.ItemIndex;
		HitResults.Add(NewResult);
	}

	TArray<FHitResult> TouchAllResults;
	// TODO: Fill in 'all results' for overlaps

	FCollisionAnalyzerModule::Get()->CaptureQuery(P2UVector(PGeomPose.p), FVector(0,0,0), Rot, ECAQueryType::GeomOverlap, QueryShape, QueryMode, Dims, TraceChannel, Params, ResponseParams, ObjectParams, HitResults, TouchAllResults, CPUTime);
}

#define STARTQUERYTIMER() double StartTime = FPlatformTime::Seconds()
#define CAPTUREGEOMSWEEP(World, Start, End, Rot, QueryMode, PGeom, TraceChannel, Params, ResponseParam, ObjectParam, Results) if (GCollisionAnalyzerIsRecording && IsInGameThread()) { CaptureGeomSweep(World, Start, End, Rot, QueryMode, PGeom, TraceChannel, Params, ResponseParam, ObjectParam, Results, FPlatformTime::Seconds() - StartTime); }
#define CAPTURERAYCAST(World, Start, End, QueryMode, TraceChannel, Params, ResponseParam, ObjectParam, Results)	if (GCollisionAnalyzerIsRecording && IsInGameThread()) { CaptureRaycast(World, Start, End, QueryMode, TraceChannel, Params, ResponseParam, ObjectParam, Results, FPlatformTime::Seconds() - StartTime); }
#define CAPTUREGEOMOVERLAP(World, PGeom, PGeomPose, QueryMode, TraceChannel, Params, ResponseParams, ObjectParams, Results)	if (GCollisionAnalyzerIsRecording && IsInGameThread()) { CaptureOverlap(World, PGeom, PGeomPose, QueryMode, TraceChannel, Params, ResponseParams, ObjectParams, Results, FPlatformTime::Seconds() - StartTime); }


#else

#define STARTQUERYTIMER() 
#define CAPTUREGEOMSWEEP(...)
#define CAPTURERAYCAST(...)
#define CAPTUREGEOMOVERLAP(...)

#endif // ENABLE_COLLISION_ANALYZER

#if WITH_PHYSX
PxQueryFlags StaticDynamicQueryFlags(const FCollisionQueryParams& Params)
{
	switch(Params.MobilityType)
	{
		case EQueryMobilityType::Any: return  PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
		case EQueryMobilityType::Static: return  PxQueryFlag::eSTATIC;
		case EQueryMobilityType::Dynamic: return  PxQueryFlag::eDYNAMIC;
		default: check(0);
	}

	check(0);
	return PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
}
#endif


//////////////////////////////////////////////////////////////////////////
// RAYCAST

bool RaycastTest(const UWorld* World, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastAny);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits

#if WITH_PHYSX
	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if (DeltaMag > KINDA_SMALL_NUMBER)
	{
		{
			const PxVec3 PDir = U2PVector(Delta / DeltaMag);
			PxRaycastBuffer PRaycastBuffer;

			// Create filter data used to filter collisions
			PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, false);
			PxSceneQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT);
			PxHitFlags POutputFlags = PxHitFlags();
			FPxQueryFilterCallback PQueryCallback(Params);
			PQueryCallback.bIgnoreTouches = true; // pre-filter to ignore touches and only get blocking hits.

			FPhysScene* PhysScene = World->GetPhysicsScene();
			{
				// Enable scene locks, in case they are required
				PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);
				SCOPED_SCENE_READ_LOCK(SyncScene);
				FScopedSQHitchRepeater<decltype(PRaycastBuffer)> HitchRepeater(PRaycastBuffer, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do
				{
					SyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());
				bHaveBlockingHit = PRaycastBuffer.hasBlock;
			}

			// Test async scene if we have no blocking hit, and async tests are requested
			if (!bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
			{
				PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
				SCOPED_SCENE_READ_LOCK(AsyncScene);
				FScopedSQHitchRepeater<decltype(PRaycastBuffer)> HitchRepeater(PRaycastBuffer, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do
				{
					AsyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());
				bHaveBlockingHit = PRaycastBuffer.hasBlock;
			}
		}
	}

	TArray<FHitResult> Hits;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(World->DebugDrawSceneQueries(Params.TraceTag))
	{
		DrawLineTraces(World, Start, End, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	CAPTURERAYCAST(World, Start, End, ECAQueryMode::Test, TraceChannel, Params, ResponseParams, ObjectParams, Hits);

#endif // WITH_PHYSX 
	return bHaveBlockingHit;
}

bool RaycastSingle(const UWorld* World, struct FHitResult& OutHit, const FVector Start, const FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastSingle);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	OutHit = FHitResult();
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits
#if WITH_PHYSX

	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if (DeltaMag > KINDA_SMALL_NUMBER)
	{
		{
			FScopedMultiSceneReadLock SceneLocks;

			// Create filter data used to filter collisions
			PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, false);
			PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER);
			PxHitFlags POutputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE | PxHitFlag::eMTD | PxHitFlag::eFACE_INDEX;
			FPxQueryFilterCallback PQueryCallback(Params);
			PQueryCallback.bIgnoreTouches = true; // pre-filter to ignore touches and only get blocking hits.

			PxVec3 PStart = U2PVector(Start);
			PxVec3 PDir = U2PVector(Delta / DeltaMag);

			FPhysScene* PhysScene = World->GetPhysicsScene();
			PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

			// Enable scene locks, in case they are required
			SceneLocks.LockRead(World, SyncScene, PST_Sync);

			PxRaycastBuffer PRaycastBuffer;
			{
				FScopedSQHitchRepeater<decltype(PRaycastBuffer)> HitchRepeater(PRaycastBuffer, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do
				{
					SyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());
			}
			bHaveBlockingHit = PRaycastBuffer.hasBlock;
			if (!bHaveBlockingHit)
			{
				// Not going to use anything from this scene, so unlock it now.
				SceneLocks.UnlockRead(SyncScene, PST_Sync);
			}

			// Test async scene if async tests are requested
			if (Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
			{
				PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
				SceneLocks.LockRead(World, AsyncScene, PST_Async);
				PxRaycastBuffer PRaycastBufferAsync;
				FScopedSQHitchRepeater<decltype(PRaycastBufferAsync)> HitchRepeater(PRaycastBufferAsync, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do
				{
					AsyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());
				const bool bHaveBlockingHitAsync = PRaycastBufferAsync.hasBlock;

				// If we have a blocking hit in the async scene and there was no sync blocking hit, or if the async blocking hit came first,
				// then this becomes the blocking hit.  We can test the PxSceneQueryImpactHit::distance since the DeltaMag is the same for both queries.
				if (bHaveBlockingHitAsync && (!bHaveBlockingHit || PRaycastBufferAsync.block.distance < PRaycastBuffer.block.distance))
				{
					PRaycastBuffer = PRaycastBufferAsync;
					bHaveBlockingHit = true;
				}
				else
				{
					// Not going to use anything from this scene, so unlock it now.
					SceneLocks.UnlockRead(AsyncScene, PST_Async);
				}
			}

			if (bHaveBlockingHit) // If we got a hit
			{
				PxTransform PStartTM(U2PVector(Start));
				if (ConvertQueryImpactHit(World, PRaycastBuffer.block, OutHit, DeltaMag, PFilter, Start, End, NULL, PStartTM, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial) == EConvertQueryResult::Invalid)
				{
					bHaveBlockingHit = false;
					UE_LOG(LogCollision, Error, TEXT("RaycastSingle resulted in a NaN/INF in PHit!"));
#if ENABLE_NAN_DIAGNOSTIC
					UE_LOG(LogCollision, Error, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
					UE_LOG(LogCollision, Error, TEXT("--------Start : %s"), *Start.ToString());
					UE_LOG(LogCollision, Error, TEXT("--------End : %s"), *End.ToString());
					UE_LOG(LogCollision, Error, TEXT("--------%s"), *Params.ToString());
#endif
				}
			}
		}

	}


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (World->DebugDrawSceneQueries(Params.TraceTag))
	{
		TArray<FHitResult> Hits;
		if (bHaveBlockingHit)
		{
			Hits.Add(OutHit);
		}
		DrawLineTraces(World, Start, End, Hits, DebugLineLifetime);	
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if ENABLE_COLLISION_ANALYZER
	if (GCollisionAnalyzerIsRecording && IsInGameThread())
	{
		TArray<FHitResult> Hits;
		if (bHaveBlockingHit)
		{
			Hits.Add(OutHit);
		}
		CAPTURERAYCAST(World, Start, End, ECAQueryMode::Single, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
	}
#endif
#endif // WITH_PHYSX

	return bHaveBlockingHit;
}

#if WITH_PHYSX
template<typename HitType>
class FDynamicHitBuffer : public PxHitCallback<HitType>
{
private:
	/** Hit buffer used to provide hits via processTouches */
	TTypeCompatibleBytes<HitType> HitBuffer[HIT_BUFFER_SIZE];

	/** Hits encountered. Can be larger than HIT_BUFFER_SIZE */
	TArray<TTypeCompatibleBytes<HitType>, TInlineAllocator<HIT_BUFFER_SIZE>> Hits;

public:
	FDynamicHitBuffer()
		: PxHitCallback<HitType>((HitType*)HitBuffer, HIT_BUFFER_SIZE)
	{}

	virtual PxAgain processTouches(const HitType* buffer, PxU32 nbHits) override
	{
		Hits.Append((TTypeCompatibleBytes<HitType>*)buffer, nbHits);
		return true;
	}

	virtual void finalizeQuery() override
	{
		if (this->hasBlock)
		{
			// copy blocking hit to hits
			processTouches(&this->block, 1);
		}
	}

	FORCEINLINE int32 GetNumHits() const
	{
		return Hits.Num();
	}

	FORCEINLINE HitType* GetHits()
	{
		return (HitType*)Hits.GetData();
	}
};
#endif // WITH_PHYSX

bool RaycastMulti(const UWorld* World, TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_RaycastMultiple);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	OutHits.Reset();

	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

#if WITH_PHYSX
	FVector Delta = End - Start;
	float DeltaMag = Delta.Size();
	if (DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, true);
		PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER);
		PxHitFlags POutputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE | PxHitFlag::eMTD | PxHitFlag::eFACE_INDEX;
		FPxQueryFilterCallback PQueryCallback(Params);
		FDynamicHitBuffer<PxRaycastHit> PRaycastBuffer;

		bool bBlockingHit = false;
		const PxVec3 PDir = U2PVector(Delta/DeltaMag);

		// Enable scene locks, in case they are required
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		FScopedMultiSceneReadLock SceneLocks;
		SceneLocks.LockRead(World, SyncScene, PST_Sync);
		{
			FScopedSQHitchRepeater<decltype(PRaycastBuffer)> HitchRepeater(PRaycastBuffer, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				SyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
			} while (HitchRepeater.RepeatOnHitch());
		}

		PxI32 NumHits = PRaycastBuffer.GetNumHits();

		if (NumHits == 0)
		{
			// Not going to use anything from this scene, so unlock it now.
			SceneLocks.UnlockRead(SyncScene, PST_Sync);
		}

		// Test async scene if async tests are requested and there was no overflow
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SceneLocks.LockRead(World, AsyncScene, PST_Async);

			// Write into the same PHits buffer
			bool bBlockingHitAsync = false;

			// If we have a blocking hit from the sync scene, there is no need to raycast past that hit
			const float RayLength = bBlockingHit ? PRaycastBuffer.GetHits()[NumHits-1].distance : DeltaMag;

			PxI32 NumAsyncHits = 0;
			if(RayLength > SMALL_NUMBER) // don't bother doing the trace if the sync scene trace gave a hit time of zero
			{
				FScopedSQHitchRepeater<decltype(PRaycastBuffer)> HitchRepeater(PRaycastBuffer, PQueryCallback, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do 
				{
					AsyncScene->raycast(U2PVector(Start), PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());

				NumAsyncHits = PRaycastBuffer.GetNumHits() - NumHits;
			}

			if (NumAsyncHits == 0)
			{
				// Not going to use anything from this scene, so unlock it now.
				SceneLocks.UnlockRead(AsyncScene, PST_Async);
			}

			PxI32 TotalNumHits = NumHits + NumAsyncHits;

			// If there is a blocking hit in the sync scene, and it is closer than the blocking hit in the async scene (or there is no blocking hit in the async scene),
			// then move it to the end of the array to get it out of the way.
			if (bBlockingHit)
			{
				if (!bBlockingHitAsync || PRaycastBuffer.GetHits()[NumHits-1].distance < PRaycastBuffer.GetHits()[TotalNumHits-1].distance)
				{
					PRaycastBuffer.GetHits()[TotalNumHits-1] = PRaycastBuffer.GetHits()[NumHits-1];
				}
			}

			// Merge results
			NumHits = TotalNumHits;

			bBlockingHit = bBlockingHit || bBlockingHitAsync;

			// Now eliminate hits which are farther than the nearest blocking hit, or even those that are the exact same distance as the blocking hit,
			// to ensure the blocking hit is the last in the array after sorting in ConvertRaycastResults (below).
			if (bBlockingHit)
			{
				const PxF32 MaxDistance = PRaycastBuffer.GetHits()[NumHits-1].distance;
				PxI32 TestHitCount = NumHits-1;
				for (PxI32 HitNum = TestHitCount; HitNum-- > 0;)
				{
					if (PRaycastBuffer.GetHits()[HitNum].distance >= MaxDistance)
					{
						PRaycastBuffer.GetHits()[HitNum] = PRaycastBuffer.GetHits()[--TestHitCount];
					}
				}
				if (TestHitCount < NumHits-1)
				{
					PRaycastBuffer.GetHits()[TestHitCount] = PRaycastBuffer.GetHits()[NumHits-1];
					NumHits = TestHitCount + 1;
				}
			}
		}

		if (NumHits > 0)
		{
			if (ConvertRaycastResults(bBlockingHit, World, NumHits, PRaycastBuffer.GetHits(), DeltaMag, PFilter, OutHits, Start, End, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial) == EConvertQueryResult::Invalid)
			{
				// We don't need to change bBlockingHit, that's done by ConvertRaycastResults if it removed the blocking hit.
				UE_LOG(LogCollision, Error, TEXT("RaycastMulti resulted in a NaN/INF in PHit!"));
#if ENABLE_NAN_DIAGNOSTIC
				UE_LOG(LogCollision, Error, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Error, TEXT("--------Start : %s"), *Start.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------End : %s"), *End.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------%s"), *Params.ToString());
#endif
			}
		}

		bHaveBlockingHit = bBlockingHit;

	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(World->DebugDrawSceneQueries(Params.TraceTag))
	{
		DrawLineTraces(World, Start, End, OutHits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	CAPTURERAYCAST(World, Start, End, ECAQueryMode::Multi, TraceChannel, Params, ResponseParams, ObjectParams, OutHits);

#endif // WITH_PHYSX
	return bHaveBlockingHit;
}

//////////////////////////////////////////////////////////////////////////
// GEOM SWEEP

#if WITH_PHYSX

PxU32 FindFaceIndex(const PxSweepHit& PHit, const PxVec3& unitDir)
{
	PxConvexMeshGeometry convexGeom;
	if(PHit.shape->getConvexMeshGeometry(convexGeom))
	{
		//PhysX has given us the most correct face. However, we actually want the most useful face which is the one with the most opposed normal within some radius.
		//So for example, if we are sweeping against a corner we should take the plane that is most opposing, even if it's not the exact one we hit.
		static const float FindFaceInRadius = 1.f; // tolerance to determine how far from the actual contact point we want to search.

		const PxTransform pose = PHit.actor->getGlobalPose() * PHit.shape->getLocalPose();
		const PxVec3 impactPos(PHit.position);
		{
			//This is copied directly from PxFindFace. However, we made some modifications in order to favor 'most opposing' faces.
			static const PxReal gEpsilon = .01f;
			PX_ASSERT(unitDir.isFinite());
			PX_ASSERT(unitDir.isNormalized());
			PX_ASSERT(impactPos.isFinite());
			PX_ASSERT(pose.isFinite());

			const PxVec3 impact = impactPos - unitDir * gEpsilon;

			const PxVec3 localPoint = pose.transformInv(impact);
			const PxVec3 localDir = pose.rotateInv(unitDir);

			// Create shape to vertex scale transformation matrix
			const PxMeshScale& meshScale = convexGeom.scale;
			const PxMat33 rot(meshScale.rotation);
			PxMat33 shape2VertexSkew = rot.getTranspose();
			const PxMat33 diagonal = PxMat33::createDiagonal(PxVec3(1.0f / meshScale.scale.x, 1.0f / meshScale.scale.y, 1.0f / meshScale.scale.z));
			shape2VertexSkew = shape2VertexSkew * diagonal;
			shape2VertexSkew = shape2VertexSkew * rot;

			const PxU32 nbPolys = convexGeom.convexMesh->getNbPolygons();
			// BEGIN EPIC MODIFICATION Improved selection of 'most opposing' face
			bool bMinIndexValid = false;
			PxU32 minIndex = 0;
			PxReal maxD = -PX_MAX_REAL;
			PxU32 maxDIndex = 0;
			PxReal minNormalDot = PX_MAX_REAL;

			for (PxU32 j = 0; j < nbPolys; j++)
			{
				PxHullPolygon hullPolygon;
				convexGeom.convexMesh->getPolygonData(j, hullPolygon);

				// transform hull plane into shape space
				PxPlane plane;
				const PxVec3 tmp = shape2VertexSkew.transformTranspose(PxVec3(hullPolygon.mPlane[0], hullPolygon.mPlane[1], hullPolygon.mPlane[2]));
				const PxReal denom = 1.0f / tmp.magnitude();
				plane.n = tmp * denom;
				plane.d = hullPolygon.mPlane[3] * denom;

				PxReal d = plane.distance(localPoint);
				// Track plane that impact point is furthest point (will be out fallback normal)
				if (d>maxD)
				{
					maxDIndex = j;
					maxD = d;
				}

				//Because we are searching against a convex hull, we will never get multiple faces that are both in front of the contact point _and_ have an opposing normal (except the face we hit).
				//However, we may have just missed a plane which is now "behind" the contact point while still being inside the radius
				if (d<-FindFaceInRadius)
					continue;

				// Calculate direction dot plane normal
				const PxReal normalDot = plane.n.dot(localDir);
				// If this is more opposing than our current 'most opposing' normal, update 'most opposing'
				if (normalDot<minNormalDot)
				{
					minIndex = j;
					bMinIndexValid = true;
					minNormalDot = normalDot;
				}
			}

			// If we found at least one face that we are considered 'on', use best normal
			if (bMinIndexValid)
			{
				return minIndex;
			}
			// Fallback is the face that we are most in front of
			else
			{
				return maxDIndex;
			}
		}
	}

	return PHit.faceIndex;	//If no custom logic just return whatever face index they initially had
}
#endif

bool GeomSweepTest(const UWorld* World, const struct FCollisionShape& CollisionShape, const FQuat& Rot, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepAny);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	bool bHaveBlockingHit = false; // Track if we get any 'blocking' hits

#if WITH_PHYSX
	FPhysXShapeAdaptor ShapeAdaptor(Rot, CollisionShape);
	const PxGeometry& PGeom = ShapeAdaptor.GetGeometry();
	const PxQuat& PGeomRot = ShapeAdaptor.GetGeomOrientation();

	const FVector Delta = End - Start;
	const float DeltaMag = Delta.Size();
	if (DeltaMag > KINDA_SMALL_NUMBER)
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, false);
		PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER | PxQueryFlag::ePOSTFILTER | PxQueryFlag::eANY_HIT);
		PxHitFlags POutputFlags; 

		FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params);
		PQueryCallbackSweep.bIgnoreTouches = true; // pre-filter to ignore touches and only get blocking hits.

		PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxVec3 PDir = U2PVector(Delta/DeltaMag);

		FPhysScene* PhysScene = World->GetPhysicsScene();
		{
			// Enable scene locks, in case they are required
			PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);
			SCOPED_SCENE_READ_LOCK(SyncScene);
			PxSweepBuffer PSweepBuffer;
			FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				SyncScene->sweep(PGeom, PStartTM, PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);
			} while (HitchRepeater.RepeatOnHitch());

			bHaveBlockingHit = PSweepBuffer.hasBlock;
		}

		// Test async scene if async tests are requested and there was no blocking hit was found in the sync scene (since no hit info other than a boolean yes/no is recorded)
		if( !bHaveBlockingHit && Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SCOPED_SCENE_READ_LOCK(AsyncScene);
			PxSweepBuffer PSweepBuffer;
			FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				AsyncScene->sweep(PGeom, PStartTM, PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);
			} while (HitchRepeater.RepeatOnHitch());
			bHaveBlockingHit = PSweepBuffer.hasBlock;
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if(World->DebugDrawSceneQueries(Params.TraceTag))
	{
		TArray<FHitResult> Hits;
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if ENABLE_COLLISION_ANALYZER
	if (GCollisionAnalyzerIsRecording)
	{
		TArray<FHitResult> Hits;
		CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryMode::Test, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
	}
#endif // ENABLE_COLLISION_ANALYZER

#endif // WITH_PHYSX

	//@TODO: BOX2D: Implement GeomSweepTest

	return bHaveBlockingHit;
}

bool GeomSweepSingle(const UWorld* World, const struct FCollisionShape& CollisionShape, const FQuat& Rot, FHitResult& OutHit, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepSingle);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	OutHit = FHitResult();
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

#if WITH_PHYSX
	FPhysXShapeAdaptor ShapeAdaptor(Rot, CollisionShape);
	const PxGeometry& PGeom = ShapeAdaptor.GetGeometry();
	const PxQuat& PGeomRot = ShapeAdaptor.GetGeomOrientation();

	const FVector Delta = End - Start;
	const float DeltaMagSize = Delta.Size();
	const float DeltaMag = FMath::IsNearlyZero(DeltaMagSize) ? 0.f : DeltaMagSize;
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, false);
		//UE_LOG(LogCollision, Log, TEXT("PFilter: %x %x %x %x"), PFilter.word0, PFilter.word1, PFilter.word2, PFilter.word3);
		PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER);
		PxHitFlags POutputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE | PxHitFlag::eMTD;
		FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params);
		PQueryCallbackSweep.bIgnoreTouches = true; // pre-filter to ignore touches and only get blocking hits.

		PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxVec3 PDir = DeltaMag == 0.f ? PxVec3(1.f, 0.f, 0.f) : U2PVector(Delta/DeltaMag);	//If DeltaMag is 0 (equality of float is fine because we sanitized to 0) then just use any normalized direction

		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// Enable scene locks, in case they are required
		FScopedMultiSceneReadLock SceneLocks;
		SceneLocks.LockRead(World, SyncScene, PST_Sync);

		PxSweepBuffer PSweepBuffer;
		{
			FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				SyncScene->sweep(PGeom, PStartTM, PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);

			} while (HitchRepeater.RepeatOnHitch());
		}
		bHaveBlockingHit = PSweepBuffer.hasBlock;
		PxSweepHit PHit = PSweepBuffer.block;

		if (!bHaveBlockingHit)
		{
			// Not using anything from this scene, so unlock it.
			SceneLocks.UnlockRead(SyncScene, PST_Sync);
		}

		// Test async scene if async tests are requested
		if( Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SceneLocks.LockRead(World, AsyncScene, PST_Async);

			bool bHaveBlockingHitAsync;
			PxSweepBuffer PSweepBufferAsync;
			FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				AsyncScene->sweep(PGeom, PStartTM, PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);

			} while (HitchRepeater.RepeatOnHitch());
			bHaveBlockingHitAsync = PSweepBufferAsync.hasBlock;
			PxSweepHit PHitAsync = PSweepBufferAsync.block;

			// If we have a blocking hit in the async scene and there was no sync blocking hit, or if the async blocking hit came first,
			// then this becomes the blocking hit.  We can test the PxSceneQueryImpactHit::distance since the DeltaMag is the same for both queries.
			if (bHaveBlockingHitAsync && (!bHaveBlockingHit || PHitAsync.distance < PHit.distance))
			{
				PHit = PHitAsync;
				bHaveBlockingHit = true;
			}
			else
			{
				// Not using anything from this scene, so unlock it.
				SceneLocks.UnlockRead(AsyncScene, PST_Async);
			}
		}

		if(bHaveBlockingHit) // If we got a hit, convert it to unreal type
		{
			PHit.faceIndex = FindFaceIndex(PHit, PDir);
			if (ConvertQueryImpactHit(World, PHit, OutHit, DeltaMag, PFilter, Start, End, &PGeom, PStartTM, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial) == EConvertQueryResult::Invalid)
			{
				bHaveBlockingHit = false;
				UE_LOG(LogCollision, Error, TEXT("GeomSweepSingle resulted in a NaN/INF in PHit!"));
#if ENABLE_NAN_DIAGNOSTIC
				UE_LOG(LogCollision, Error, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Error, TEXT("--------Start : %s"), *Start.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------End : %s"), *End.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------%s"), *Params.ToString());
#endif
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (World->DebugDrawSceneQueries(Params.TraceTag))
	{
		TArray<FHitResult> Hits;
		if (bHaveBlockingHit)
		{
			Hits.Add(OutHit);
		}
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, Hits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if ENABLE_COLLISION_ANALYZER
	if (GCollisionAnalyzerIsRecording)
	{
		TArray<FHitResult> Hits;
		if (bHaveBlockingHit)
		{
			Hits.Add(OutHit);
		}
		CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryMode::Single, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
	}
#endif

#endif // WITH_PHYSX

	//@TODO: BOX2D: Implement GeomSweepSingle

	return bHaveBlockingHit;
}

#if WITH_PHYSX
bool GeomSweepMulti_PhysX(const UWorld* World, const PxGeometry& PGeom, const PxQuat& PGeomRot, TArray<FHitResult>& OutHits, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomSweepMultiple);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();
	bool bBlockingHit = false;

	const int32 InitialHitCount = OutHits.Num();

	// Create filter data used to filter collisions
	PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, true);
	PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER | PxQueryFlag::ePOSTFILTER);
	PxHitFlags POutputFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eDISTANCE | PxHitFlag::eMTD | PxHitFlag::eFACE_INDEX;
	FPxQueryFilterCallbackSweep PQueryCallbackSweep(Params);

	const FVector Delta = End - Start;
	const float DeltaMagSize = Delta.Size();
	const float DeltaMag = FMath::IsNearlyZero(DeltaMagSize) ? 0.f : DeltaMagSize;
	{
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// Lock scene
		FScopedMultiSceneReadLock SceneLocks;
		SceneLocks.LockRead(World, SyncScene, PST_Sync);

		const PxTransform PStartTM(U2PVector(Start), PGeomRot);
		PxVec3 PDir = DeltaMag == 0.f ? PxVec3(1.f, 0.f, 0.f) : U2PVector(Delta/DeltaMag);	//If DeltaMag is 0 (equality of float is fine because we sanitized to 0) then just use any normalized direction

		// Keep track of closest blocking hit distance.
		float MinBlockDistance = DeltaMag;

		FDynamicHitBuffer<PxSweepHit> PSweepBuffer;
		{
			FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
			do
			{
				SyncScene->sweep(PGeom, PStartTM, PDir, DeltaMag, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);
			} while (HitchRepeater.RepeatOnHitch());
		}
				
		bool bBlockingHitSync = PSweepBuffer.hasBlock;
		PxI32 NumHits = PSweepBuffer.GetNumHits();

		if (bBlockingHitSync)
		{
			MinBlockDistance = PSweepBuffer.block.distance;
			bBlockingHit = true;
		}
		else if (NumHits == 0)
		{
			// Not using anything from this scene, so unlock it.
			SceneLocks.UnlockRead(SyncScene, PST_Sync);
		}

		// Test async scene if async tests are requested and there was no overflow
		if (Params.bTraceAsyncScene && MinBlockDistance > SMALL_NUMBER && PhysScene->HasAsyncScene())
		{
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			SceneLocks.LockRead(World, AsyncScene, PST_Async);

			{
				FScopedSQHitchRepeater<decltype(PSweepBuffer)> HitchRepeater(PSweepBuffer, PQueryCallbackSweep, FHitchDetectionInfo(Start, End, TraceChannel, Params));
				do
				{
					AsyncScene->sweep(PGeom, PStartTM, PDir, MinBlockDistance, HitchRepeater.GetBuffer(), POutputFlags, PQueryFilterData, &PQueryCallbackSweep);
				} while (HitchRepeater.RepeatOnHitch());
			}

			bool bBlockingHitAsync = PSweepBuffer.hasBlock;
			PxI32 NumAsyncHits = PSweepBuffer.GetNumHits() - NumHits;
			if (NumAsyncHits == 0)
			{
				// Not using anything from this scene, so unlock it.
				SceneLocks.UnlockRead(AsyncScene, PST_Async);
			}

			if (bBlockingHitAsync)
			{
				MinBlockDistance = FMath::Min<float>(PSweepBuffer.block.distance, MinBlockDistance);
				bBlockingHit = true;
			}
		}

		NumHits = PSweepBuffer.GetNumHits();

		// Convert all hits to unreal structs. This will remove any hits further than MinBlockDistance, and sort results.
		if (NumHits > 0)
		{
			if (AddSweepResults(bBlockingHit, World, NumHits, PSweepBuffer.GetHits(), DeltaMag, PFilter, OutHits, Start, End, PGeom, PStartTM, MinBlockDistance, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial) == EConvertQueryResult::Invalid)
			{
				// We don't need to change bBlockingHit, that's done by AddSweepResults if it removed the blocking hit.
				UE_LOG(LogCollision, Error, TEXT("GeomSweepMulti resulted in a NaN/INF in PHit!"));
#if ENABLE_NAN_DIAGNOSTIC				
				UE_LOG(LogCollision, Error, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
				UE_LOG(LogCollision, Error, TEXT("--------Start : %s"), *Start.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------End : %s"), *End.ToString());
				UE_LOG(LogCollision, Error, TEXT("--------%s"), *Params.ToString());
#endif
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (World->DebugDrawSceneQueries(Params.TraceTag))
	{
		TArray<FHitResult> OnlyMyHits(OutHits);
		OnlyMyHits.RemoveAt(0, InitialHitCount, false); // Remove whatever was there initially.
		DrawGeomSweeps(World, Start, End, PGeom, PGeomRot, OnlyMyHits, DebugLineLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if ENABLE_COLLISION_ANALYZER
	if (GCollisionAnalyzerIsRecording)
	{
		TArray<FHitResult> OnlyMyHits(OutHits);
		OnlyMyHits.RemoveAt(0, InitialHitCount, false); // Remove whatever was there initially.
		CAPTUREGEOMSWEEP(World, Start, End, PGeomRot, ECAQueryMode::Multi, PGeom, TraceChannel, Params, ResponseParams, ObjectParams, OnlyMyHits);
	}
#endif // ENABLE_COLLISION_ANALYZER

	return bBlockingHit;
}
#endif // WITH_PHYSX 


bool GeomSweepMulti(const UWorld* World, const struct FCollisionShape& CollisionShape, const FQuat& Rot, TArray<FHitResult>& OutHits, FVector Start, FVector End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	OutHits.Reset();

	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	// Track if we get any 'blocking' hits
	bool bBlockingHit = false;

#if WITH_PHYSX
	FPhysXShapeAdaptor ShapeAdaptor(Rot, CollisionShape);
	const PxGeometry& PGeom = ShapeAdaptor.GetGeometry();
	const PxQuat& PGeomRot = ShapeAdaptor.GetGeomOrientation();

	bBlockingHit = GeomSweepMulti_PhysX(World, PGeom, PGeomRot, OutHits, Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
#endif // WITH_PHYSX

	//@TODO: BOX2D: Implement GeomSweepMulti

	return bBlockingHit;
}

//////////////////////////////////////////////////////////////////////////
// GEOM OVERLAP

namespace EQueryInfo
{
	//This is used for templatizing code based on the info we're trying to get out.
	enum Type
	{
		GatherAll,		//get all data and actually return it
		IsBlocking,		//is any of the data blocking? only return a bool so don't bother collecting
		IsAnything		//is any of the data blocking or touching? only return a bool so don't bother collecting
	};
}

#if WITH_PHYSX

template <EQueryInfo::Type InfoType>
bool GeomOverlapMultiImp_PhysX(const UWorld* World, const PxGeometry& PGeom, const PxTransform& PGeomPose, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	SCOPE_CYCLE_COUNTER(STAT_Collision_SceneQueryTotal);
	SCOPE_CYCLE_COUNTER(STAT_Collision_GeomOverlapMultiple);
	FScopeCycleCounter Counter(Params.StatId);
	STARTQUERYTIMER();

	bool bHaveBlockingHit = false;

	// overlapMultiple only supports sphere/capsule/box 
	if (PGeom.getType()==PxGeometryType::eSPHERE || PGeom.getType()==PxGeometryType::eCAPSULE || PGeom.getType()==PxGeometryType::eBOX || PGeom.getType()==PxGeometryType::eCONVEXMESH )
	{
		// Create filter data used to filter collisions
		PxFilterData PFilter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, InfoType != EQueryInfo::IsAnything);
		PxQueryFilterData PQueryFilterData(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER);
		PxQueryFilterData PQueryFilterDataAny(PFilter, StaticDynamicQueryFlags(Params) | PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT);
		FPxQueryFilterCallback PQueryCallback(Params);
		PQueryCallback.bIgnoreTouches |= (InfoType == EQueryInfo::IsBlocking); // pre-filter to ignore touches and only get blocking hits, if that's what we're after.
		PQueryCallback.bIsOverlapQuery = true;

		// Enable scene locks, in case they are required
		FScopedMultiSceneReadLock SceneLocks;
		FPhysScene* PhysScene = World->GetPhysicsScene();
		PxScene* SyncScene = PhysScene->GetPhysXScene(PST_Sync);

		// we can't use scoped because we later do a conversion which depends on these results and it should all be atomic
		SceneLocks.LockRead(World, SyncScene, PST_Sync);

		FDynamicHitBuffer<PxOverlapHit> POverlapBuffer;
		PxI32 NumHits = 0;
		
		if ((InfoType == EQueryInfo::IsAnything) || (InfoType == EQueryInfo::IsBlocking))
		{
			FScopedSQHitchRepeater<decltype(POverlapBuffer)> HitchRepeater(POverlapBuffer, PQueryCallback, FHitchDetectionInfo(PGeomPose, TraceChannel, Params));
			do
			{
				SyncScene->overlap(PGeom, PGeomPose, HitchRepeater.GetBuffer(), PQueryFilterDataAny, &PQueryCallback);
			} while (HitchRepeater.RepeatOnHitch());

			if (POverlapBuffer.hasBlock)
			{
				return true;
			}
		}
		else
		{
			checkSlow(InfoType == EQueryInfo::GatherAll);
			
			FScopedSQHitchRepeater<decltype(POverlapBuffer)> HitchRepeater(POverlapBuffer, PQueryCallback, FHitchDetectionInfo(PGeomPose, TraceChannel, Params));
			do
			{
				SyncScene->overlap(PGeom, PGeomPose, HitchRepeater.GetBuffer(), PQueryFilterData, &PQueryCallback);
			} while (HitchRepeater.RepeatOnHitch());

			NumHits = POverlapBuffer.GetNumHits();
			if (NumHits == 0)
			{
				// Not using anything from this scene, so unlock it.
				SceneLocks.UnlockRead(SyncScene, PST_Sync);
			}
		}

		// Test async scene if async tests are requested and there was no overflow
		if (Params.bTraceAsyncScene && PhysScene->HasAsyncScene())
		{		
			PxScene* AsyncScene = PhysScene->GetPhysXScene(PST_Async);
			
			// we can't use scoped because we later do a conversion which depends on these results and it should all be atomic
			SceneLocks.LockRead(World, AsyncScene, PST_Async);
		
			if ((InfoType == EQueryInfo::IsAnything) || (InfoType == EQueryInfo::IsBlocking))
			{
				FScopedSQHitchRepeater<decltype(POverlapBuffer)> HitchRepeater(POverlapBuffer, PQueryCallback, FHitchDetectionInfo(PGeomPose, TraceChannel, Params));
				do
				{
					AsyncScene->overlap(PGeom, PGeomPose, HitchRepeater.GetBuffer(), PQueryFilterDataAny, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());
				
				if (POverlapBuffer.hasBlock)
				{
					return true;
				}
			}
			else
			{
				checkSlow(InfoType == EQueryInfo::GatherAll);

				FScopedSQHitchRepeater<decltype(POverlapBuffer)> HitchRepeater(POverlapBuffer, PQueryCallback, FHitchDetectionInfo(PGeomPose, TraceChannel, Params));
				do
				{
					AsyncScene->overlap(PGeom, PGeomPose, HitchRepeater.GetBuffer(), PQueryFilterData, &PQueryCallback);
				} while (HitchRepeater.RepeatOnHitch());

				PxI32 NumAsyncHits = POverlapBuffer.GetNumHits() - NumHits;
				if (NumAsyncHits == 0)
				{
					// Not using anything from this scene, so unlock it.
					SceneLocks.UnlockRead(AsyncScene, PST_Async);
				}
			}
		}

		NumHits = POverlapBuffer.GetNumHits();

		if (InfoType == EQueryInfo::GatherAll)	//if we are gathering all we need to actually convert to UE format
		{
			if (NumHits > 0)
			{
				bHaveBlockingHit = ConvertOverlapResults(NumHits, POverlapBuffer.GetHits(), PFilter, OutOverlaps);
			}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (World->DebugDrawSceneQueries(Params.TraceTag))
			{
				DrawGeomOverlaps(World, PGeom, PGeomPose, OutOverlaps, DebugLineLifetime);
			}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		}

#if ENABLE_COLLISION_ANALYZER
		if (GCollisionAnalyzerIsRecording)
		{
			// Determine query mode ('single' doesn't really exist for overlaps)
			ECAQueryMode::Type QueryMode = (InfoType == EQueryInfo::GatherAll)  ? ECAQueryMode::Multi : ECAQueryMode::Test;

			CAPTUREGEOMOVERLAP(World, PGeom, PGeomPose, QueryMode, TraceChannel, Params, ResponseParams, ObjectParams, OutOverlaps);
		}
#endif // ENABLE_COLLISION_ANALYZER
	}
	else
	{
		UE_LOG(LogCollision, Log, TEXT("GeomOverlapMulti : unsupported shape - only supports sphere, capsule, box"));
	}

	return bHaveBlockingHit;
}

bool GeomOverlapMulti_PhysX(const UWorld* World, const PxGeometry& PGeom, const PxTransform& PGeomPose, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	return GeomOverlapMultiImp_PhysX<EQueryInfo::GatherAll>(World, PGeom, PGeomPose, OutOverlaps, TraceChannel, Params, ResponseParams, ObjectParams);
}

#endif

template <EQueryInfo::Type InfoType>
bool GeomOverlapMultiImp(const UWorld* World, const struct FCollisionShape& CollisionShape, const FVector& Pos, const FQuat& Rot, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

#if WITH_PHYSX
	FPhysXShapeAdaptor ShapeAdaptor(Rot, CollisionShape);
	const PxGeometry& PGeom = ShapeAdaptor.GetGeometry();
	const PxTransform& PGeomPose = ShapeAdaptor.GetGeomPose(Pos);
	bHaveBlockingHit = GeomOverlapMultiImp_PhysX<InfoType>(World, PGeom, PGeomPose, OutOverlaps, TraceChannel, Params, ResponseParams, ObjectParams);

#endif // WITH_PHYSX

	//@TODO: BOX2D: Implement GeomOverlapMulti

	return bHaveBlockingHit;
}

bool GeomOverlapMulti(const UWorld* World, const struct FCollisionShape& CollisionShape, const FVector& Pos, const FQuat& Rot, TArray<FOverlapResult>& OutOverlaps, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
#if WITH_PHYSX
	OutOverlaps.Reset();
	return GeomOverlapMultiImp<EQueryInfo::GatherAll>(World, CollisionShape, Pos, Rot, OutOverlaps, TraceChannel, Params, ResponseParams, ObjectParams);
#else
	return false;
#endif // WITH_PHYSX
}

bool GeomOverlapBlockingTest(const UWorld* World, const struct FCollisionShape& CollisionShape, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
#if WITH_PHYSX
	TArray<FOverlapResult> Overlaps;	//needed only for template shared code
	return GeomOverlapMultiImp<EQueryInfo::IsBlocking>(World, CollisionShape, Pos, Rot, Overlaps, TraceChannel, Params, ResponseParams, ObjectParams);
#else
	return false;
#endif // WITH_PHYSX
}

bool GeomOverlapAnyTest(const UWorld* World, const struct FCollisionShape& CollisionShape, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
#if WITH_PHYSX
	TArray<FOverlapResult> Overlaps;	//needed only for template shared code
	return GeomOverlapMultiImp<EQueryInfo::IsAnything>(World, CollisionShape, Pos, Rot, Overlaps, TraceChannel, Params, ResponseParams, ObjectParams);
#else
	return false;
#endif // WITH_PHYSX
}

//////////////////////////////////////////////////////////////////////////

#if WITH_PHYSX

static const PxQuat CapsuleRotator(0.f, 0.707106781f, 0.f, 0.707106781f );

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

	// if 'nothing', then set no bits
	PNewData.word3 |= CreateChannelAndFilter((ECollisionChannel)MultiTrace, ObjectParam.IgnoreMask);

	return PNewData;
}

PxFilterData CreateTraceQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer& InCollisionResponseContainer, const FCollisionQueryParams& Params)
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
	
	// if 'nothing', then set no bits
	PNewData.word3 |= CreateChannelAndFilter((ECollisionChannel)MyChannel, Params.IgnoreMask);

	return PNewData;
}

/** Utility for creating a PhysX PxFilterData for performing a query (trace) against the scene */
PxFilterData CreateQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer& InCollisionResponseContainer, const struct FCollisionQueryParams& QueryParam, const struct FCollisionObjectQueryParams & ObjectParam, const bool bMultitrace)
{
	if (ObjectParam.IsValid() )
	{
		return CreateObjectQueryFilterData(bTraceComplex, (bMultitrace? TRACE_MULTI : TRACE_SINGLE), ObjectParam);
	}
	else
	{
		return CreateTraceQueryFilterData(MyChannel, bTraceComplex, InCollisionResponseContainer, QueryParam);
	}
}
#endif // WITH_PHYSX
