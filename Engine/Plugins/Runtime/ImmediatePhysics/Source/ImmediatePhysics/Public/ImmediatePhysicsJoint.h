// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace ImmediatePhysics
{

struct FActorHandle;

struct FJoint
{
	FActorHandle* DynamicActor;
	FActorHandle* OtherActor;
};

}