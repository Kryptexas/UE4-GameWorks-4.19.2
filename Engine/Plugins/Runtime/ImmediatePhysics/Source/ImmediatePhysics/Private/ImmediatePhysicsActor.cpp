// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ImmediatePhysicsActor.h"
#include "PhysicsPublic.h"
#include "PhysicsEngine/BodySetup.h"

namespace ImmediatePhysics
{

#if WITH_PHYSX
void FActor::CreateGeometry(PxRigidActor* RigidActor, const PxTransform& ActorToBodyTM)
{
	const uint32 NumShapes = RigidActor->getNbShapes();
	TArray<PxShape*> ActorShapes;
	ActorShapes.AddUninitialized(NumShapes);
	RigidActor->getShapes(ActorShapes.GetData(), sizeof(PxShape*)*NumShapes);
	PxTransform BodyToActorTM = ActorToBodyTM.getInverse();
	
	Shapes.Empty(NumShapes);

	for(PxShape* Shape : ActorShapes)
	{
		if(!(Shape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE))
		{
			continue;
		}

		const PxTransform LocalPose = Shape->getLocalPose();
		const PxTransform BodyLocalShape = BodyToActorTM * Shape->getLocalPose();
		const PxGeometryHolder& GeomHolder = Shape->getGeometry();
		const PxBounds3 Bounds = PxGeometryQuery::getWorldBounds(GeomHolder.any(), BodyLocalShape, /*inflation=*/1.f);
		const float BoundsMag = Bounds.getExtents().magnitude();
		const PxVec3 BoundsCenter = Bounds.getCenter();
		switch (GeomHolder.getType())
		{
			case PxGeometryType::eSPHERE:		Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxSphereGeometry(GeomHolder.sphere().radius)); break;
			case PxGeometryType::eCAPSULE:		Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxCapsuleGeometry(GeomHolder.capsule().radius, GeomHolder.capsule().halfHeight)); break;
			case PxGeometryType::eBOX:			Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxBoxGeometry(GeomHolder.box().halfExtents)); break;
			case PxGeometryType::eCONVEXMESH:	Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxConvexMeshGeometry(GeomHolder.convexMesh().convexMesh, GeomHolder.convexMesh().scale, GeomHolder.convexMesh().meshFlags)); break;
			case PxGeometryType::eHEIGHTFIELD:	Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxHeightFieldGeometry(GeomHolder.heightField().heightField, GeomHolder.heightField().heightFieldFlags, GeomHolder.heightField().heightScale, GeomHolder.heightField().rowScale, GeomHolder.heightField().columnScale)); break;
			case PxGeometryType::eTRIANGLEMESH: Shapes.Emplace(BodyLocalShape, BoundsCenter, BoundsMag, new PxTriangleMeshGeometry(GeomHolder.triangleMesh().triangleMesh, GeomHolder.triangleMesh().scale, GeomHolder.triangleMesh().meshFlags)); break;
			default: continue;	//we don't bother with other types
		}
	}
}

void FActor::TerminateGeometry()
{
	for(FShape& Shape : Shapes)
	{
		delete Shape.Geometry;
	}

	Shapes.Empty();
}
#endif

}

