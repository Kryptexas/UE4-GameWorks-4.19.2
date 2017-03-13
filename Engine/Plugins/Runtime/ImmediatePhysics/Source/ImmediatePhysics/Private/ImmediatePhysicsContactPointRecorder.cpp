// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ImmediatePhysicsContactPointRecorder.h"
#include "ImmediatePhysicsContactPair.h"
#include "ImmediatePhysicsSimulation.h"

namespace ImmediatePhysics
{

bool FContactPointRecorder::recordContacts(const Gu::ContactPoint* ContactPoints, const PxU32 NumContacts, const PxU32 Index)
{
	FContactPair ContactPair;
	ContactPair.DynamicActorDataIndex = DynamicActorDataIndex;
	ContactPair.OtherActorDataIndex = OtherActorDataIndex;
	ContactPair.StartContactIndex = Simulation.ContactPoints.Num();
	ContactPair.NumContacts = NumContacts;
	ContactPair.PairIdx = PairIdx;

	for (PxU32 ContactIndex = 0; ContactIndex < NumContacts; ++ContactIndex)
	{
		//Fill in solver-specific data that our contact gen does not produce...

		Gu::ContactPoint NewPoint = ContactPoints[ContactIndex];
		NewPoint.maxImpulse = PX_MAX_F32;
		NewPoint.targetVel = PxVec3(0.f);
		NewPoint.staticFriction = 0.7f;
		NewPoint.dynamicFriction = 0.7f;
		NewPoint.restitution = 0.3f;
		NewPoint.materialFlags = 0;
		Simulation.ContactPoints.Add(NewPoint);
	}

	Simulation.ContactPairs.Add(ContactPair);
	return true;
}

}