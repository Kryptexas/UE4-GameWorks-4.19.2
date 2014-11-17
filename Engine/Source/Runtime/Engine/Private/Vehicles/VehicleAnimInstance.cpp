// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UVehicleAnimInstance.cpp: Single Node Tree Instance 
	Only plays one animation at a time. 
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationRuntime.h"

/////////////////////////////////////////////////////
// UVehicleAnimInstance
/////////////////////////////////////////////////////

UVehicleAnimInstance::UVehicleAnimInstance(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

class AWheeledVehicle * UVehicleAnimInstance::GetVehicle()
{
	return Cast<AWheeledVehicle> (GetOwningActor());
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

