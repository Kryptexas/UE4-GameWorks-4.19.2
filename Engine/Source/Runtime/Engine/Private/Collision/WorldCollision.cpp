// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WorldCollision.cpp: UWorld collision implementation
=============================================================================*/

#include "EnginePrivate.h"
#include "Collision.h"

#if WITH_PHYSX
	#include "../PhysicsEngine/PhysXSupport.h"
#endif

#include "PhysXCollision.h"
#include "CollisionConversions.h"
#include "CollisionDebugDrawing.h"

DEFINE_LOG_CATEGORY(LogCollision);

/** Collision stats */


DEFINE_STAT(STAT_Collision_RaycastAny);
DEFINE_STAT(STAT_Collision_RaycastSingle);
DEFINE_STAT(STAT_Collision_RaycastMultiple);
DEFINE_STAT(STAT_Collision_GeomSweepAny);
DEFINE_STAT(STAT_Collision_GeomSweepSingle);
DEFINE_STAT(STAT_Collision_GeomSweepMultiple);
DEFINE_STAT(STAT_Collision_GeomOverlapAny);
DEFINE_STAT(STAT_Collision_GeomOverlapSingle);
DEFINE_STAT(STAT_Collision_GeomOverlapMultiple);

/** default collision response container - to be used without reconstructing every time**/
FCollisionResponseContainer FCollisionResponseContainer::DefaultResponseContainer(ECR_Block);

/* This is default response param that's used by trace query **/
FCollisionResponseParams		FCollisionResponseParams::DefaultResponseParam;
FCollisionObjectQueryParams		FCollisionObjectQueryParams::DefaultObjectQueryParam;
FCollisionShape					FCollisionShape::LineShape;

// default being the 0. That isn't invalid, but ObjectQuery param overrides this 
ECollisionChannel DefaultCollisionChannel = (ECollisionChannel) 0;

bool UWorld::LineTraceTest(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const
{
#if WITH_PHYSX
	return RaycastTest(this, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
#endif // WITH_PHYSX
	return false;
}


bool UWorld::LineTraceSingle(struct FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const
{
#if WITH_PHYSX
	return RaycastSingle(this, OutHit, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
#else
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;
	return false;
#endif // WITH_PHYSX
}

bool UWorld::LineTraceMulti(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const
{
#if WITH_PHYSX
	return RaycastMulti(this, OutHits, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
#endif // WITH_PHYSX
	return false;
}

bool UWorld::SweepTest(const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const
{
	if(CollisionShape.IsNearlyZero())
	{
		// if extent is 0, we'll just do linetrace instead
		return LineTraceTest(Start, End, TraceChannel, Params);
	}

#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			const PxQuat PGeomRot = U2PQuat(Rot);

			return GeomSweepTest(this, PBoxGeom, PGeomRot, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return GeomSweepTest(this, PSphereGeom, PGeomRot, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
			return GeomSweepTest(this, PCapsuleGeom, PGeomRot, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX
	return false;
}

bool UWorld::SweepSingle(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam) const
{
	if(CollisionShape.IsNearlyZero())
	{
		return LineTraceSingle(OutHit, Start, End, TraceChannel, Params);
	}
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			const PxQuat PGeomRot = U2PQuat(Rot);

			return GeomSweepSingle(this, PBoxGeom, PGeomRot, OutHit, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return GeomSweepSingle(this, PSphereGeom, PGeomRot, OutHit, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
			return GeomSweepSingle(this, PCapsuleGeom, PGeomRot, OutHit, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX

	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;
	return false;
}

bool UWorld::SweepMulti( TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End, const FQuat& Rot,ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam ) const
{
	if(CollisionShape.IsNearlyZero())
	{
		return LineTraceMulti(OutHits, Start, End, TraceChannel, Params);
	}
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			const PxQuat PGeomRot = U2PQuat(Rot);

			return GeomSweepMulti(this, PBoxGeom, PGeomRot, OutHits, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return GeomSweepMulti(this, PSphereGeom, PGeomRot, OutHits, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
			return GeomSweepMulti(this, PCapsuleGeom, PGeomRot, OutHits, Start, End, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}
#endif //WITH_PHYSX
	return false;
}

bool UWorld::OverlapTest(const FVector& Pos, const FQuat& Rot,ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam ) const
{
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			return GeomOverlapTest(this, PBoxGeom, PGeomPose, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			return GeomOverlapTest(this, PSphereGeom, PGeomPose, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			return GeomOverlapTest(this, PCapsuleGeom, PGeomPose, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX
	return false;
}

bool UWorld::OverlapSingle(struct FOverlapResult& OutOverlap, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam ) const
{
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			return GeomOverlapSingle(this, PBoxGeom, PGeomPose, OutOverlap, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			return GeomOverlapSingle(this, PSphereGeom, PGeomPose, OutOverlap, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			return GeomOverlapSingle(this, PCapsuleGeom, PGeomPose, OutOverlap, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}
#endif //WITH_PHYSX
	return false;
}

bool UWorld::OverlapMulti(TArray<struct FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot,ECollisionChannel TraceChannel, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParam ) const
{
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			return GeomOverlapMulti(this, PBoxGeom, PGeomPose, OutOverlaps, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			return GeomOverlapMulti(this, PSphereGeom, PGeomPose, OutOverlaps, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			return GeomOverlapMulti(this, PCapsuleGeom, PGeomPose, OutOverlaps, TraceChannel, Params, ResponseParam, FCollisionObjectQueryParams::DefaultObjectQueryParam);
		}
	default:
		// invalid point
		ensure(false);
	}
#endif //WITH_PHYSX
	return false;
}

// object query interfaces

bool UWorld::LineTraceTest(const FVector& Start,const FVector& End,const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
#if WITH_PHYSX
	return RaycastTest(this, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
#endif
	return false;
}

bool UWorld::LineTraceSingle(struct FHitResult& OutHit,const FVector& Start,const FVector& End,const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
#if WITH_PHYSX
	return RaycastSingle(this, OutHit, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
#else
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;
	return false;
#endif // WITH_PHYSX
}

bool UWorld::LineTraceMulti(TArray<struct FHitResult>& OutHits,const FVector& Start,const FVector& End,const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	// object query returns true if any hit is found, not only blocking hit
#if WITH_PHYSX
	RaycastMulti(this, OutHits, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
#endif // WITH_PHYSX
	return (OutHits.Num() > 0);
}

bool UWorld::SweepTest(const FVector& Start, const FVector& End, const FQuat& Rot, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	if(CollisionShape.IsNearlyZero())
	{
		// if extent is 0, we'll just do linetrace instead
		return LineTraceTest(Start, End, Params, ObjectQueryParams);
	}

#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			const PxQuat PGeomRot = U2PQuat(Rot);

			return GeomSweepTest(this, PBoxGeom, PGeomRot, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return GeomSweepTest(this, PSphereGeom, PGeomRot, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
			return GeomSweepTest(this, PCapsuleGeom, PGeomRot, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX
	return false;
}

bool UWorld::SweepSingle(struct FHitResult& OutHit,const FVector& Start,const FVector& End, const FQuat& Rot, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	if(CollisionShape.IsNearlyZero())
	{
		return LineTraceSingle(OutHit, Start, End, Params, ObjectQueryParams);
	}
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			const PxQuat PGeomRot = U2PQuat(Rot);

			return GeomSweepSingle(this, PBoxGeom, PGeomRot, OutHit, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
			PxQuat PGeomRot = PxQuat::createIdentity();

			return GeomSweepSingle(this, PSphereGeom, PGeomRot, OutHit, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
			return GeomSweepSingle(this, PCapsuleGeom, PGeomRot, OutHit, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX

	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;
	return false;
}

bool UWorld::SweepMulti(TArray<struct FHitResult>& OutHits,const FVector& Start,const FVector& End, const FQuat& Rot, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	// object query returns true if any hit is found, not only blocking hit
	if(CollisionShape.IsNearlyZero())
	{
		LineTraceMulti(OutHits, Start, End, Params, ObjectQueryParams);
	}
#if WITH_PHYSX
	else
	{
		switch (CollisionShape.ShapeType)
		{
		case ECollisionShape::Box:
			{
				const PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
				const PxQuat PGeomRot = U2PQuat(Rot);

				GeomSweepMulti(this, PBoxGeom, PGeomRot, OutHits, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
			}
			break;
		case ECollisionShape::Sphere:
			{
				PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius());
				PxQuat PGeomRot = PxQuat::createIdentity();

				GeomSweepMulti(this, PSphereGeom, PGeomRot, OutHits, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
			}
			break;
		case ECollisionShape::Capsule:
			{
				PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
				const PxQuat PGeomRot = ConvertToPhysXCapsuleRot(Rot);
				GeomSweepMulti(this, PCapsuleGeom, PGeomRot, OutHits, Start, End, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
			}
			break;
		default:
			// invalid point
			ensure(false);
		}
	}
#endif //WITH_PHYSX
	return (OutHits.Num() > 0);
}

bool UWorld::OverlapTest(const FVector& Pos, const FQuat& Rot,const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			return GeomOverlapTest(this, PBoxGeom, PGeomPose, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			return GeomOverlapTest(this, PSphereGeom, PGeomPose, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			return GeomOverlapTest(this, PCapsuleGeom, PGeomPose, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	default:
		// invalid point
		ensure(false);
	}

#endif //WITH_PHYSX
	return false;
}

bool UWorld::OverlapSingle(struct FOverlapResult& OutOverlap,const FVector& Pos, const FQuat& Rot, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			return GeomOverlapSingle(this, PBoxGeom, PGeomPose, OutOverlap, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			return GeomOverlapSingle(this, PSphereGeom, PGeomPose, OutOverlap, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}

	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			return GeomOverlapSingle(this, PCapsuleGeom, PGeomPose, OutOverlap, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
	default:
		// invalid point
		ensure(false);
	}
#endif //WITH_PHYSX
	return false;
}

bool UWorld::OverlapMulti(TArray<struct FOverlapResult>& OutOverlaps,const FVector& Pos, const FQuat& Rot, const struct FCollisionShape & CollisionShape, const struct FCollisionQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	// object query returns true if any hit is found, not only blocking hit
#if WITH_PHYSX
	switch (CollisionShape.ShapeType)
	{
	case ECollisionShape::Box:
		{
			PxBoxGeometry PBoxGeom( U2PVector(CollisionShape.GetBox()) );
			PxTransform PGeomPose = U2PTransform(FTransform(Rot, Pos));

			GeomOverlapMulti(this, PBoxGeom, PGeomPose, OutOverlaps, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
		break;
	case ECollisionShape::Sphere:
		{
			PxSphereGeometry PSphereGeom( CollisionShape.GetSphereRadius() );
			PxTransform PGeomPose(U2PVector(Pos), PxQuat::createIdentity());
			GeomOverlapMulti(this, PSphereGeom, PGeomPose, OutOverlaps, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
		break;
	case ECollisionShape::Capsule:
		{
			PxCapsuleGeometry PCapsuleGeom( CollisionShape.GetCapsuleRadius(), CollisionShape.GetCapsuleAxisHalfLength() );
			PxTransform PGeomPose = ConvertToPhysXCapsulePose( FTransform(Rot,Pos) );

			GeomOverlapMulti(this, PCapsuleGeom, PGeomPose, OutOverlaps, DefaultCollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam, ObjectQueryParams);
		}
		break;
	default:
		// invalid point
		ensure(false);
	}
#endif //WITH_PHYSX
	return (OutOverlaps.Num() > 0);
}

bool UWorld::ComponentOverlapTest(class UPrimitiveComponent* PrimComp, const FVector& Pos, const FRotator& Rot, const struct FComponentQueryParams& Params) const
{
	if(GetPhysicsScene() == NULL)
	{
		return false;
	}

	if(PrimComp == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentOverlapMulti : No PrimComp"));
		return false;
	}

	// if target is skeletalmeshcomponent and do not support singlebody physics, we don't support this yet
	// talk to @JG, SP, LH
	if ( !PrimComp->ShouldTrackOverlaps() )
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentOverlapMulti : (%s) Does not support skeletalmesh with Physics Asset and destructibles."), *PrimComp->GetPathName());
		return false;
	}
#if WITH_PHYSX
	ECollisionChannel TraceChannel = PrimComp->GetCollisionObjectType();

	PxRigidActor* PRigidActor = PrimComp->BodyInstance.GetPxRigidActor();
	if(PRigidActor == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentOverlapMulti : (%s) No physics data"), *PrimComp->GetPathName());
		return false;
	}

	// calculate the test global pose of the actor
	PxTransform PTestGlobalPose = U2PTransform(FTransform(Rot, Pos));

	// Get all the shapes from the actor
	TArray<PxShape*, TInlineAllocator<8>> PShapes;
	PShapes.AddZeroed(PRigidActor->getNbShapes());
	int32 NumShapes = PRigidActor->getShapes(PShapes.GetData(), PShapes.Num());

	// Iterate over each shape
	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);
		// Calc shape global pose
		PxTransform PLocalPose = PShape->getLocalPose();
		PxTransform PShapeGlobalPose = PTestGlobalPose.transform(PLocalPose);

		GET_GEOMETRY_FROM_SHAPE(PGeom, PShape);

		if(PGeom != NULL)
		{
			if( GeomOverlapTest(this, *PGeom, PShapeGlobalPose, TraceChannel, Params, FCollisionResponseParams(PrimComp->GetCollisionResponseToChannels())))
			{
				// in this test, it only matters true or false. 
				// if we found first true, we don't care next test anymore. 
				return true;
			}
		}
	}

#endif //WITH_PHYSX
	return false;
}

bool UWorld::ComponentOverlapMulti(TArray<struct FOverlapResult>& OutOverlaps, const class UPrimitiveComponent* PrimComp, const FVector& Pos, const FRotator& Rot, const struct FComponentQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	if (PrimComp)
	{
		return ComponentOverlapMulti(OutOverlaps, PrimComp, Pos, Rot, PrimComp->GetCollisionObjectType(), Params, ObjectQueryParams);
	}

	UE_LOG(LogCollision, Log, TEXT("ComponentOverlapMulti : No PrimComp"));
	return false;
}

bool UWorld::ComponentOverlapMulti(TArray<struct FOverlapResult>& OutOverlaps, const class UPrimitiveComponent* PrimComp, const FVector& Pos, const FRotator& Rot, ECollisionChannel TestChannel, const struct FComponentQueryParams& Params, const struct FCollisionObjectQueryParams& ObjectQueryParams) const
{
	if(GetPhysicsScene() == NULL)
	{
		return false;
	}

	if(PrimComp == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentOverlapMulti : No PrimComp"));
		return false;
	}

	if ( !PrimComp->ShouldTrackOverlaps() )
	{
		UE_LOG(LogCollision, Warning, TEXT("ComponentOverlapMulti : (%s) not supported."), *PrimComp->GetPathName());
		return false;
	}

	return PrimComp->ComponentOverlapMulti(OutOverlaps, this, Pos, Rot, Params, ObjectQueryParams);
}

bool UWorld::ComponentSweepSingle(struct FHitResult& OutHit,class UPrimitiveComponent* PrimComp, const FVector& Start, const FVector& End, const FRotator& Rot, const struct FComponentQueryParams& Params) const
{
	OutHit.TraceStart = Start;
	OutHit.TraceEnd = End;

	if(GetPhysicsScene() == NULL)
	{
		return false;
	}

	if(PrimComp == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepSingle : No PrimComp"));
		return false;
	}

	// if target is skeletalmeshcomponent and do not support singlebody physics
	if ( !PrimComp->ShouldTrackOverlaps() )
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepSingle : (%s) Does not support skeletalmesh with Physics Asset and destructibles."), *PrimComp->GetPathName());
		return false;
	}

	ECollisionChannel TraceChannel = PrimComp->GetCollisionObjectType();
#if WITH_PHYSX
	// if extent is 0, do line trace
	if (PrimComp->IsZeroExtent())
	{
		return RaycastSingle(this, OutHit, Start, End, TraceChannel, Params, FCollisionResponseParams(PrimComp->GetCollisionResponseToChannels()));
	}

	PxRigidActor* PRigidActor = PrimComp->BodyInstance.GetPxRigidActor();
	if(PRigidActor == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepMulti : (%s) No physics data"), *PrimComp->GetPathName());
		return false;
	}

	// Get all the shapes from the actor
	TArray<PxShape*, TInlineAllocator<8>> PShapes;
	PShapes.AddZeroed(PRigidActor->getNbShapes());
	int32 NumShapes = PRigidActor->getShapes(PShapes.GetData(), PShapes.Num());

	// calculate the test global pose of the actor
	PxTransform PGlobalStartPose = U2PTransform(FTransform(Start));
	PxTransform PGlobalEndPose = U2PTransform(FTransform(End));

	bool bHaveBlockingHit = false;
	PxQuat PGeomRot = U2PQuat(Rot.Quaternion());

	// Iterate over each shape
	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);

		// Calc shape global pose
		PxTransform PLocalShape = PShape->getLocalPose();
		PxTransform PShapeGlobalStartPose = PGlobalStartPose.transform(PLocalShape);
		PxTransform PShapeGlobalEndPose = PGlobalEndPose.transform(PLocalShape);
		// consider localshape rotation for shape rotation
		PxQuat PShapeRot = PGeomRot * PLocalShape.q;

		GET_GEOMETRY_FROM_SHAPE(PGeom, PShape);

		if(PGeom != NULL)
		{
			// @todo UE4, this might not be the best behavior. If we're looking for the most closest, this have to change to save the result, and find the closest one or 
			// any other options, right now if anything hits first, it will return
			if (GeomSweepSingle(this, *PGeom, PShapeRot, OutHit, P2UVector(PShapeGlobalStartPose.p), P2UVector(PShapeGlobalEndPose.p), TraceChannel, Params, FCollisionResponseParams(PrimComp->GetCollisionResponseToChannels())))
			{
				bHaveBlockingHit = true;
				break;
			}
		}
	}

	return bHaveBlockingHit;
#endif //WITH_PHYSX
	return false;
}

bool UWorld::ComponentSweepMulti(TArray<struct FHitResult>& OutHits, class UPrimitiveComponent* PrimComp, const FVector& Start, const FVector& End, const FRotator& Rot, const struct FComponentQueryParams& Params) const
{
	if(GetPhysicsScene() == NULL)
	{
		return false;
	}

	if(PrimComp == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepMulti : No PrimComp"));
		return false;
	}

	// if target is skeletalmeshcomponent and do not support singlebody physics
	if ( !PrimComp->ShouldTrackOverlaps() )
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepMulti : (%s) Does not support skeletalmesh with Physics Asset and destructibles."), *PrimComp->GetPathName());
		return false;
	}

	ECollisionChannel TraceChannel = PrimComp->GetCollisionObjectType();

#if WITH_PHYSX
	// if extent is 0, do line trace
	if (PrimComp->IsZeroExtent())
	{
		return RaycastMulti(this, OutHits, Start, End, TraceChannel, Params, FCollisionResponseParams(PrimComp->GetCollisionResponseToChannels()));
	}

	PxRigidActor* PRigidActor = PrimComp->BodyInstance.GetPxRigidActor();
	if(PRigidActor == NULL)
	{
		UE_LOG(LogCollision, Log, TEXT("ComponentSweepMulti : (%s) No physics data"), *PrimComp->GetPathName());
		return false;
	}
	PxScene * const PScene = PRigidActor->getScene();

	OutHits.Empty();

	// Get all the shapes from the actor
	TArray<PxShape*, TInlineAllocator<8>> PShapes;
	{
		SCOPED_SCENE_READ_LOCK(PScene);
		PShapes.AddZeroed(PRigidActor->getNbShapes());
		PRigidActor->getShapes(PShapes.GetData(), PShapes.Num());
	}

	// calculate the test global pose of the actor
	PxTransform PGlobalStartPose = U2PTransform(FTransform(Start));
	PxTransform PGlobalEndPose = U2PTransform(FTransform(End));

	bool bHaveBlockingHit = false;
	PxQuat PGeomRot = U2PQuat(Rot.Quaternion());

	// Iterate over each shape
	SCENE_LOCK_READ(PScene);
	for(int32 ShapeIdx=0; ShapeIdx<PShapes.Num(); ShapeIdx++)
	{
		PxShape* PShape = PShapes[ShapeIdx];
		check(PShape);

		TArray<struct FHitResult> Hits;

		// Calc shape global pose
		PxTransform PLocalShape = PShape->getLocalPose();
		PxTransform PShapeGlobalStartPose = PGlobalStartPose.transform(PLocalShape);
		PxTransform PShapeGlobalEndPose = PGlobalEndPose.transform(PLocalShape);
		// consider localshape rotation for shape rotation
		PxQuat PShapeRot = PGeomRot * PLocalShape.q;

		GET_GEOMETRY_FROM_SHAPE(PGeom, PShape);

		if(PGeom != NULL)
		{
			SCENE_UNLOCK_READ(PScene);
			if (GeomSweepMulti(this, *PGeom, PShapeRot, Hits, P2UVector(PShapeGlobalStartPose.p), P2UVector(PShapeGlobalEndPose.p), TraceChannel, Params, FCollisionResponseParams(PrimComp->GetCollisionResponseToChannels())))
			{
				bHaveBlockingHit = true;
			}

			OutHits.Append(Hits);
			SCENE_LOCK_READ(PScene);
		}
	}
	SCENE_UNLOCK_READ(PScene);

	return bHaveBlockingHit;
#endif //WITH_PHYSX
	return false;
}


#if ENABLE_COLLISION_ANALYZER

#include "CollisionAnalyzerModule.h"

static class FCollisionExec : private FSelfRegisteringExec
{
public:
	/** Console commands, see embeded usage statement **/
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE
	{
#if ENABLE_COLLISION_ANALYZER
		if (FParse::Command(&Cmd, TEXT("CANALYZER")))
		{
			FGlobalTabmanager::Get()->InvokeTab(FName(TEXT("CollisionAnalyzerApp")));
			return true;
		}
#endif // ENABLE_COLLISION_ANALYZER
		return false;
	}
} CollisionExec;

#endif // ENABLE_COLLISION_ANALYZER


