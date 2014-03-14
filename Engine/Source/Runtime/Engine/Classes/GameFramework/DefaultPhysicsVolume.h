// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// DefaultPhysicsVolume:  the default physics volume for areas of the level with
// no physics volume specified
//=============================================================================

#pragma once
#include "DefaultPhysicsVolume.generated.h"

UCLASS(notplaceable, transient, MinimalAPI)
class ADefaultPhysicsVolume : public APhysicsVolume
{
	GENERATED_UCLASS_BODY()

	// Begin AActor interface
	virtual void Destroyed() OVERRIDE;
	// End AActor interface
};



