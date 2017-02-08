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
		const PxTransform LocalPose = Shape->getLocalPose();
		const PxTransform BodyLocalShape = BodyToActorTM * Shape->getLocalPose();
		const PxGeometryHolder& GeomHolder = Shape->getGeometry();
		const float Bounds = PxGeometryQuery::getWorldBounds(GeomHolder.any(), BodyLocalShape, /*inflation=*/1.f).getExtents().magnitude() * 2.f;	//TODO: we have to double this because center might be wrong. Long term, should probably use AABB
		switch (GeomHolder.getType())
		{
			case PxGeometryType::eSPHERE:		Shapes.Emplace(BodyLocalShape, new PxSphereGeometry(GeomHolder.sphere().radius), Bounds); break;
			case PxGeometryType::eCAPSULE:		Shapes.Emplace(BodyLocalShape, new PxCapsuleGeometry(GeomHolder.capsule().radius, GeomHolder.capsule().halfHeight), Bounds); break;
			case PxGeometryType::eBOX:			Shapes.Emplace(BodyLocalShape, new PxBoxGeometry(GeomHolder.box().halfExtents), Bounds); break;
			case PxGeometryType::eCONVEXMESH:	Shapes.Emplace(BodyLocalShape, new PxConvexMeshGeometry(GeomHolder.convexMesh().convexMesh, GeomHolder.convexMesh().scale, GeomHolder.convexMesh().meshFlags), Bounds); break;
			case PxGeometryType::eHEIGHTFIELD:	Shapes.Emplace(BodyLocalShape, new PxHeightFieldGeometry(GeomHolder.heightField().heightField, GeomHolder.heightField().heightFieldFlags, GeomHolder.heightField().heightScale, GeomHolder.heightField().rowScale, GeomHolder.heightField().columnScale), Bounds);
			//case PxGeometryType::eTRIANGLEMESH:	Shapes.Emplace(BodyLocalShape, new PxTriangleMeshGeometry(GeomHolder.triangleMesh().triangleMesh, GeomHolder.triangleMesh().scale, GeomHolder.triangleMesh().meshFlags), Bounds);
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

